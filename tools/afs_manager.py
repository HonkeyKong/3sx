#!/usr/bin/env python3
"""Cross-platform AFS archive manager with CLI and Tk GUI.

This tool intentionally uses only the Python standard library. It understands
the AFS variant used by SF33RD.AFS and preserves the opaque portion of each
48-byte attribute record when archives are edited.
"""

from __future__ import annotations

import argparse
import base64
import json
import os
import re
import shutil
import struct
import sys
import tempfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import BinaryIO, Iterable

MAGIC = b"AFS\0"
ATTRIBUTE_SIZE = 48
NAME_SIZE = 32
DEFAULT_ALIGNMENT = 2048
MANIFEST_NAME = "afs_manifest.json"
MAX_ENTRIES = 1_000_000
COPY_CHUNK = 1024 * 1024


class AFSError(Exception):
    """Raised for malformed archives or unsafe operations."""


def align(value: int, alignment: int) -> int:
    if alignment <= 0 or alignment & (alignment - 1):
        raise AFSError("alignment must be a positive power of two")
    return (value + alignment - 1) & -alignment


def decode_name(raw: bytes) -> str:
    raw = raw.split(b"\0", 1)[0]
    for encoding in ("cp932", "utf-8"):
        try:
            return raw.decode(encoding)
        except UnicodeDecodeError:
            pass
    return raw.decode("cp932", errors="replace")


def encode_name(name: str) -> bytes:
    try:
        raw = name.encode("cp932")
    except UnicodeEncodeError as exc:
        raise AFSError(f"name is not representable as CP932: {name!r}") from exc
    if len(raw) >= NAME_SIZE:
        raise AFSError(f"encoded AFS name is longer than {NAME_SIZE - 1} bytes: {name!r}")
    return raw + b"\0" * (NAME_SIZE - len(raw))


def safe_filename(name: str, index: int) -> str:
    name = name.replace("\\", "/").split("/")[-1]
    name = re.sub(r"[\x00-\x1f<>:\"/\\|?*]", "_", name).strip(" .")
    if not name or name in {".", ".."}:
        name = f"entry_{index:05d}.bin"
    return f"{index:05d}_{name}"


@dataclass
class EntrySource:
    path: Path
    offset: int
    size: int

    def copy_to(self, destination: BinaryIO) -> None:
        with self.path.open("rb") as source:
            source.seek(self.offset)
            remaining = self.size
            while remaining:
                block = source.read(min(remaining, COPY_CHUNK))
                if not block:
                    raise AFSError(f"unexpected end of file while reading {self.path}")
                destination.write(block)
                remaining -= len(block)


@dataclass
class AFSEntry:
    name: str
    size: int
    source: EntrySource | None
    attributes: bytes = field(default_factory=lambda: b"\0" * ATTRIBUTE_SIZE)

    @property
    def is_empty(self) -> bool:
        return self.source is None


class AFSArchive:
    def __init__(self, path: Path | None, entries: list[AFSEntry]):
        self.path = path
        self.entries = entries

    @classmethod
    def open(cls, path: os.PathLike[str] | str) -> "AFSArchive":
        archive_path = Path(path).resolve()
        file_size = archive_path.stat().st_size
        with archive_path.open("rb") as stream:
            if stream.read(4) != MAGIC:
                raise AFSError(f"not an AFS archive: {archive_path}")
            count_raw = stream.read(4)
            if len(count_raw) != 4:
                raise AFSError("truncated AFS header")
            count = struct.unpack("<I", count_raw)[0]
            if count > MAX_ENTRIES or 16 + count * 8 > file_size:
                raise AFSError(f"invalid AFS entry count: {count}")

            records: list[tuple[int, int]] = []
            for _ in range(count):
                raw = stream.read(8)
                if len(raw) != 8:
                    raise AFSError("truncated AFS entry table")
                offset, size = struct.unpack("<II", raw)
                if offset == 0:
                    if size != 0:
                        raise AFSError("empty AFS entry has a nonzero size")
                elif offset > file_size or size > file_size - offset:
                    raise AFSError("AFS entry points outside the archive")
                records.append((offset, size))

            locator_position = stream.tell()
            locator = stream.read(8)
            attributes = cls._read_attributes(stream, locator, file_size, count, locator_position)
            if attributes is None:
                first_offset = min((offset for offset, _ in records if offset), default=0)
                if first_offset >= 8:
                    stream.seek(first_offset - 8)
                    attributes = cls._read_attributes(
                        stream, stream.read(8), file_size, count, locator_position
                    )

            entries: list[AFSEntry] = []
            for index, (offset, size) in enumerate(records):
                raw_attributes = attributes[index] if attributes else b"\0" * ATTRIBUTE_SIZE
                name = decode_name(raw_attributes[:NAME_SIZE]) if attributes else ""
                source = EntrySource(archive_path, offset, size) if offset else None
                entries.append(AFSEntry(name=name, size=size, source=source, attributes=raw_attributes))
        return cls(archive_path, entries)

    @staticmethod
    def _read_attributes(
        stream: BinaryIO, locator: bytes, file_size: int, count: int, table_end: int
    ) -> list[bytes] | None:
        if len(locator) != 8:
            return None
        offset, size = struct.unpack("<II", locator)
        required = count * ATTRIBUTE_SIZE
        if not offset or size < required or offset < table_end or offset > file_size - size:
            return None
        stream.seek(offset)
        data = stream.read(required)
        if len(data) != required:
            return None
        return [data[i : i + ATTRIBUTE_SIZE] for i in range(0, required, ATTRIBUTE_SIZE)]

    @classmethod
    def from_directory(cls, directory: os.PathLike[str] | str) -> "AFSArchive":
        root = Path(directory).resolve()
        manifest_path = root / MANIFEST_NAME
        if manifest_path.exists():
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            if manifest.get("format") != "3sx-afs-manifest-v1":
                raise AFSError("unsupported AFS manifest format")
            entries = []
            for item in manifest["entries"]:
                raw = base64.b64decode(item.get("attributes", ""), validate=True)
                if len(raw) != ATTRIBUTE_SIZE:
                    raw = b"\0" * ATTRIBUTE_SIZE
                if item.get("empty"):
                    entries.append(AFSEntry(item.get("name", ""), 0, None, raw))
                    continue
                relative = Path(item["file"])
                if relative.is_absolute() or ".." in relative.parts:
                    raise AFSError(f"unsafe path in manifest: {relative}")
                path = (root / relative).resolve()
                if root not in path.parents:
                    raise AFSError(f"manifest path escapes unpack directory: {relative}")
                size = path.stat().st_size
                entries.append(AFSEntry(item.get("name", path.name), size, EntrySource(path, 0, size), raw))
            return cls(None, entries)

        files = sorted(path for path in root.iterdir() if path.is_file())
        entries = [AFSEntry(path.name, path.stat().st_size, EntrySource(path, 0, path.stat().st_size)) for path in files]
        return cls(None, entries)

    def replace(self, index: int, path: os.PathLike[str] | str, name: str | None = None) -> None:
        if not 0 <= index < len(self.entries):
            raise AFSError(f"entry index out of range: {index}")
        source_path = Path(path).resolve()
        size = source_path.stat().st_size
        old = self.entries[index]
        self.entries[index] = AFSEntry(name if name is not None else old.name, size, EntrySource(source_path, 0, size), old.attributes)

    def insert(self, index: int, path: os.PathLike[str] | str, name: str | None = None) -> None:
        if not 0 <= index <= len(self.entries):
            raise AFSError(f"insertion index out of range: {index}")
        source_path = Path(path).resolve()
        size = source_path.stat().st_size
        self.entries.insert(index, AFSEntry(name or source_path.name, size, EntrySource(source_path, 0, size)))

    def extract(self, index: int, destination: os.PathLike[str] | str) -> Path:
        if not 0 <= index < len(self.entries):
            raise AFSError(f"entry index out of range: {index}")
        entry = self.entries[index]
        if entry.source is None:
            raise AFSError(f"entry {index} is empty")
        output = Path(destination)
        output.parent.mkdir(parents=True, exist_ok=True)
        with output.open("wb") as stream:
            entry.source.copy_to(stream)
        return output

    def unpack(self, directory: os.PathLike[str] | str) -> Path:
        root = Path(directory)
        files_root = root / "files"
        files_root.mkdir(parents=True, exist_ok=True)
        manifest_entries = []
        used_names: set[str] = set()
        for index, entry in enumerate(self.entries):
            item = {
                "index": index,
                "name": entry.name,
                "size": entry.size,
                "attributes": base64.b64encode(entry.attributes).decode("ascii"),
            }
            if entry.source is None:
                item["empty"] = True
            else:
                filename = safe_filename(entry.name, index)
                while filename.casefold() in used_names:
                    filename = f"{index:05d}_{filename}"
                used_names.add(filename.casefold())
                relative = Path("files") / filename
                self.extract(index, root / relative)
                item["file"] = relative.as_posix()
            manifest_entries.append(item)
        manifest = {"format": "3sx-afs-manifest-v1", "entries": manifest_entries}
        manifest_path = root / MANIFEST_NAME
        manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        return manifest_path

    def write(self, destination: os.PathLike[str] | str, alignment: int = DEFAULT_ALIGNMENT) -> Path:
        output = Path(destination).resolve()
        output.parent.mkdir(parents=True, exist_ok=True)
        header_size = 16 + len(self.entries) * 8
        cursor = align(header_size, alignment)
        offsets: list[int] = []
        for entry in self.entries:
            if entry.source is None:
                offsets.append(0)
            else:
                offsets.append(cursor)
                cursor = align(cursor + entry.size, alignment)
        attributes_offset = cursor
        attributes_size = len(self.entries) * ATTRIBUTE_SIZE

        fd, temporary_name = tempfile.mkstemp(prefix=f".{output.name}.", suffix=".tmp", dir=output.parent)
        try:
            with os.fdopen(fd, "w+b") as stream:
                stream.write(MAGIC)
                stream.write(struct.pack("<I", len(self.entries)))
                for offset, entry in zip(offsets, self.entries):
                    stream.write(struct.pack("<II", offset, entry.size if offset else 0))
                stream.write(struct.pack("<II", attributes_offset, attributes_size))

                for offset, entry in zip(offsets, self.entries):
                    if not offset:
                        continue
                    stream.seek(offset)
                    assert entry.source is not None
                    entry.source.copy_to(stream)

                stream.seek(attributes_offset)
                for entry in self.entries:
                    metadata = bytearray(entry.attributes[:ATTRIBUTE_SIZE].ljust(ATTRIBUTE_SIZE, b"\0"))
                    metadata[:NAME_SIZE] = encode_name(entry.name)
                    metadata[44:48] = struct.pack("<I", entry.size)
                    stream.write(metadata)
                stream.flush()
                os.fsync(stream.fileno())
            os.replace(temporary_name, output)
        except BaseException:
            try:
                os.unlink(temporary_name)
            except FileNotFoundError:
                pass
            raise
        refreshed = self.open(output)
        self.path = refreshed.path
        self.entries = refreshed.entries
        return output


def parse_index(value: str) -> int:
    try:
        index = int(value, 0)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(f"invalid entry index: {value}") from exc
    if index < 0:
        raise argparse.ArgumentTypeError("entry index cannot be negative")
    return index


def run_cli(args: argparse.Namespace) -> int:
    if args.command == "pack":
        archive = AFSArchive.from_directory(args.directory)
        archive.write(args.output, args.alignment)
        return 0

    archive = AFSArchive.open(args.archive)
    if args.command == "list":
        for index, entry in enumerate(archive.entries):
            print(f"{index:5d}  {entry.size:10d}  {entry.name}")
    elif args.command == "extract":
        entry = archive.entries[args.index]
        destination = Path(args.output) if args.output else Path(safe_filename(entry.name, args.index))
        archive.extract(args.index, destination)
    elif args.command == "unpack":
        archive.unpack(args.directory)
    elif args.command in {"replace", "insert"}:
        if args.command == "replace":
            archive.replace(args.index, args.file, args.name)
        else:
            archive.insert(args.index, args.file, args.name)
        archive.write(args.output, args.alignment)
    return 0


def launch_gui(initial_archive: str | None = None) -> None:
    try:
        import tkinter as tk
        from tkinter import filedialog, messagebox, simpledialog, ttk
    except ImportError as exc:
        raise AFSError("Tkinter is unavailable; use the command-line interface") from exc

    class App:
        def __init__(self) -> None:
            self.archive: AFSArchive | None = None
            self.root = tk.Tk()
            self.root.title("3SX AFS Manager")
            self.root.geometry("850x560")
            toolbar = ttk.Frame(self.root, padding=6)
            toolbar.pack(fill="x")
            for label, action in (
                ("Open", self.open_archive), ("Save As", self.save_as), ("Extract", self.extract_selected),
                ("Unpack All", self.unpack_all), ("Replace", self.replace_selected), ("Insert", self.insert_entry),
                ("Pack Folder", self.pack_folder),
            ):
                ttk.Button(toolbar, text=label, command=action).pack(side="left", padx=2)
            self.tree = ttk.Treeview(self.root, columns=("index", "size", "name"), show="headings", selectmode="browse")
            self.tree.heading("index", text="Index")
            self.tree.heading("size", text="Size")
            self.tree.heading("name", text="Name")
            self.tree.column("index", width=80, anchor="e")
            self.tree.column("size", width=120, anchor="e")
            self.tree.column("name", width=580)
            self.tree.pack(fill="both", expand=True, padx=6)
            self.status = tk.StringVar(value="Open an AFS archive to begin")
            ttk.Label(self.root, textvariable=self.status, padding=6).pack(fill="x")
            if initial_archive:
                self.load(Path(initial_archive))

        def guarded(self, action) -> None:
            try:
                action()
            except (AFSError, OSError, ValueError, json.JSONDecodeError) as exc:
                messagebox.showerror("AFS Manager", str(exc), parent=self.root)

        def selected_index(self) -> int:
            selected = self.tree.selection()
            if not selected:
                raise AFSError("select an entry first")
            return int(self.tree.item(selected[0], "values")[0])

        def refresh(self) -> None:
            self.tree.delete(*self.tree.get_children())
            if not self.archive:
                return
            for index, entry in enumerate(self.archive.entries):
                self.tree.insert("", "end", values=(index, entry.size, entry.name))
            self.status.set(f"{len(self.archive.entries)} entries")

        def load(self, path: Path) -> None:
            self.guarded(lambda: self._load(path))

        def _load(self, path: Path) -> None:
            self.archive = AFSArchive.open(path)
            self.root.title(f"3SX AFS Manager — {path.name}")
            self.refresh()

        def open_archive(self) -> None:
            path = filedialog.askopenfilename(parent=self.root, filetypes=(("AFS archives", "*.afs"), ("All files", "*")))
            if path:
                self.load(Path(path))

        def save_as(self) -> None:
            if not self.archive:
                return
            path = filedialog.asksaveasfilename(parent=self.root, defaultextension=".afs", filetypes=(("AFS archives", "*.afs"),))
            if path:
                self.guarded(lambda: self.archive.write(path))

        def extract_selected(self) -> None:
            if not self.archive:
                return
            def action() -> None:
                index = self.selected_index()
                entry = self.archive.entries[index]
                path = filedialog.asksaveasfilename(parent=self.root, initialfile=safe_filename(entry.name, index))
                if path:
                    self.archive.extract(index, path)
            self.guarded(action)

        def unpack_all(self) -> None:
            if not self.archive:
                return
            directory = filedialog.askdirectory(parent=self.root)
            if directory:
                self.guarded(lambda: self.archive.unpack(directory))

        def replace_selected(self) -> None:
            if not self.archive:
                return
            def action() -> None:
                index = self.selected_index()
                path = filedialog.askopenfilename(parent=self.root)
                if path:
                    self.archive.replace(index, path)
                    self.refresh()
            self.guarded(action)

        def insert_entry(self) -> None:
            if not self.archive:
                return
            def action() -> None:
                selected = self.tree.selection()
                index = self.selected_index() if selected else len(self.archive.entries)
                path = filedialog.askopenfilename(parent=self.root)
                if path:
                    name = simpledialog.askstring("AFS entry name", "Name:", initialvalue=Path(path).name, parent=self.root)
                    if name is not None:
                        self.archive.insert(index, path, name)
                        self.refresh()
            self.guarded(action)

        def pack_folder(self) -> None:
            directory = filedialog.askdirectory(parent=self.root)
            if not directory:
                return
            output = filedialog.asksaveasfilename(parent=self.root, defaultextension=".afs", filetypes=(("AFS archives", "*.afs"),))
            if output:
                self.guarded(lambda: AFSArchive.from_directory(directory).write(output))

        def run(self) -> None:
            self.root.mainloop()

    App().run()


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Inspect and edit AFS archives")
    subparsers = parser.add_subparsers(dest="command")
    gui = subparsers.add_parser("gui", help="launch the graphical interface")
    gui.add_argument("archive", nargs="?")
    listing = subparsers.add_parser("list", help="list archive entries")
    listing.add_argument("archive")
    extract = subparsers.add_parser("extract", help="extract one entry")
    extract.add_argument("archive")
    extract.add_argument("index", type=parse_index)
    extract.add_argument("output", nargs="?")
    unpack = subparsers.add_parser("unpack", help="extract all entries and write a manifest")
    unpack.add_argument("archive")
    unpack.add_argument("directory")
    pack = subparsers.add_parser("pack", help="pack a manifest directory or flat folder")
    pack.add_argument("directory")
    pack.add_argument("output")
    pack.add_argument("--alignment", type=parse_index, default=DEFAULT_ALIGNMENT)
    for command in ("replace", "insert"):
        edit = subparsers.add_parser(command, help=f"{command} an archive entry and write a new archive")
        edit.add_argument("archive")
        edit.add_argument("index", type=parse_index)
        edit.add_argument("file")
        edit.add_argument("output")
        edit.add_argument("--name")
        edit.add_argument("--alignment", type=parse_index, default=DEFAULT_ALIGNMENT)
    return parser


def main(argv: Iterable[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        if args.command in (None, "gui"):
            launch_gui(getattr(args, "archive", None))
            return 0
        return run_cli(args)
    except (AFSError, OSError, ValueError, json.JSONDecodeError) as exc:
        parser.error(str(exc))
        return 2


if __name__ == "__main__":
    raise SystemExit(main())

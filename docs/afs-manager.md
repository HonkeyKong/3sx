# AFS Manager

`tools/afs_manager.py` is a standard-library Python tool for inspecting and
editing AFS archives. It provides a Tk GUI by default and matching command-line
operations for automation. It does not include or download game data.

Launch the GUI:

```bash
python3 tools/afs_manager.py
python3 tools/afs_manager.py gui /path/to/SF33RD.AFS
```

CLI examples:

```bash
python3 tools/afs_manager.py list archive.afs
python3 tools/afs_manager.py extract archive.afs 17 output.bin
python3 tools/afs_manager.py unpack archive.afs unpacked/
python3 tools/afs_manager.py pack unpacked/ rebuilt.afs
python3 tools/afs_manager.py replace archive.afs 17 replacement.bin edited.afs
python3 tools/afs_manager.py insert archive.afs 17 new.bin edited.afs --name NEW.BIN
```

Unpacking writes extracted payloads under `files/` and an
`afs_manifest.json`. Packing that directory preserves entry order, names, empty
entries, and opaque attribute metadata. Packing a directory without a manifest
uses its regular files in sorted order.

Archive writes use a temporary file followed by an atomic replacement. Choose a
new output path when preserving the source archive is important. The default
payload alignment is 2048 bytes and can be changed for CLI packing/editing with
`--alignment`.

## Runtime usage report

When the game unloads, it writes `afs_usage.txt` to the 3SX preference
directory. For libretro builds this is the frontend's save directory under
`3sx/`. The report lists every fully loaded entry, a summary, and every entry
that was not fully loaded:

```text
LOADED index=17 size=12345 name="EXAMPLE.BIN"
SUMMARY loaded=1 unused=2 total=3
UNUSED index=0 size=4096 name="UNUSED.BIN"
```

An entry counts as loaded only after its complete payload has been read;
opening, partially reading, or canceling a request does not count. The runtime
log callback also receives `[AFS_USAGE]` records as entries complete and when
the final report is generated.

The numeric index remains authoritative when an archive has no filename
attributes or an entry's name is empty.

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path


SPEC = importlib.util.spec_from_file_location("afs_manager", Path(__file__).parents[1] / "tools" / "afs_manager.py")
afs = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
sys.modules[SPEC.name] = afs
SPEC.loader.exec_module(afs)


class AFSManagerTests(unittest.TestCase):
    def test_pack_parse_unpack_and_repack(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            inputs = root / "input"
            inputs.mkdir()
            (inputs / "first.bin").write_bytes(b"first payload")
            (inputs / "second.dat").write_bytes(bytes(range(64)))

            archive_path = root / "test.afs"
            afs.AFSArchive.from_directory(inputs).write(archive_path)
            archive = afs.AFSArchive.open(archive_path)
            self.assertEqual([entry.name for entry in archive.entries], ["first.bin", "second.dat"])
            self.assertEqual(archive.entries[0].source.offset % afs.DEFAULT_ALIGNMENT, 0)

            unpacked = root / "unpacked"
            archive.unpack(unpacked)
            repacked = root / "repacked.afs"
            afs.AFSArchive.from_directory(unpacked).write(repacked)
            roundtrip = afs.AFSArchive.open(repacked)
            extracted = root / "result.bin"
            roundtrip.extract(1, extracted)
            self.assertEqual(extracted.read_bytes(), bytes(range(64)))

    def test_replace_and_insert(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            one = root / "one"
            two = root / "two"
            three = root / "three"
            one.write_bytes(b"1")
            two.write_bytes(b"22")
            three.write_bytes(b"333")
            archive = afs.AFSArchive(None, [afs.AFSEntry("one", 1, afs.EntrySource(one, 0, 1))])
            archive.replace(0, two, "two")
            archive.insert(0, three, "three")
            output = root / "edited.afs"
            archive.write(output)
            archive.write(root / "edited-again.afs")
            edited = afs.AFSArchive.open(root / "edited-again.afs")
            self.assertEqual([(entry.name, entry.size) for entry in edited.entries], [("three", 3), ("two", 2)])

    def test_safe_filename_removes_paths(self):
        self.assertEqual(afs.safe_filename("../../bad\\name.bin", 7), "00007_name.bin")


if __name__ == "__main__":
    unittest.main()

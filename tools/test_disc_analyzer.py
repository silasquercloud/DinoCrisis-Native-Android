import tempfile
import unittest
from pathlib import Path

from disc_analyzer import SECTOR_SIZE, analyze, parse_cue


def both_endian(value: int) -> bytes:
    return value.to_bytes(4, "little") + value.to_bytes(4, "big")


def mode2_sector(payload: bytes, mode: int = 2) -> bytes:
    payload = payload.ljust(2048, b"\0")[:2048]
    return b"\0\xff" + b"\xff" * 10 + bytes((0, 0, 0, mode)) + b"\0" * 8 + payload + b"\0" * (SECTOR_SIZE - 24 - 2048)


class DiscAnalyzerTest(unittest.TestCase):
    def test_parses_cue_tracks_and_indexes(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "data.bin").write_bytes(b"\0" * (SECTOR_SIZE + 3))
            (root / "audio.bin").write_bytes(b"\0" * (SECTOR_SIZE * 2))
            (root / "disc.cue").write_text(
                'FILE "data.bin" BINARY\n TRACK 01 MODE2/2352\n INDEX 01 00:00:00\n'
                'FILE "audio.bin" BINARY\n TRACK 02 AUDIO\n INDEX 00 00:00:00\n INDEX 01 00:02:00\n',
                encoding="ascii")
            tracks = parse_cue(root / "disc.cue")
            self.assertEqual([track.sector_count for track in tracks], [1, 2])
            self.assertEqual(tracks[0].trailing_bytes, 3)
            self.assertEqual(tracks[1].indexes[1].lba, 150)

    def test_reads_iso_directory_and_psx_exe_header(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sectors = [bytearray(SECTOR_SIZE) for _ in range(40)]
            pvd = bytearray(2048)
            pvd[:7] = b"\x01CD001\x01"
            pvd[40:72] = b"SYNTHETIC".ljust(32, b" ")
            pvd[80:84] = (40).to_bytes(4, "little")
            root_record = bytearray(34)
            root_record[0] = 34
            root_record[2:10] = both_endian(22)
            root_record[10:18] = both_endian(2048)
            root_record[32] = 1
            root_record[33] = 0
            pvd[156:190] = root_record
            sectors[16] = bytearray(mode2_sector(pvd))
            entry = bytearray(44)
            entry[0] = 44
            entry[2:10] = both_endian(30)
            entry[10:18] = both_endian(2048)
            entry[32] = 10
            entry[33:43] = b"BOOT.EXE;1"
            sectors[22] = bytearray(mode2_sector(entry))
            exe = bytearray(2048)
            exe[:8] = b"PS-X EXE"
            exe[0x10:0x14] = (0x80010000).to_bytes(4, "little")
            exe[0x18:0x1C] = (0x80010000).to_bytes(4, "little")
            exe[0x1C:0x20] = (64).to_bytes(4, "little")
            sectors[30] = bytearray(mode2_sector(exe))
            (root / "data.bin").write_bytes(b"".join(sectors))
            (root / "disc.cue").write_text('FILE "data.bin" BINARY\n TRACK 01 MODE2/2352\n INDEX 01 00:00:00\n', encoding="ascii")
            result = analyze(root / "disc.cue")
            self.assertEqual(result.iso_entries[0].name, "BOOT.EXE;1")
            self.assertEqual(result.psx_exe["sector"], 30)
            self.assertEqual(result.psx_exe["entry_point"], 0x80010000)
            self.assertFalse(result.native_recompilation["possible_from_metadata"])
            self.assertIn("MIPS instructions", result.native_recompilation["blockers"][0])


if __name__ == "__main__":
    unittest.main()
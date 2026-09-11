#!/usr/bin/env python3
"""Read-only CUE/Mode2/ISO9660 metadata analyzer for locally supplied discs."""

from __future__ import annotations

import argparse
import json
import re
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Optional

SECTOR_SIZE = 2352
MODE2_USER_DATA_OFFSET = 24
USER_DATA_SIZE = 2048
FRAMES_PER_SECOND = 75


@dataclass
class Index:
    number: int
    minute: int
    second: int
    frame: int

    @property
    def lba(self) -> int:
        return (self.minute * 60 + self.second) * FRAMES_PER_SECOND + self.frame


@dataclass
class Track:
    number: int
    file_name: str
    file_path: str
    mode: str
    file_size: int
    sector_size: int
    sector_count: int
    trailing_bytes: int
    indexes: list[Index] = field(default_factory=list)


@dataclass
class IsoEntry:
    name: str
    extent_lba: int
    size: int
    directory_record_offset: int


@dataclass
class Analysis:
    cue_path: str
    tracks: list[Track]
    iso_present: bool = False
    iso_volume_id: Optional[str] = None
    iso_volume_sectors: Optional[int] = None
    iso_root_lba: Optional[int] = None
    iso_root_size: Optional[int] = None
    iso_entries: list[IsoEntry] = field(default_factory=list)
    psx_exe: Optional[dict] = None
    raw_mips_candidates: list[dict] = field(default_factory=list)
    native_recompilation: dict = field(default_factory=dict)
    warnings: list[str] = field(default_factory=list)


def _msf(value: str) -> tuple[int, int, int]:
    match = re.fullmatch(r"(\d{2}):(\d{2}):(\d{2})", value)
    if not match:
        raise ValueError(f"invalid MSF index: {value}")
    return tuple(int(part) for part in match.groups())


def parse_cue(cue_path: Path) -> list[Track]:
    tracks: list[Track] = []
    current_file: Optional[Path] = None
    current_track: Optional[Track] = None
    for raw_line in cue_path.read_text(encoding="ascii").splitlines():
        line = raw_line.strip()
        file_match = re.match(r'^FILE\s+"([^"]+)"\s+\S+', line, re.IGNORECASE)
        if file_match:
            current_file = cue_path.parent / file_match.group(1)
            continue
        track_match = re.match(r"TRACK\s+(\d+)\s+(\S+)", line, re.IGNORECASE)
        if track_match:
            if current_file is None:
                raise ValueError("TRACK appears before FILE")
            number = int(track_match.group(1))
            mode = track_match.group(2).upper()
            size = current_file.stat().st_size
            current_track = Track(number, current_file.name, str(current_file), mode, size,
                                  SECTOR_SIZE, size // SECTOR_SIZE, size % SECTOR_SIZE)
            tracks.append(current_track)
            continue
        index_match = re.match(r"INDEX\s+(\d+)\s+(\d{2}:\d{2}:\d{2})", line, re.IGNORECASE)
        if index_match and current_track is not None:
            minute, second, frame = _msf(index_match.group(2))
            current_track.indexes.append(Index(int(index_match.group(1)), minute, second, frame))
    if not tracks:
        raise ValueError("CUE contains no tracks")
    return tracks


def _both_endian_u32(value: bytes) -> int:
    if len(value) != 8:
        raise ValueError("ISO both-endian field must be 8 bytes")
    little = int.from_bytes(value[:4], "little")
    big = int.from_bytes(value[4:], "big")
    if little != big:
        raise ValueError(f"inconsistent ISO both-endian value: {value.hex()}")
    return little


def _user_data(raw: bytes, lba: int) -> bytes:
    start = lba * SECTOR_SIZE + MODE2_USER_DATA_OFFSET
    end = start + USER_DATA_SIZE
    return raw[start:end]


def _parse_iso(raw: bytes, analysis: Analysis) -> None:
    pvd = _user_data(raw, 16)
    if len(pvd) < USER_DATA_SIZE or pvd[:7] != b"\x01CD001\x01":
        return
    analysis.iso_present = True
    analysis.iso_volume_id = pvd[40:72].decode("ascii", "replace").rstrip(" ")
    analysis.iso_volume_sectors = int.from_bytes(pvd[80:84], "little")
    analysis.iso_root_lba = _both_endian_u32(pvd[156 + 2:156 + 10])
    analysis.iso_root_size = _both_endian_u32(pvd[156 + 10:156 + 18])
    root = _user_data(raw, analysis.iso_root_lba)
    position = 0
    while position < len(root) and root[position]:
        length = root[position]
        record = root[position:position + length]
        if len(record) < 34:
            break
        name_length = record[32]
        name = record[33:33 + name_length].decode("ascii", "replace")
        if name not in ("\x00", "\x01"):
            analysis.iso_entries.append(IsoEntry(
                name,
                _both_endian_u32(record[2:10]),
                _both_endian_u32(record[10:18]),
                analysis.iso_root_lba * SECTOR_SIZE + MODE2_USER_DATA_OFFSET + position,
            ))
        position += length
    for entry in analysis.iso_entries:
        if entry.extent_lba * SECTOR_SIZE + entry.size > len(raw):
            analysis.warnings.append(
                f"ISO entry {entry.name} exceeds available Track 1 data "
                f"(LBA {entry.extent_lba}, {entry.size} bytes)"
            )


def _find_psx_exe(raw: bytes) -> Optional[dict]:
    position = raw.find(b"PS-X EXE")
    if position < 0:
        return None
    header = raw[position:position + 2048]
    if len(header) < 0x30:
        return None
    return {
        "byte_offset": position,
        "sector": position // SECTOR_SIZE,
        "sector_offset": position % SECTOR_SIZE,
        "load_address": int.from_bytes(header[0x18:0x1C], "little"),
        "global_pointer": int.from_bytes(header[0x14:0x18], "little"),
        "entry_point": int.from_bytes(header[0x10:0x14], "little"),
        "text_size": int.from_bytes(header[0x1C:0x20], "little"),
        "bss_address": int.from_bytes(header[0x30:0x34], "little"),
        "bss_size": int.from_bytes(header[0x34:0x38], "little"),
        "stack_address": int.from_bytes(header[0x38:0x3C], "little"),
        "stack_size": int.from_bytes(header[0x3C:0x40], "little"),
    }


def _find_raw_mips_candidates(raw: bytes) -> list[dict]:
    candidates = []
    for sector in range(24, len(raw) // SECTOR_SIZE):
        payload = _user_data(raw, sector)
        if len(payload) < 16:
            continue
        words = [int.from_bytes(payload[offset:offset + 4], "little") for offset in range(0, 16, 4)]
        # Common little-endian MIPS prologue instructions: stack adjustment and saves.
        stack_adjust = any((word & 0xFFFF0000) == 0x27BD0000 for word in words)
        stack_save = any((word & 0xFFFF0000) in (0xAFBF0000, 0xAFB00000, 0xAFB10000, 0x8FB10000)
                         for word in words)
        if stack_adjust or stack_save:
            candidates.append({"sector": sector, "byte_offset": sector * SECTOR_SIZE + MODE2_USER_DATA_OFFSET,
                               "reason": "MIPS-like prologue; no PS-X EXE header"})
            if len(candidates) == 8:
                break
    return candidates


def analyze(cue_path: Path) -> Analysis:
    tracks = parse_cue(cue_path)
    analysis = Analysis(str(cue_path), tracks)
    data_tracks = [track for track in tracks if not track.mode.endswith("AUDIO")]
    if data_tracks:
        track = data_tracks[0]
        raw = Path(track.file_path).read_bytes()
        _parse_iso(raw, analysis)
        analysis.psx_exe = _find_psx_exe(raw)
        if analysis.psx_exe is not None:
            analysis.native_recompilation = {
                "possible_from_metadata": False,
                "code_region": {
                    "file_offset": analysis.psx_exe["byte_offset"] + 2048,
                    "load_address": analysis.psx_exe["load_address"],
                    "size": analysis.psx_exe["text_size"],
                },
                "data_region": "embedded in executable/disc files; no extraction performed",
                "bss_region": {
                    "address": analysis.psx_exe["bss_address"],
                    "size": analysis.psx_exe["bss_size"],
                },
                "blockers": [
                    "MIPS instructions require source-level/native recompilation or a CPU implementation",
                    "BIOS, GPU, CD-ROM, controller, and memory-mapped service dependencies are unresolved",
                ],
            }
        if analysis.psx_exe is None:
            analysis.raw_mips_candidates = _find_raw_mips_candidates(raw)
            analysis.warnings.append("No PS-X EXE header detected; load address and entry point are unknown")
    return analysis


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("cue", type=Path, help="local CUE file")
    parser.add_argument("--json", action="store_true", help="emit machine-readable JSON")
    args = parser.parse_args()
    result = asdict(analyze(args.cue))
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
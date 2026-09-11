# Phase 4: local disc analysis

The original files remain in `local_game_data/`, are read-only inputs, and are ignored by Git. This document records safe technical metadata only and contains no disc bytes or extracted game assets.

## Verified local files

The following local-only files were inspected without modifying or copying them:

- `Dino Crisis (USA).cue` — 204 bytes
- `Dino Crisis (USA) (Track 1).bin` — 379,984,416 bytes
- `Dino Crisis (USA) (Track 2).bin` — 37,396,800 bytes

## CUE structure

```text
FILE "Dino Crisis (USA) (Track 1).bin" BINARY
  TRACK 01 MODE2/2352
    INDEX 01 00:00:00
FILE "Dino Crisis (USA) (Track 2).bin" BINARY
  TRACK 02 AUDIO
    INDEX 00 00:00:00
    INDEX 01 00:02:00
```

The verified track geometry is:

| Track | Mode | Size | Complete 2352-byte sectors | Index information |
| --- | --- | ---: | ---: | --- |
| 1 | MODE2/2352 | 379,984,416 bytes | 161,558 | INDEX 01 at file LBA 0 |
| 2 | AUDIO | 37,396,800 bytes | 15,900 | pregap 150 frames; INDEX 01 at frame 150 |

Track 1 uses 2352-byte sectors, with a 24-byte header/subheader and 2048-byte user-data payload. The audio track is separate and not used for the executable image.

## ISO and executable findings

The analyzer found a valid ISO9660 Primary Volume Descriptor and a valid `PS-X EXE` header in the local Track 1 data:

- Volume identifier: `SLUS_00922`
- ISO root directory LBA: 22
- ISO root directory size: 2048 bytes
- ISO directory entries include:
  - `PSX`
  - `SLUS_009.22;1` — extent LBA 161099, size 630,784 bytes
  - `SYSTEM.CNF;1` — extent LBA 161407, size 68 bytes
  - `ZNULL.DAT;1` — extent LBA 161708, size 32,256,000 bytes
- `PS-X EXE` header found at file byte offset 378,904,872
- Header sector: 161099
- Header offset within sector: 24 bytes
- Load address: `0x80018000`
- Entry point: `0x8001a1c8`
- Text size: `628736` bytes (`0x99000`)

This is the strongest proof available from the local user-owned dump: the disc contains a standard PS1 executable image, and the header declares the executable load and entry addresses. The actual code bytes beyond the header were not re-created or extracted here; the analyzer only inspected the local disc metadata.

## Safety notes

- The local legal copy remains outside the Git repository in `local_game_data/` and is ignored by Git.
- The analyzer is read-only; it does not write, copy, or transform the original disc files.
- The report above contains only metadata and header values, not the data payload from the disc.
- `ZNULL.DAT;1` exceeds the available Track 1 extent in this local copy, so the full file cannot be validated from the current dump alone.

## Verification evidence

The following checks were run successfully:

- `python3 tools/disc_analyzer.py "local_game_data/Dino Crisis (USA).cue" --json`
- `cd tools && python3 -m unittest -q test_disc_analyzer.py`

The test suite passed with `Ran 2 tests ... OK`.

The analysis is therefore limited to verified metadata and executable header fields. It does not establish or implement a PS1 emulator or game logic; it is strictly a legal, read-only disc-inspection step.
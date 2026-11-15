# fork-build Specification

## Purpose
TBD - created by archiving change add-slot-carousel. Update Purpose after archive.
## Requirements
### Requirement: Reproducible containerized build
The project SHALL build the SuperChis firmware with `./build.sh`, which runs `make BOARD=chis`
inside the image defined by `docker/Dockerfile`. The image MUST mirror upstream SuperFW CI
(Ubuntu, apt `gcc-arm-none-eabi`, host gcc/g++, python3 with `python` alias). The upstream
`Makefile` MUST NOT require changes to build on macOS.

#### Scenario: Clean build succeeds (agent)
- **WHEN** `./build.sh clean && ./build.sh` runs on a host with Docker running
- **THEN** it exits 0 and produces `superfw.gba` at the repo root

#### Scenario: Upstream unit tests still pass (agent)
- **WHEN** upstream host tests are run inside the build image (`cd tests && make`)
- **THEN** they exit 0

### Requirement: Visible fork identity
The firmware SHALL identify itself as the slot. superchis fork so the user can tell it apart
from stock flashed SuperFW. The Info screen version string MUST include a fork suffix and
milestone (e.g. `slot.M1`), and boot MUST show a brief splash (≤ 2 s) naming the fork.

#### Scenario: Version string shows fork (hardware)
- **WHEN** the user opens Select → Info in the SD-booted firmware
- **THEN** the version line includes the fork suffix and the upstream commit slug

#### Scenario: Splash on boot (agent + hardware)
- **WHEN** the firmware starts (mGBA preview or hardware)
- **THEN** a splash naming "slot. superchis" appears before the home screen

### Requirement: SD chain-load delivery, no flashing
The deliverable SHALL be a `.gba` file the user copies to the SD card and launches from stock
flashed SuperFW. The project MUST NOT include any automated or default flashing step. Removing
the file MUST fully revert the user to stock.

#### Scenario: Chain-load from stock (hardware)
- **WHEN** the user launches the fork's `.gba` from stock SuperFW's browser
- **THEN** the fork boots to its home screen

### Requirement: README install, removal and test instructions
`README.md` SHALL document: building, copying to SD, the `Games/GBA|GB|GBC` layout, removal,
the per-milestone hardware checklist, known limitations (in-game exit returns to stock), and
a "Flashing (untested — help wanted)" section stating the author has not flashed it and
pointing to SuperFW's DS-based recovery tool.

#### Scenario: README current (agent)
- **WHEN** a milestone's tasks are complete
- **THEN** README.md install/test sections reflect that milestone's behaviour

### Requirement: License and attribution
The repository SHALL be distributed under GPL-3.0-or-later with a `NOTICE.md` crediting
SuperFW (davidgf), slot. (Brandon Kowalski and contributors, artwork by Markus), slot. advance /
DS Style (FrankieT19), and retaining Apache-2.0 notices for any EZ-FLASH/DS Style-derived code.

#### Scenario: Notices present (agent)
- **WHEN** any file is adapted from `vendor/slot-advance`
- **THEN** its header names the source file and licence, and NOTICE.md lists it


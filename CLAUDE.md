# CLAUDE.md

## What this is
slot. superchis: a GPL-3.0 continuation fork of SuperFW (davidgfnet/superfw) that replaces
SuperFW's file browser with the slot. cartridge carousel, ported from FrankieT19/slot-advance.
Target hardware: SuperChis Prime GBA flashcart only (`BOARD=chis`). English only.

## Hard rules
- Never flash anything. Deliverable is `superfw.gba` chain-loaded from SD by the user's stock
  flashed SuperFW. Flashing is documented as untested/help-wanted in README only.
- Never edit `vendor/slot-advance/` (pinned submodule). Copy/adapt into `src/slot/` with attribution.
- Keep upstream SuperFW files' diffs minimal and surgical; new code goes in `src/slot/`.
  `git diff upstream/master -- src/ Makefile` must stay reviewable.
- EWRAM is ~91% used by stock SuperFW (~22 KB free). Large buffers go in SDRAM, never EWRAM.
- SuperCard SDRAM (hardware-verified 2026-11-15, SLOT_DIAG run): no byte stores (`strb` corrupts the
  neighbour; ADR-0010) and no DMA with an SDRAM source (wrong data / hang). Bulk copies use
  `Slot_Copy` (IWRAM ARM ldm/stm). mGBA cannot catch either; use `make SLOT_DIAG=1` on hardware.
- The art/sound pack is built from the submodule (`tools/slot_pack.py`), compressed and unpacked at boot into SDRAM `0x08E00000` (ADR-0011). Sound DMA must read IWRAM only (`slot_audio.h`).
- Keep README.md install/test steps current with every user-visible change.
- Respect ADRs in `adr/` (immutable once accepted; supersede, don't edit).

## Build
- Canonical: `./build.sh` (runs `make BOARD=chis COMPRESSION_RATIO=10` in docker/Dockerfile,
  mirroring upstream CI). Docker Desktop must be running. Output: `superfw.gba`.
- Clean: `./build.sh clean`.
- Host unit tests (upstream): `cd tests && make` (needs lcov/OpenSSL, not in the image).
- Slot unit tests (scan/sort/settings): `./build.sh slot-tests`. Emulator preview ROM: `./build.sh preview`
  then `tools/preview_shots.sh slot-preview.gba <sequence>` (PNGs in `build/shots/`).

## Test card
- Mirror: `/Volumes/16TB_EXT/emulation/flashcarts/SUPERCHIS/` (edit here first).
- Card: `/Volumes/SUPERCHIS`. Sync order (card holds hardware-generated state):
  1. card → mirror: `rsync -rt --modify-window=1 --exclude '._*' /Volumes/SUPERCHIS/{SAVEGAME,.superfw,SYSTEM} <mirror>/`
     (SAVEGAME = saves, .superfw = SuperFW state, SYSTEM = Cart Studio art written on the card).
  2. mirror → card: `rsync -rt --modify-window=1 --delete --exclude '.metadata_never_index' --exclude '._*' <mirror>/ /Volumes/SUPERCHIS/`
  3. `dot_clean -m /Volumes/SUPERCHIS`, then `diskutil eject /Volumes/SUPERCHIS`.
  Never `--delete` mirror → card without step 1. Only the user verifies on hardware.
- Library layout: `Games/GBA`, `Games/GB`, `Games/GBC`; art in `/SYSTEM/SLOT` (Cart Studio).

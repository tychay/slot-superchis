## Why

The SuperChis Prime is a capable GBA flashcart, but its firmware (SuperFW) presents games as a
file browser. slot. (and its GBA port, slot. advance) shows a library as a shelf of physical
cartridges, but only runs on the RG SP and EZ-FLASH Omega. Both SuperFW and slot. advance are
GPL and open source, and SuperFW already chain-loads from SD. That makes a no-flash, low-risk
port possible: a SuperFW fork whose home screen is the slot. carousel.

## What Changes

This change delivers milestones **M0 → M1 → M2**, with a hardware checkpoint after each.

- **M0 — Fork pipe**: the fork builds reproducibly in Docker (`./build.sh`, mirroring upstream CI),
  carries a visible fork identity (version string + brief splash), and boots from SD under stock
  flashed SuperFW. README documents install-by-copy and removal-by-delete.
- **M1 — MVP carousel**: SuperFW's home screen becomes a slot.-style carousel with three shelves
  (GBA, GB, GBC) scanned from `Games/GBA|GB|GBC`, drawn with plain built-in cartridge
  silhouettes and the file name as label text. L/R switches shelf, Left/Right picks a cart,
  A plays the insert animation and hands the file to SuperFW's existing launch path
  (`browser_open()` → GBA load popup or Goombacolor). **Select opens the stock SuperFW UI**
  (escape hatch: settings, file browser, tools, info). A preview ROM with a stubbed library
  runs in mGBA for agent screenshot checks.
- **M2 — Art & feel**: per-game label art and shell colours read from `/SYSTEM/SLOT` in the
  format slot. advance / Cart Studio writes; Wood, Dark and Light themes; insert sound.
- The slot. advance renderer, navigation physics and artwork are adapted (not rewritten) from
  the pinned `vendor/slot-advance` submodule through a backend shim in `src/slot/`.
- Large slot. assets ship as an SDRAM asset pack unpacked by the existing `rom_boot.S` loader,
  because EWRAM is ~91% used.

**Deferred to `TODO.md`** (not in this change): return-to-carousel / eject animation after
in-game exit (on SD-boot, exit resets into the flashed stock SuperFW), slot.-style settings
screen, footer clock, `USERNAME.TXT`, hotkey remapping, eject sound, NOR-flashed games shelf,
other emulator shelves (NES/SMS/etc.), translations, other SuperCard boards, flashed `.fw`
install (documented only as untested, help-wanted).

## Capabilities

### New Capabilities
- `fork-build`: reproducible Docker build of the chis firmware, visible fork identity, SD
  chain-load delivery and README install/removal/test instructions.
- `cartridge-carousel`: carousel home screen — library scan, shelves, navigation, launch
  hand-off to SuperFW, Select escape hatch to the stock UI.
- `carousel-preview`: preview ROM build with a stubbed library for headless mGBA screenshots.
- `cart-art`: Cart Studio / slot. advance label and shell art from `/SYSTEM/SLOT`, themes,
  insert sound.

### Modified Capabilities
- (none — no existing specs)

## Impact

- **Upstream SuperFW files touched (minimal hooks)**: `src/menu.c` (home-screen entry and
  Select hand-off), `Makefile` (new sources, asset pack, preview target), `rom_boot.S` (unpack
  the slot asset pack), `src/common.h` (SDRAM map constant), `src/main.c` if needed for entry.
- **New code**: `src/slot/` (shim + adapted frontend), `tools/` asset-pack generator,
  `docker/`, `build.sh`, `README.md`, `TODO.md`, `NOTICE.md`.
- **Dependencies**: `vendor/slot-advance` submodule (pinned, read-only), Docker, mGBA (host,
  for preview screenshots).
- **Licensing**: combined work distributed as GPL-3.0-or-later with upstream notices retained
  (SuperFW GPL-3.0-or-later per source headers; slot. advance GPL-3.0-or-later; DS Style /
  EZ-FLASH parts Apache-2.0).
- **User data**: none modified; saves stay in SuperFW's `SAVEGAME/`.

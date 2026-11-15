# slot. superchis

A continuation fork of [SuperFW](https://github.com/davidgfnet/superfw) (GBA flashcart firmware)
for the **SuperChis Prime**, heading toward a [slot.](https://github.com/slot-cfw/slot)-style
cartridge carousel (ported from [slot. advance](https://github.com/FrankieT19/slot-advance)) as
its home screen. Licensed GPL-3.0-or-later, see `LICENSE` and `NOTICE.md`.

## What it is (current status: M0)

**M0 is the fork pipe only.** It behaves exactly like SuperFW 0.22 with three differences:

- a brief (about 1.5 s) "slot. superchis" splash at boot,
- a `slot.M0` line on the Info tab under the version,
- it is built from this repository with a reproducible Docker build.

There is no carousel yet; the home screen is still SuperFW's file browser. The carousel
(three shelves read from `Games/GBA`, `Games/GB`, `Games/GBC`) arrives in M1. See `TODO.md` for
features deliberately deferred.

Target hardware: SuperChis Prime only (`BOARD=chis`). English only.

## Install

This is delivered as a `.gba` file that you run from the SD card. Nothing is flashed.

1. Your SuperChis must already run stock SuperFW from its flash (that is how you launch the fork).
2. Copy `slot-superchis-M0.gba` (or your own `superfw.gba` build, see Build) to the SD card.
   Any folder works at M0.
3. Put the card in the cart, boot into stock SuperFW, and launch the file from its file browser
   like any other GBA game.
4. Keep your games where you already have them. For M1 and later, the carousel will read
   `Games/GBA`, `Games/GB` and `Games/GBC` at the card root; creating those folders now is
   harmless but has no effect at M0.

Saves and settings stay in SuperFW's usual `SAVEGAME/` and `.superfw/` folders.

## Remove

Delete the `.gba` file from the card. Nothing else changed: the cart's flash still holds stock
SuperFW, and the shared `SAVEGAME/` and `.superfw/` folders are the same ones stock SuperFW uses.

## Build

Requires Docker (Desktop or Engine) running. The build runs inside the image in `docker/`, which
mirrors upstream SuperFW's CI.

```sh
./build.sh          # builds superfw.gba (make BOARD=chis COMPRESSION_RATIO=10)
./build.sh clean    # clean build outputs
```

The build fails if `superfw.gba` grows beyond 2,048 KB (override with
`./build.sh BOARD=chis COMPRESSION_RATIO=10 SLOT_MAX_IMAGE_KB=<kb>`). The fork milestone shown on
the Info tab is `SLOT_MILESTONE` at the top of the `Makefile`.

## Development builds between M0 and M1

Builds made from `main` during M1 (payload infrastructure, not yet a release) differ from M0:

- after the splash, a solid **orange stub screen** is shown (the empty carousel payload, `slot.payload`);
  press any key to reach the stock SuperFW UI. On the Settings/UI/Tools/Info tabs, Select returns to the stub.
- the stock file browser lists at most 12,288 entries per folder (was 16,384): SDRAM was needed for the
  payload window (see `openspec/changes/add-slot-carousel/design.md`, "SDRAM map (audited)").

## Hardware test checklist (M0)

Boot the file from stock SuperFW, then confirm:

- [ ] The "slot. superchis" splash appears briefly before the normal SuperFW screen.
- [ ] Info tab (Select, then Info) shows `slot.M0` below the variant line.
- [ ] A GBA game launches from the browser and plays.
- [ ] A GB or GBC game launches and plays.
- [ ] Exiting a game (in-game menu quit or reset) lands back in stock SuperFW, whose Info tab shows
      no `slot.` suffix.

## Known limitations

- No carousel at M0 (the stock file browser is the home screen).
- When you exit a game, the cart resets into the flashed stock SuperFW, not back into this fork.
- Only the SuperChis Prime board is targeted; other SuperCard variants are untested.

## Flashing (untested, help wanted)

The author has **never flashed this firmware** and it is **untested**. It is designed to be run
from SD, not flashed, and no flashing step is provided or recommended. If you flash it anyway and
it goes wrong, recovery needs a Nintendo DS or DS Lite plus a Slot-1 cart, using
https://github.com/davidgfnet/superfw-nds-flasher-tool. Reports from anyone who has done this
safely are welcome.

## Credits

SuperFW by David Guillen Fandos; slot. by Brandon Kowalski and contributors (artwork by Markus);
slot. advance / DS Style by FrankieT19; EZ-FLASH / SimpleLight authors. Full details in
`NOTICE.md`. The original SuperFW README is kept at `docs/SUPERFW-README.md`.

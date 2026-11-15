# slot. superchis

A continuation fork of [SuperFW](https://github.com/davidgfnet/superfw) (GBA flashcart firmware)
for the **SuperChis Prime** whose home screen is a [slot.](https://github.com/slot-cfw/slot)-style
cartridge carousel (ported from [slot. advance](https://github.com/FrankieT19/slot-advance)).
Licensed GPL-3.0-or-later, see `LICENSE` and `NOTICE.md`.

## What it is (current status: M2)

At **M2** the fork boots to a cartridge carousel instead of SuperFW's file browser:

- a short "slot." splash, then three shelves of cartridges read from `Games/GBA`,
  `Games/GB` and `Games/GBC` on the SD card, with the selected cart's name shown as text;
- **label art and shell colours** from slot. Cart Studio (see "Cart Studio art" below); carts
  without art stay plain silhouettes with the name on the label;
- **Wood, Dark and Light themes** (hold B, press Start to cycle);
- slot.'s **insert sound** when you press A;
- slot.'s spring motion and the insert animation;
- pressing A hands the game to SuperFW's own launcher: GBA games open SuperFW's load popup
  (saves, patches, in-game menu all work as in stock SuperFW), GB/GBC games start the bundled
  Goombacolor;
- Select opens SuperFW's stock UI (Recent, Browse, Settings, Tools, Info), so nothing in SuperFW
  becomes unreachable;
- the last shelf and the selected cart of each shelf are remembered.

Not yet (see `TODO.md`): the slot.-style settings screen, return-to-carousel after a game exits,
other systems.

Target hardware: SuperChis Prime only (`BOARD=chis`). English only.

## Controls (M2)

| Where | Key | Does |
|---|---|---|
| Carousel | Left / Right | previous / next cart (hold to repeat: first after 400 ms, then every 200 ms; wraps around) |
| Carousel | L / R | previous / next shelf (GBA, GB, GBC, wrapping) |
| Carousel | A | play the insert animation, then open the game in SuperFW's launcher |
| Carousel | hold B, press Start | next theme: Wood, Dark, Light (saved in `SETTINGS.TXT`) |
| Carousel | Select | open SuperFW's stock UI |
| SuperFW load popup | B | cancel, back to the carousel with the same cart selected |
| Stock Settings / UI / Tools / Info tab | Select | back to the carousel |

(Select on the stock Recent, Browse and NOR tabs keeps its normal SuperFW meaning, so first move to
one of the four tabs above with the L/R shoulder buttons.)

The footer shows `Select: SuperFW menu` on the left and `<shelf> n/total` on the right. If a folder
holds more than 2,048 matching games, the first 2,048 found are shown and the footer says
`First 2048 games shown`; files whose full path would not fit SuperFW's 255-character path limit
are skipped and counted in the footer.

## Install

This is delivered as a `.gba` file that you run from the SD card. Nothing is flashed.

1. Your SuperChis must already run stock SuperFW from its flash (that is how you launch the fork).
2. Copy `slot-superchis-M2.gba` (or your own `superfw.gba` build, see Build) to the SD card.
   Any folder works.
3. Put your games, **unzipped**, in `Games/GBA` (`.gba`), `Games/GB` (`.gb`) and `Games/GBC`
   (`.gbc`) at the card root. Folders are not searched recursively; hidden files and macOS `._*`
   files are ignored.
4. Put the card in the cart, boot into stock SuperFW, and launch the file from its file browser
   like any other GBA game. The fork's carousel appears.
5. To make returning to the fork quick after a game, leave stock SuperFW's **UI tab → Recent
   ROMs** set to Enabled (the default): stock SuperFW then opens on its Recent tab, and the fork's
   `.gba` is the first entry there because games launched from the carousel are *not* added to
   SuperFW's recent list. Power on, press A, and you are back in the carousel.

6. Optional, for label art: follow "Cart Studio art" below.

Saves and SuperFW's settings stay in the usual `SAVEGAME/` and `.superfw/` folders. The carousel's
own state is `SYSTEM/SLOT/SETTINGS.TXT` (slot. advance's format, so the same card works with
slot. Cart Studio later).

## Cart Studio art

The carousel shows the cart images that [slot. Cart Studio](https://studio.slot-cfw.fyi) makes, read
from `SYSTEM/SLOT/CARTS/<GBA|GB|GBC>/<ROM name>.bmp` on the card (`CARTS_FLAT/` for the flat
look). The file name is the ROM's file name without its extension, exactly. A cart with no BMP
shows the plain silhouette with its name.

Cart Studio does not recognise a SuperChis card (it looks for EZ-Flash kernel files), so it offers
a **"Download"** `labels.zip` instead of writing to the card. The steps:

1. Open studio.slot-cfw.fyi in Chrome or Edge and choose the card (or any folder with
   `Games/GBA`, `Games/GB`, `Games/GBC` in it) so it can match your ROMs. Make your labels.
2. Choose **Download for slot. advance** (not "Write to card") and save `labels.zip`.
3. Convert it into the card's `SYSTEM/SLOT` folder. On macOS or Linux, with Docker running:

   ```sh
   tools/convert_labels.sh labels.zip /Volumes/SUPERCHIS      # the card root, the folder that holds Games/
   ```

   This runs the vendored slot. advance converter in the build container, reads the ROM headers in
   `Games/` to pick each cart's shell colour and outline, and writes `SYSTEM/SLOT/CARTS`,
   `CARTS_FLAT` and `LABELS` onto the card. On Windows use slot. advance's `Convert Labels.bat`
   / Label Converter.exe (in `vendor/slot-advance/`) pointed at `labels.zip` and the card.
4. Put the card in the cart and boot the fork. Art streams in while you scroll (a cart may show
   its plain silhouette for a few frames first). To use the flat look, set `Skeuomorphic=0` in
   `SYSTEM/SLOT/SETTINGS.TXT`.

## Remove

Delete the `.gba` file from the card. Nothing else changed: the cart's flash still holds stock
SuperFW, and the shared `SAVEGAME/` and `.superfw/` folders are the same ones stock SuperFW uses.
`SYSTEM/SLOT/` may be deleted as well.

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

The carousel is a separate program, `slot.payload` (built from `src/slot/`), copied to SDRAM at
boot and run from there; the firmware itself changes only in a few `// slot:` marked places.
Its built-in art is extracted at build time from the pinned `vendor/slot-advance` submodule
(`git submodule update --init` first on a fresh clone).

## Developer tools (not part of the install)

These are for working on the carousel without a cart. They are **not** firmware and must not be
copied to the card.

**Preview ROM.** `./build.sh preview` builds `slot-preview.gba`: the same carousel code linked
against a stub host (`src/slot/preview_host.c`) with a fictional in-memory library instead of an SD
card, so it runs in a stock emulator (mGBA). Select shows a stub "stock UI" screen (Select returns), and
A shows a stub "load popup" with the launch path (B returns). The library mode is chosen by the
keys held when the ROM starts: nothing = about 8 fictional titles per shelf, **Up** = a 3,000-title
stress shelf (some 240-character names, some UTF-8, some too long to launch), **Down** = every
shelf empty, **Left** = no `Games` folder.

**Screenshot harness.** `tools/preview_shots.sh <rom.gba> <sequence> [outdir]` runs a ROM headless
in libmgba inside the build container (no window, no GUI or screen-capture permissions) with a
scripted key sequence and writes PNGs to `build/shots/` (git-ignored). Example, boot, Right twice, switch shelf:

```sh
./build.sh preview
tools/preview_shots.sh slot-preview.gba wait:150,shot:boot,Right,Right,wait:40,shot:right2,R,wait:40,shot:gb
tools/preview_shots.sh slot-preview.gba hold:Up:20,wait:300,shot:stress,Left,wait:50,shot:wrap   # stress shelf
```

Run the script without arguments for the step grammar (`wait:N`, `KEY`, `hold:KEY[:N]`, `shot[:NAME]`).

**Host unit tests.** `./build.sh slot-tests` compiles and runs, in the build container with
AddressSanitizer, the tests for the pure carousel modules: `tests/slot_scan_test.c` (filter, sort,
the 5,000-entry / 255-byte-name large-library cases) and `tests/slot_settings_test.c`
(`SETTINGS.TXT` round trip and validation), and `tests/slot_bmp_test.c` (Cart Studio art: the
real converter's output, generated in the container, plus converted real labels in
`tests/fixtures/art`, must validate and decode to the expected sizes). (Upstream's own `tests/Makefile` is unchanged; it needs
`lcov` and OpenSSL, which the build image does not install.)

## Hardware test checklist (M2)

Install `slot-superchis-M2.gba` and run Cart Studio as described above, then confirm:

- [ ] The "slot." splash, "Reading shelf...", then the carousel. Carts that have art show their
      labels and shell colours; carts without art show the plain silhouette with the name.
- [ ] Hold Right across a shelf where every cart has art: scrolling continues without long
      freezes (art may appear a frame or two late; note any stutter).
- [ ] Hold B and press Start: the theme cycles Wood, Dark, Light and the choice survives a power
      cycle. The footer text is readable in all three.
- [ ] Press A on a GBA game: the insert sound plays with the insert animation, the game starts,
      and the **game's own audio is clean** afterwards (no hiss, crackle or silence). Repeat
      with a GB game and Advance Wars. Mute check: set `Sounds=0` in `SETTINGS.TXT` and A is silent.
- [ ] Everything in the M1 checklist below still works (launch, saves, B in the popup, Select
      round trip, Recent, stock browser).

### M1 checklist (regression)

Boot the file from stock SuperFW, then confirm:

- [ ] The "slot." splash appears, then a "Reading shelf..." screen (stays visible if the read hangs), then the carousel with shelves and sort order as expected
      (alphabetical, case-insensitive, per `Games/GBA`, `GB`, `GBC`).
- [ ] Navigation is smooth (Left/Right, hold to repeat, L/R shelves); note any stutter.
- [ ] Advance Wars launches from the carousel; save in-game, exit, relaunch: the save is intact.
- [ ] Tetris (GB) and Link's Awakening DX (GBC) launch.
- [ ] B in the load popup returns to the carousel with the same cart selected.
- [ ] Select shows stock Settings/Info and they work; Select (on Settings/UI/Tools/Info) returns to
      the carousel.
- [ ] The position (shelf and cart) is restored after a power cycle.
- [ ] After an in-game exit, the stock Recent tab shows the fork `.gba` first (task 4.9: carousel
      launches are not added to the recent list).
- [ ] The stock file browser still lists files correctly (SDRAM sanity check).

## Known limitations

- When you exit a game, the cart resets into the flashed stock SuperFW, not back into this fork
  (see step 5 of Install for the quick way back).
- The stock file browser lists at most 12,288 entries per folder (was 16,384): SDRAM was needed for
  the carousel's window (see `openspec/changes/add-slot-carousel/design.md`, "SDRAM map (audited)").
- No settings screen: themes cycle with B+Start, everything else is `SYSTEM/SLOT/SETTINGS.TXT`.
- Art must be the converter's exact format (16-bit RGB555 BMP, 120x71 GBA / 104x112 GB and GBC);
  other BMPs are ignored and the plain cart is shown.
- The insert sound is a one-shot played from a small buffer refilled each frame: if the carousel
  ever misses two VBlanks in a row while it plays (not seen in testing) it may crackle.
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

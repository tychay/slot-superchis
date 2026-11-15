## Context

SuperFW (C, GPL-3.0-or-later) is the SuperChis firmware. `rom_boot.S` unpacks
`firmware.ewram.gba` into EWRAM and unpacks asset blobs (fonts, emulators, patch DB) into SDRAM.
Then `main_gba()` runs a simple loop: `menu_keypress()` → `menu_render()` → `wait_for_vblank()` →
`menu_flip()`. The stock UI is Mode 4 (8bpp, page-flipped, BG2 + window 0).

SuperFW already builds separately linked binaries (`ingamemenu.payload`, `directsave.payload`,
`superfw.dldi.payload`) and embeds them as blobs (`src/binassets.S`, `DEF_ASSET`).

Measured stock budget (`BOARD=chis`, Docker build):
- EWRAM 234,848 / 257,024 B (91.4 %, ~22 KB free).
- IWRAM 11,480 / 32,768 B (35 %).
- Image 1,671,680 B; chis flash cap 2,048 KB (`FW_MAX_SIZE_KB`).

SDRAM map while the menu runs (`src/common.h`, `src/menu.c`, `rom_boot.S`):
- `t_sdram_state` from `0x08000000` (≤ 14.5 MB).
- Fonts at `0x08F00000`.
- ROM-load hiscratch at `0x09000000` (up to 8 MB, only during load).
- Emulator/menu assets at `0x09E00000`.

slot. advance (`vendor/slot-advance/common/`) is the source of the renderer, navigation physics,
art loader, audio and assets.
- It is written as headers included into the DS Style kernel.
- It depends on about 40 kernel symbols: FatFs `f_*`, `gfile`, `pReadCache`,
  `pFilename_buffer`, `UIAudio_*`, `Launcher_*`, `gl_*` settings, `SD_list_MENU`.
- It uses libgba-style names (`VBlankIntrWait`, `dmaCopy`, `REG_*`, `KEY_*`).
- It renders in Mode 3 (16bpp) from a 76.8 KB back buffer.
- `slot_assets.h` holds about 360 KB of const art arrays.
- `Slot_Play()` launches by delegating to the kernel's own loader, the same pattern we need.

## Goals / Non-Goals

**Goals:**
- The carousel is the home screen; stock SuperFW launch, save, patch and IGM paths are reused unchanged.
- The diff against upstream SuperFW stays small and reviewable; new code lives in `src/slot/`.
- Stock EWRAM/IWRAM headroom is preserved and the image stays under the 2 MB cap.
- The agent can iterate on the UI without hardware (preview ROM + mGBA).

**Non-Goals:**
- Return-to-carousel after in-game exit, settings screen, clock, username, hotkeys, NOR shelf,
  other systems, translations, other boards, flashing (see proposal / TODO.md).
- Rewriting slot. advance's renderer, or tracking slot. advance upstream automatically.

## Decisions

### D1. The carousel is a separately linked payload executed from SDRAM
`src/slot/` builds into `slot.payload`, linked at a fixed SDRAM address. `rom_boot.S` copies it to
that address with the other assets. The firmware jumps to its entry point with an ABI table.

- **Why:** the frontend code (~2.5k dense lines plus the renderer) cannot fit in ~22 KB of EWRAM.
  SDRAM is the cart's ROM bus, where games execute anyway. SuperFW already has the payload
  pattern (in-game menu).
- **Hot paths:** the ARM row blitter (`slot_blit.s`) and the decoder go to an IWRAM section
  (~20 KB free) or run from SDRAM with tuned WAITCNT. M1 measures and picks.
- **Alternatives rejected:**
  - Link into the EWRAM image: does not fit.
  - IWRAM-only: too small.
  - Separate `.gba` app that launches games: SuperFW has no boot-this-file hand-off, and the
    in-game exit would land in stock.

### D2. Narrow ABI between firmware and payload
The firmware passes a `const struct slot_host_api *` to `slot_main(api)`. It holds:
- FatFs file and directory operations (`f_open/f_read/f_close/f_opendir/f_readdir/f_write/f_mkdir/f_rename/f_stat`).
- `get_keypress`, `wait_for_vblank`, and `frame_count`.
- `open_file(path, size)`, which wraps the static `browser_open()`.
- `enter_stock_ui()`.
- An SDRAM scratch pointer and size.

The payload returns an action code: `SLOT_LAUNCH` (path set) or `SLOT_STOCK_UI`.

- **Why:** the payload never links against firmware symbols, so the two can be built and
  versioned independently. The same API can be stubbed for the preview ROM (D6).
- **Alternative rejected:** sharing the firmware's symbols via a linker map. That is brittle
  across upstream merges.

### D3. Main-loop mode switch, minimal upstream hooks
`main_gba()` gains a mode variable.
- **Carousel mode:** call the payload until it returns.
  - `SLOT_LAUNCH`: restore stock video state, call `open_file()`, which opens the stock GBA load
    popup or starts Goombacolor.
  - `SLOT_STOCK_UI`: restore stock video state and run the stock loop.
- **Stock mode:** returning to the carousel happens when either:
  - a popup that the carousel opened is dismissed (a flag set by `open_file`), or
  - the user presses Select on a top-level stock tab.

Upstream edits are limited to:
- `main.c`: dispatch.
- `menu.c`: a non-static `menu_open_file()` wrapper, a popup-closed hook, the Select-to-return
  hook, and a version suffix.
- `rom_boot.S` and `Makefile`: payload and art blobs.
- `common.h`: SDRAM map constants.

### D4. Video: Mode 3 owned by the payload; stock state saved and restored
The payload saves `REG_DISPCNT`, BG/window registers and palette RAM. It switches to Mode 3 and
renders from an SDRAM back buffer, DMA-ing dirty row bands to VRAM in VBlank, as slot. advance
does. Before it returns, it restores the stock registers and palette and clears VRAM, so the
stock Mode 4 UI redraws cleanly.

- **Risk:** SDRAM→VRAM DMA is slower than EWRAM→VRAM. See Risks.

### D5. Assets are a separate compressed SDRAM blob
A tool, `tools/slot_pack.py`, emits `res/slot_assets.pack` from the pinned
`vendor/slot-advance` sources. It takes the arrays in `common/slot_assets.h` plus the PCM.
`rom_boot.S` unpacks it (apultra or upkr, same as the other assets) into a reserved SDRAM
region. The payload reads art through pointers in a pack header.

- **SDRAM region:** SUPERSEDED by "SDRAM map (audited)" below (window is `0x08B80000-0x08E7FFFF`). Original text: proposed `0x09800000–0x09DFFFFF`, after the 8 MB hiscratch and before the
  `0x09E00000` assets. The implementer MUST verify it against every SDRAM user, and add
  `_Static_assert`s and a build-time size check.
- **Why:** keeps 360 KB of art out of EWRAM; reuses the existing loader and compressors.
- **Image cap:** a build check fails if `superfw.gba` exceeds 2,048 KB.

### SDRAM map (audited)

Audited in task 3.1 against `src/common.h`, `src/menu.c`, `src/loader.c`, `src/main.c`,
`src/misc.c`, `src/virtfs.c`, `src/supercard_io.S`, `rom_boot.S`. This **amends D5**: the
proposed window `0x09800000-0x09DFFFFF` is NOT usable, for two reasons.

1. **The whole `0x09xxxxxx` half is shadowed by the SuperCard SD interface** whenever it is mapped
   (`set_supercard_mode(MAPPED_SDRAM, true, true)`, the menu's normal state): `supercard_io.S`
   puts the SD write/read/command registers at `0x09000000`, `0x09100000`, `0x09200000`,
   `0x09800000` and the mode register at `0x09FFFFFE`. Every stock access to `0x09xxxxxx`
   (hiscratch, patch DB, assets) is wrapped in `set_supercard_mode(..., false)`. A payload
   executing from, or a FatFs read landing in, `0x09xxxxxx` would break the moment it called
   the SD driver. The `0x08xxxxxx` half is unaffected (the font pack at `0x08E80000` is read
   with the SD interface on).
2. `0x09D00000-0x09DFFFFF` holds the built-in patch DB and `0x09E00000+` the emulator assets
   (the design text had the fonts at `0x08F00000`; the code says `0x08E80000`).

| Region | Start | End (incl.) | Size | Used by | When |
|---|---|---|---|---|---|
| Boot image (SD boot) | `0x08000000` | `0x081FFFFF` max | <= 2 MiB (1.59 MiB now) | `rom_boot.S` source for all copy/unpack steps | boot only, until the firmware is unpacked to EWRAM |
| `sdr_state->scratch` | `0x08000000` | `0x081FFFFF` | 2 MiB | FW update (`menu.c` ~2204-2245), NOR flashing (`menu.c` ~2581, `loader.c` flash_gba_nor) | during those operations |
| `fileorder`, `filefilter` | `0x08200000`, `0x0820C000` | `0x0820BFFF`, `0x08217FFF` | 48 KiB each | browser sort/filter (`menu.c`) | menu running |
| `fentries` | `0x08218000` | `0x08B2FFFF` | 9,535,488 B (12288 x 776) | browser file list | menu running |
| `rentries`, `nordata` | `0x08B30000` | `0x08B3EE2B` | 52,000 + 8,972 B | recent list, NOR registry | menu running |
| (free margin below window) | `0x08B3EE2C` | `0x08B7FFFF` | 266,708 B | nothing | |
| **slot window** | **`0x08B80000`** | **`0x08E7FFFF`** | **3 MiB** | **slot.payload, mailbox, statics, scratch (layout below)** | **resident, also while the stock UI runs** |
| Fonts + cheats | `0x08E80000` | `0x08FFFFFF` | 1.5 MiB | font glyph DB (`rom_boot.S`, constantly read by the menu); cheats at `FONT_BASE + font_block_size()` (`menu.c` ~758) | boot + menu running |
| hiscratch | `0x09000000` | `0x097FFFFF` | 8 MiB | patch-engine ROM scratch (`menu.c` ~501-526, SD off); `rom_boot.S` temp for compressed fonts (not on chis) | during patch generation |
| unused | `0x09800000` | `0x09BFFFFF` | 4 MiB | nothing, but unreachable with SD on | |
| `ROM_OFF_USRPATCH_DB` | `0x09C00000` | `0x09CFFFFF` | 1 MiB | defined in `common.h`, never referenced | |
| built-in patch DB | `0x09D00000` | `0x09DFFFFF` | 1 MiB (103 KB used) | `rom_boot.S`, `menu.c` ~693 (SD off); user-loaded DB written by `load_patchdb_action` | boot, ROM open |
| assets (emulators) | `0x09E00000` | `0x09FFFFFD` | 2 MiB | `virtfs.c` at emulator launch | launch |
| SC mode register | `0x09FFFFFE` | | 2 B | `supercard_driver.c` | always |

Other SDRAM users, none of which touch the window while the menu runs:
- `load_gba_rom()` / `load_extemu_rom()` write the whole `0x08000000+` ROM area. That is the launch
  point of no return (it also destroys fonts and assets). If a load fails after it started
  writing, the payload may be corrupt: the firmware re-validates the payload header on every entry
  and falls back to the stock UI (`slot_host.c`). The header check is magic/ABI/range only, not a
  checksum; add one if hardware shows a failed-load case that survives the check.
- `sdram_test()` (Tools) overwrites and restores all 32 MiB; it only runs on user request.
- `flash.c` maps firmware flash over `0x08000000` while programming (SDRAM unmapped then).
- `rom_boot.S` bounces through IWRAM `0x03004000-0x030047FF`, which overlaps the payload IWRAM
  range, but only during boot, long before the payload is ever copied to IWRAM.

**Boot unpack safety (SD boot).** The fork's `superfw.gba` sits at `0x08000000` while `rom_boot`
runs and is at most `FW_MAX_SIZE_KB` = 2 MiB (build guard). The payload is copied to
`0x08B80000`, 9.5 MiB above the highest possible source byte; `common.h` has a
`_Static_assert(SLOT_OFF_WINDOW >= FW_MAX_SIZE_KB * 1024)`. The existing destinations
(fonts, hiscratch, patch DB, assets) are all >= `0x08E80000`.

**Chosen window: `0x08B80000-0x08E7FFFF` (3 MiB)**, carved out of `t_sdram_state` by lowering
`BROWSER_MAXFN_CNT` from 16384 to 12288 (`config.h`, one marked line). `sizeof(t_sdram_state)`
is then 11,791,916 B, asserted `<= SLOT_OFF_WINDOW` in `menu.c`. Cost: a single folder listing
shows at most 12,288 entries instead of 16,384 (stock browser only; the carousel will have its
own scan cap). Layout (numbers in `src/slot/slot_layout.h`, shared by C, cpp'd linker scripts
and `rom_boot.S`):

| Sub-region | Address | Size | Content |
|---|---|---|---|
| payload image | `0x08B80000` | 256 KiB max | header, text, rodata, IWRAM load image, .data load image |
| mailbox | `0x08BC0000` | 4 KiB | `struct slot_mailbox` (launch size + path) |
| statics | `0x08BC1000` | 64 KiB | payload `.data`/`.bss` |
| scratch | `0x08BD1000` | 2.69 MiB | back buffer (75 KiB), art cache, pack (M2) |

Alternative considered and not taken: keep `0x09800000-0x09BFFFFF` (4 MiB, truly free) and have the
host FatFs wrappers switch the SD interface off after every call. Rejected for M1: every pointer
the payload passes to FatFs (FIL, DIR, FILINFO, read buffers) would have to live outside
`0x09xxxxxx` and reads would need bounce buffers; payload statics also live in SDRAM
(ADR-0005), so it is an easy rule to break silently. Revisit if the 3 MiB window proves too small
(the M2 art pack, 360 KB uncompressed, fits easily).

**IWRAM.** Firmware `.data/.bss/.iwram` ends at `0x03002CD8` (11,480 B). The payload's hot code runs
from `0x03003000-0x03004FFF` (8 KiB max, 36 B used by the stub): above the firmware, inside the
stack headroom (the user stack grows down from `0x03007F00`, so >= 12 KiB of stack remains
while the payload runs). The payload copies its own `.iwram` there in `slot_entry()` on every
entry, so nothing needs to survive while the stock UI runs. `gba_ewram.ld` asserts
`__IWRAM_END__ <= 0x03003000`.

**ABI (D2 refinement).** `open_file()` and `enter_stock_ui()` are not host calls: the payload
returns `SLOT_LAUNCH` (path and size in the mailbox) or `SLOT_STOCK_UI`, and the firmware does
the work after restoring video (D3/D4). The API struct carries `abi_version` + `struct_size` +
`sizeof(FIL/DIR/FILINFO)`; both sides check them. Video state is restored by the firmware
(`menu_slot_restore_video()`: it re-runs `setup_video()` plus the video half of `menu_init()`),
not saved by the payload, because the BG scroll/affine/window/blend registers are write-only
on GBA and cannot be read back.

### D6. Preview ROM = same payload + stub host
`make preview` (via `./build.sh preview`) links `src/slot/` against `src/slot/preview_host.c`.
- The stub implements `slot_host_api` with an in-memory fictional library (three shelves).
- Art comes from the same pack, placed in ROM.
- There is no SD, SDRAM-mode or flash access, so `slot-preview.gba` runs in a stock mGBA.
- `tools/preview_shots.sh` drives mGBA with a scripted input sequence and saves PNGs to
  `build/shots/`.
- The first M1 task is a spike to pick the mGBA mechanism: the Lua scripting API (`--script`),
  or the SDL frontend with a movie/input file plus screenshot hotkey.

### D7. Adapted copies, not includes, of slot. advance code
Files from `vendor/slot-advance/common/` are copied into `src/slot/`.
- Each copy gets a header naming the source path, the pinned commit and the licence.
- Each is then edited to use the host API in place of DS Style globals.
- The submodule stays read-only reference plus the asset source for `slot_pack.py`.

- **Why:** the headers are too entangled with DS Style globals to include unmodified.
  Copying keeps provenance visible. A future re-sync is a manual diff against the pinned commit.

### D8. Library and settings paths
- Library: `/Games/GBA|GB|GBC`, matching slot. advance.
- Art: `/SYSTEM/SLOT/…` in slot. advance's layout, so Cart Studio's slot. advance target works unchanged.
- Settings: `/SYSTEM/SLOT/SETTINGS.TXT`, a key=value subset (shelf, per-shelf index, theme).
- Saves: SuperFW's own `SAVEGAME/`, untouched.

### D9. EWRAM/IWRAM budget
- Firmware-side additions (hooks, ABI table, mode flag, path buffer) MUST stay ≤ 2 KB of EWRAM.
- Payload statics live in SDRAM.
- IWRAM additions (blitter) MUST stay ≤ 8 KB.
- The build log's `--print-memory-usage` lines are checked in each milestone's tasks.

## Risks / Trade-offs

- **[SDRAM execution/DMA too slow]** Carousel motion could stutter.
  → Dirty-band redraw (already in slot. advance). Move the blitter to IWRAM. Tune WAITCNT the way
  SuperFW does for games. Measure FPS on hardware at the M1 checkpoint. Fallback: reduce motion
  frames.
- **[SDRAM region collision]** The payload or art could overwrite or be overwritten by menu state.
  → Static asserts on the region. The implementer greps every `0x08`/`0x09` SDRAM use. A
  hardware check browses the stock UI after the carousel (file lists, recent) to confirm no
  corruption.
- **[Stock video state not fully restored]** The stock UI could show garbage after the carousel.
  → Save/restore the register set and palette, and clear VRAM. Preview screenshots after a
  Select round-trip (stock UI stubbed as a solid Mode 4 screen).
- **[Upstream merge friction]** SuperFW changes `menu.c` often.
  → Hooks stay few and small, with `// slot:` markers. `git diff upstream/master -- src/ Makefile`
  is reviewed at each milestone.
- **[slot. advance moves fast]** Daily pushes.
  → Pinned at tag `v1.2.4` (`a2a2fe9`). Re-pin deliberately.
- **[Image size]** Art plus payload could push the image past 2 MB.
  → Build-time cap check. Drop the realistic-art built-ins in favour of plain moulds if needed.
- **[Emulator can't validate hardware paths]** SD, SDRAM mode, launch and saves are hardware-only.
  → Per-milestone hardware checklist in README; only the user verifies.
- **[In-game exit lands in stock flashed SuperFW]** Known and accepted for SD-boot. Documented;
  return-to-carousel is in TODO.

## Migration Plan

- **Install:** copy the built `.gba` to the SD root, keeping `Games/` and saves. Launch it from
  stock SuperFW.
- **Rollback:** delete the `.gba`. Stock flashed SuperFW is untouched.
- **Card data:** `/SYSTEM/SLOT` is additive. `SAVEGAME/` and `.superfw/` are shared with stock and
  not reformatted.

## Open Questions

- ~~Which mGBA headless/scripting mechanism works?~~ RESOLVED (task 2.1): Homebrew mGBA has no `--script`;
  `libmgba-dev` 0.10.5 in the Docker image plus `tools/mgba_shots.c` works headless.
- ~~Exact free SDRAM window~~ RESOLVED (task 3.1): `0x09800000-0x09DFFFFF` is unusable (patch DB at
  `0x09D00000`, and the whole `0x09xxxxxx` half is shadowed by the SD interface). The window is
  `0x08B80000-0x08E7FFFF`; see "SDRAM map (audited)".
- (Answered in 3.6: `menu_popup_active()` polled by `slot_poll()` each frame; no callback needed. Select is
  already used by stock on Recent/Browse/NOR, so return-to-carousel works only from Settings/UI/Tools/Info.)
  Original question: Does stock SuperFW's popup flow give a clean "popup dismissed" hook, or does `menu.c` need a
  small callback? (M1 task; prefer a one-line hook.)
- Is the slot. advance on-card `/SYSTEM/SLOT` format stable across v1.2.x? (M2 task: pin and test
  with `convert_labels.py` output.)
- No ADRs exist yet; nothing to supersede.

## M1 implementation notes (sections 4-5)

- **ABI v2.** `struct slot_host_api` gained `f_unlink` (slot. advance's safe settings write is
  tmp -> unlink bak -> rename); `struct slot_mailbox` gained `entry_reason` (`SLOT_ENTER_BOOT` shows
  the splash, `SLOT_ENTER_RESUME` is a re-entry). `SLOT_ABI_VERSION` is 2 on both sides.
- **No IRQ in the payload.** slot. advance ticks its navigation spring from its own VBlank IRQ.
  The payload may not touch IE/IME, so `Slot_SyncTicks()` replays every VBlank since the last call
  from the host's IRQ-driven `frame_count` (same result: the tick depends only on frame number and
  key state). Frame sync is `wait_for_vblank()` until `frame_count` changes.
- **Re-entry state.** The payload's statics are re-zeroed on every entry, so the selection survives
  a Select round trip or a dismissed load popup through `SETTINGS.TXT` (written on Select and before
  launch), not through the mailbox.
- **Assets (M1).** No pack yet (6.1). `tools/slot_assets_gen.py` extracts the wood background, the
  dark front/back bands, the three plain carts, the font and the splash (RLE, 2.9 KB) from the pinned
  submodule into `src/slot/slot_art_gen.c` at build time; they are linked into the payload's SDRAM
  image (about 168 KB of the 256 KiB). The fast-bank copy machinery of `slot_fast_cache.h` is
  dropped: the sources already sit in SDRAM.
- **Scan memory.** One shelf is scanned at a time (rescan on shelf change, like slot. advance):
  `SLOT_SHELF_MAX` = 2048 entries, names in a 256 KiB pool; scanning stops at either limit and flags
  the shelf truncated. `_Static_assert`: frame + cache + 3 shelves fit the window's scratch.
- **Preview ROM (D6 refinement).** An emulator's ROM space is read-only, so the real payload
  (statics, back buffer and shelf in the 0x08 SDRAM window) cannot run there byte-for-byte.
  `slot-preview.gba` links the same carousel sources against `preview_host.c` into an ordinary ROM
  (`ldscripts/slot_preview.ld`) with statics and scratch in EWRAM. The entry path (`slot_entry`
  copying IWRAM code, `.data`, zeroing `.bss`) is the real one.
- **Performance is unmeasured.** The back buffer is in SDRAM (EWRAM is reserved, ADR-0005), so every
  pixel write is a cartridge-bus write. Dirty-band redraw and the IWRAM pixel loops are in; the M1
  hardware checkpoint decides whether more is needed (banded rendering through an IWRAM tile, WAITCNT).


## M2 implementation notes (section 6)

- **Pack (D5 refinement, ADR-0011).** The pack is built by `tools/slot_pack.py` (not `slot_assets_gen.py`), compressed with
  apultra and unpacked by `rom_boot.S` into `0x08E00000-0x08E7FFFF`, the top of the window; scratch is
  `0x08BC1000-0x08DFFFFF` (2.25 MiB: frame 75 KiB, cache 116 KiB, span tables 1.1 KiB, shelf up to 260 KiB).
  The payload image no longer holds art (15 KB).
- **Art loader.** BMPs are decoded row by row through an IWRAM buffer (FatFs never writes SDRAM) and copied
  with `Slot_Copy`. Opaque-span tables are per cache slot and live in SDRAM, built from the decoded rows,
  because Cart Studio outlines differ per ROM (GB vs GBC shape). Shell colours are baked into the BMPs by
  the converter. The initial fill of a shelf is synchronous; scrolling streams 24 rows per frame.
- **Themes.** Wood/Dark/Light and the flat (`Skeuomorphic=0`) variants; cycled with B+Start (L+Start would
  switch shelf on the L press).
- **Sound.** See ADR-0011: 704-byte IWRAM buffer refilled every VBlank, Timer 0 period chosen so a frame
  is exactly 352 samples, no IRQ, DMA1 reads only IWRAM, full stop on every return.

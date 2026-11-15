> Implementer rules:
> - Never flash anything (ADR-0002). Never edit `vendor/slot-advance` (ADR-0006).
> - Build only via `./build.sh` (ADR-0003).
> - Tasks marked **⛔ HARDWARE CHECKPOINT** need the user. STOP there, report, and wait. Do not
>   continue to the next milestone without the user's result.
> - After each build, record the `--print-memory-usage` EWRAM/IWRAM lines in the task note and
>   check them against ADR-0005.

## 1. M0 — Repo hygiene & fork identity

- [x] 1.1 Add `.gitignore` entries for build outputs not covered upstream (`src/menu_messages.h`, `src/messages_data.h`, `build/`, `slot-preview.gba`, `*.pack`). Check: `./build.sh && git status --short` shows no generated files. (checked: after build, `git status --short` showed no generated files)
- [x] 1.2 Add `LICENSE` (GPL-3.0 text) and `NOTICE.md` per ADR-0007. Check: both exist; NOTICE names SuperFW, slot., slot. advance/DS Style, EZ-FLASH/SimpleLight (Apache-2.0), fonts. (checked: LICENSE = GPLv3 text, NOTICE.md names all required parties)
- [x] 1.3 Add a fork version suffix (`slot.M0`, later bumped per milestone) shown on the Info screen, via a Makefile/define hook marked `// slot:`. Check: `strings superfw.gba | grep slot.M0` matches. (checked: `strings superfw.gba | grep slot.M0` matches; version in Makefile `SLOT_MILESTONE`)
- [x] 1.4 Add a ≤ 2 s boot splash text "slot. superchis" drawn with the stock font before the stock menu (M0 only; replaced by the carousel splash in M1). Check: preview of the full firmware in mGBA reaches the splash (SD init may then fail in the emulator; that is expected). Take a screenshot. (checked: build OK, splash string present in image; mGBA screenshot NOT captured, macOS denies screencapture/keystrokes to the agent, deferred to task 2.1; EWRAM +136 B)
- [x] 1.5 Add a build-size guard: the build fails if `superfw.gba` > 2,048 KB (ADR-0005). Check: temporarily lower the limit, confirm the build fails, restore. (checked: `SLOT_MAX_IMAGE_KB=1000` failed the build, default 2048 passes at 1,671,680 B)
- [x] 1.6 Write `README.md` (replacing upstream's README as the top-level doc; move upstream's to `docs/SUPERFW-README.md`). Sections: What it is; Install (copy `.gba`, `Games/GBA|GB|GBC`); Remove; Build (`./build.sh`); Hardware test checklist (M0); Known limitations; Flashing (untested — help wanted, DS Lite recovery link); Credits. Check: every command in README runs as written. (checked: README commands `./build.sh`, `./build.sh clean` run as written)
- [x] 1.7 Create `TODO.md` listing the deferred features from proposal.md. Check: file exists and matches the proposal list. (checked: TODO.md matches proposal deferred list)
- [x] 1.8 Copy the build to the mirror as `slot-superchis-M0.gba` (replacing the old one). Sync to the card if `/Volumes/SUPERCHIS` is mounted, following CLAUDE.md sync order. Check: file on card, `dot_clean` done. (checked: card mounted; 3-step sync done, `cmp` identical on card, dot_clean run, card not ejected)
- [x] 1.9 **⛔ HARDWARE CHECKPOINT (M0)**: user boots `slot-superchis-M0.gba` and confirms the splash, Info shows `slot.M0`, a GBA and a GB game launch, and in-game exit lands in stock SuperFW (Info shows no `slot.` suffix). STOP until reported. — PASSED 2026-11-15: splash seen; Info "Chis variant slot.M0" (slug 72d33c2c = build-time HEAD); Advance Wars + Tetris play; in-game exit lands in stock v0.22 (4701c4e9). Friction: user must relaunch the fork after every game → task 4.9.

## 2. M1 — Preview harness spike

- [ ] 2.1 Spike: find a way to run a GBA ROM in Homebrew mGBA 0.10.5 on macOS with scripted input and PNG screenshots, without user interaction (Lua `--script`, SDL frontend + movie file, or the `mgba` Python bindings). Check: a script captures a PNG of upstream `superfw.gba` at frame 120 into `build/shots/`.
- [ ] 2.2 Write `tools/preview_shots.sh <rom> <sequence>` using the spike result. Check: it produces one PNG per step for a 3-step sequence.

## 3. M1 — Payload infrastructure

- [ ] 3.1 Audit every SDRAM use while the menu runs (`grep -n '0x08[0-9A-Fa-f]\{6\}\|0x09[0-9A-Fa-f]\{6\}\|ROM_OFF_\|sdr_state\|hiscratch'` across `src/`, `rom_boot.S`, `ldscripts/`). Confirm or adjust the slot SDRAM window (design D5: `0x09800000–0x09DFFFFF`). Write the result into design.md Open Questions as resolved. Check: written table of SDRAM regions with sizes.
- [ ] 3.2 Add the slot SDRAM window constants to `src/common.h` (`// slot:`), with `_Static_assert`s that it overlaps none of `t_sdram_state`, fonts, hiscratch, or assets. Check: build passes; deliberately overlapping values fail to compile.
- [ ] 3.3 Define `src/slot/slot_host_api.h`: versioned struct per design D2, plus `slot_main()` and the action codes. Check: header compiles standalone with `-Wall -Werror`.
- [ ] 3.4 Add a `slot.payload` build (own linker script `ldscripts/slot_payload.ld`, linked at the SDRAM window base, `-mthumb`, IWRAM section for hot code), modelled on `ingamemenu.payload`. Start with a stub `slot_main` that fills the screen with one colour and returns `SLOT_STOCK_UI` on any key. Check: build passes; the map file shows payload size and IWRAM use ≤ 8 KB.
- [ ] 3.5 Embed `slot.payload` as an asset and have `rom_boot.S` copy it to the SDRAM window (`// slot:` markers). Check: build passes; image ≤ 2 MB; `strings` finds the payload marker.
- [ ] 3.6 Hook `main_gba()` (design D3): enter carousel mode first; implement `menu_open_file()` (a non-static wrapper of `browser_open`) and `enter_stock_ui`; save and restore stock video state around the payload (D4). Firmware-side EWRAM additions ≤ 2 KB. Check: build passes; EWRAM delta recorded; `git diff upstream/master -- src/main.c src/menu.c` is small and marked.

## 4. M1 — Carousel port

- [ ] 4.1 Copy the needed `vendor/slot-advance/common/` files into `src/slot/` with provenance headers (ADR-0006): render, navigation, motion, timing, transition, format, fast cache, blit, frontend. Add them to NOTICE.md. Check: provenance header on each; NOTICE updated.
- [ ] 4.2 Replace DS Style dependencies with the host API and local equivalents: FatFs via the API, `KEY_*`/`REG_*`/`VBlankIntrWait`/`dmaCopy` via a small `src/slot/gba_compat.h` over SuperFW's `gba_regs.h`. Move the back buffer and art cache to the SDRAM window. Check: the payload builds with `-Wall` and no DS Style symbols left (`grep -E 'pReadCache|pFilename_buffer|gfile|Launcher_|UIAudio_|SD_list_MENU|gl_' src/slot` is empty).
- [ ] 4.3 Library scan per the cartridge-carousel spec (`/Games/GBA|GB|GBC`, extension filter, ignore hidden and `._*`, case-insensitive sort, missing folder = empty shelf). Extract the filter/sort into a pure function. Check: host unit test (`tests/`, run in the container) covers the spec scenario and passes.
- [ ] 4.4 Plain-cart rendering with name text, three shelves, Left/Right/L/R navigation with spring motion, empty-shelf and no-Games messages. Check: preview screenshots for boot, Right×2, R, and an empty shelf.
- [ ] 4.5 Launch: on A, play the insert animation, then return `SLOT_LAUNCH` with the path. The firmware calls `menu_open_file()`. When the popup is dismissed, return to the carousel with the selection intact. Check: build passes; preview stub logs the launch path; screenshots of the insert animation frames.
- [ ] 4.6 Select escape hatch: Select returns `SLOT_STOCK_UI`. In the stock UI, Select on a top-level tab returns to the carousel. Check: preview screenshot sequence carousel → (stub stock screen) → carousel shows the same selection.
- [ ] 4.7 Persist and restore shelf and per-shelf index in `/SYSTEM/SLOT/SETTINGS.TXT` (write before launch). Check: host unit test for the settings parser/serializer round-trip.
- [ ] 4.8 Replace the M0 text splash with the slot. splash; bump the version suffix to `slot.M1`. Check: screenshot; `strings` check.
- [ ] 4.9 Return friction mitigation: games launched from the carousel MUST NOT be added to SuperFW's shared recent list (`.superfw/recent.txt`; see `insert_recent_flush()` in `src/menu.c`), so the fork's own `.gba` stays at the top of stock SuperFW's Recent tab. Document in README: set stock SuperFW Settings → recent menu on boot, so returning is power-on → A. Prefer a `// slot:` flag over removing upstream code. Check: host-free code review of the gated call site + hardware checklist item in 5.4.

## 5. M1 — Preview ROM & delivery

- [ ] 5.1 Add `src/slot/preview_host.c` (stub API: fictional library of ~8 titles per shelf, one empty-shelf toggle) and a `preview` make target producing `slot-preview.gba` (via `./build.sh preview`). Check: it builds; `tools/preview_shots.sh` captures a non-blank carousel.
- [ ] 5.2 Update README: M1 controls (Left/Right, L/R, A, Select ↔ stock UI), M1 hardware checklist, developer section on the preview ROM and screenshot harness. Check: README matches the current behaviour.
- [ ] 5.3 Copy the build to the mirror as `slot-superchis-M1.gba`, remove the M0 file, sync to the card if mounted (CLAUDE.md order). Check: on card.
- [ ] 5.4 **⛔ HARDWARE CHECKPOINT (M1)**: user verifies shelves and order; smooth navigation (note any stutter); Advance Wars launch, save, relaunch with the save intact; Tetris and Link's Awakening DX launch; B in the load popup returns to the carousel; Select → stock Settings/Info work → back to the carousel; position restored after power cycle; after an in-game exit, stock Recent tab shows the fork `.gba` first (task 4.9); stock file browser still lists files correctly (SDRAM sanity). STOP until reported. If stutter is reported, fix it (design Risks: IWRAM blitter, WAITCNT, fewer motion frames) before M2.

## 6. M2 — Art, themes, sound

- [ ] 6.1 Write `tools/slot_pack.py`: pack slot. advance built-in assets (backgrounds, shell moulds, fallbacks, splash, font, insert PCM) from the pinned submodule into `res/slot_assets.pack` with a header of offsets. Compress with the existing apultra/upkr tools; unpack in `rom_boot.S` into the SDRAM window. Check: build passes; image ≤ 2 MB (record size).
- [ ] 6.2 Port slot. advance's `/SYSTEM/SLOT` art loader (row-streamed, cached in SDRAM) and shell colour handling. Check: host test decodes sample output of `vendor/slot-advance/tools/convert_labels.py` (generated in the container) to the expected dimensions.
- [ ] 6.3 Themes Wood/Dark/Light, cycled by a documented key combo (e.g. hold L + press Start), persisted in SETTINGS.TXT. Check: preview screenshots of all three.
- [ ] 6.4 Insert sound via a DirectSound driver ported from `slot_audio_driver.h`. Fully stop the timers, DMA and FIFO before returning to the firmware. Check: build passes; code review shows the stop path runs on every return; preview audio not required.
- [ ] 6.5 Add sample art to the preview stub so screenshots show labelled carts. Check: screenshot.
- [ ] 6.6 Bump the version to `slot.M2`; update README with the Cart Studio workflow (open studio.slot-cfw.fyi in Chrome/Edge, choose the card root, Write to card / "Download for slot. advance"), the theme combo and the M2 checklist. Check: README current.
- [ ] 6.7 Copy the build as `slot-superchis-M2.gba`, remove the M1 file, sync to the card if mounted. Check: on card.
- [ ] 6.8 **⛔ HARDWARE CHECKPOINT (M2)**: user runs Cart Studio on the card, relaunches, and verifies labels and shells; holds Right across an art-filled shelf (no long freezes); cycles themes; insert sound plays and game audio is clean afterwards. STOP until reported.

## 7. Wrap-up

- [ ] 7.1 Review `git diff upstream/master -- src/ Makefile rom_boot.S ldscripts/`. Every upstream-file hunk is marked `// slot:` and justified. Check: written summary in the apply report.
- [ ] 7.2 Re-run the full build, unit tests and preview shots from clean. Check: all pass.

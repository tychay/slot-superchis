# Notices

slot. superchis is a combined work distributed under **GPL-3.0-or-later** (see `LICENSE`, ADR-0007).
Original licence headers in every file are retained. Binary releases must ship with, or link to,
the corresponding source.

## Credits

- **SuperFW**: David Guillen Fandos (davidgf), https://github.com/davidgfnet/superfw.
  GPL-3.0-or-later. This repository is a continuation fork of it; all upstream source files keep
  their headers. Third-party pieces inside SuperFW: nanoprintf (public domain), heapsort (3-BSD),
  FatFs by ChaN (1-BSD-like), apultra (zlib) and upkr (public domain) re-implementations used at
  build time, linkerscript/crt0 code adapted from AntonioND (CC0). See `docs/SUPERFW-README.md`.
- **slot.**: Brandon Kowalski and contributors, GPL-3.0-or-later; cartridge artwork by Markus
  (MarkusMaribu). The carousel concept, spring navigation and artwork are adapted from slot. and
  its GBA port.
- **slot. advance / DS Style**: FrankieT19, https://github.com/FrankieT19/slot-advance,
  GPL-3.0-or-later. Pinned read-only as the submodule `vendor/slot-advance`. Code adapted from it
  lives in `src/slot/` as attributed copies (ADR-0006), listed under "Adapted files".
- **DS Style / EZ-FLASH / SimpleLight** (via slot. advance): Apache-2.0. EZ-FLASH original
  backends, SimpleLight work by Sterophonick and the contributors credited in slot. advance's
  `kernels/*/UPSTREAM_README.md`. Any file adapted from these keeps its Apache-2.0 notice and is
  listed here when it is copied.
- **Fonts**: SuperFW's bundled font pack (`res/fonts/`) is generated from GNU Unscii
  (http://viznut.fi/unscii/, GPL) and a filtered subset of GNU Unifont for Hangul. slot. advance's
  label and DejaVu fonts (OFL and the DejaVu licence respectively; texts in
  `vendor/slot-advance/assets/slot/`) are not used at M0 and will be listed here when copied.

## Adapted files

Source commit for all of them: slot. advance v1.2.4, `a2a2fe9` (submodule `vendor/slot-advance`,
GPL-3.0-or-later). Each file's header names its source and what was changed.

| File in `src/slot/` | Source in `vendor/slot-advance/common/` | Status |
|---|---|---|
| `slot_motion.h` | `slot_motion.h` | copied unmodified |
| `slot_navigation.h` | `slot_navigation.h` | copied; `Slot_DialogCanClose` dropped |
| `slot_hotkeys.h` | `slot_hotkeys.h` | copied unmodified (validates two settings values) |
| `slot_format.h` | `slot_format.h` | `Slot_IndexForCount`, `Slot_ParseNatural` kept; BMP checks dropped (M2) |
| `slot_timing.h` | `slot_timing.h` | adapted: no IRQ, ticks replayed from the host frame counter |
| `slot_transition.h` | `slot_transition.h` | adapted: fades and splash on gba_compat.h; splash is RLE-packed |
| `slot_render.h` | `slot_render.h` | adapted: Wood theme only, SDRAM frame buffer, plain carts, new footer/title rows; settings/About/Help/board pages dropped |
| `slot_fast_cache.h` | `slot_fast_cache.h` | adapted: EWRAM fast banks dropped, `Slot_DrawShelfCarts` kept |
| `slot_blit.s` | `slot_blit.s` | copied; `Slot_DecodeBMP555` dropped (M2) |
| `slot_frontend.c` | `slot_frontend.h` | adapted: host API instead of DS Style, pure scan/settings modules, launch via mailbox; settings pages, cart back, clock, username, sounds, hotkeys, eject-replay, BMP art loader dropped |
| `slot_scan.c`, `slot_scan.h` | `slot_frontend.h` (Slot_Scan, Slot_SelectDirectory) | rewritten as pure functions with a shelf cap |
| `slot_settings.c`, `slot_settings.h` | `slot_frontend.h` (Slot_ReadConfigFile, Slot_SaveConfig) | rewritten on a memory buffer; same file format |

Not copied (post-M1): `slot_assets.h` is never copied: `tools/slot_assets_gen.py` extracts the few
arrays M1 draws (wood background, dark front/back bands, plain GBA/GB/GBC cartridges, font, splash;
artwork by Markus) from the submodule into a build-time generated file `src/slot/slot_art_gen.c`.
`slot_audio_driver.h`, `slot_sounds.h`, `slot_cartridge_settings.h`, `slot_clock.h`,
`slot_return*.h/.inc`, `slot_username.h` and the emulator/DS Style kernel files are not used.

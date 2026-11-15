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
  its GBA port (not yet present in the firmware at M0).
- **slot. advance / DS Style**: FrankieT19, https://github.com/FrankieT19/slot-advance,
  GPL-3.0-or-later. Pinned read-only as the submodule `vendor/slot-advance`. Code adapted from it
  lives in `src/slot/` as attributed copies (ADR-0006). Nothing has been copied at M0.
- **DS Style / EZ-FLASH / SimpleLight** (via slot. advance): Apache-2.0. EZ-FLASH original
  backends, SimpleLight work by Sterophonick and the contributors credited in slot. advance's
  `kernels/*/UPSTREAM_README.md`. Any file adapted from these keeps its Apache-2.0 notice and is
  listed here when it is copied.
- **Fonts**: SuperFW's bundled font pack (`res/fonts/`) is generated from GNU Unscii
  (http://viznut.fi/unscii/, GPL) and a filtered subset of GNU Unifont for Hangul. slot. advance's
  label and DejaVu fonts (OFL and the DejaVu licence respectively; texts in
  `vendor/slot-advance/assets/slot/`) are not used at M0 and will be listed here when copied.

## Adapted files

None yet (M0 contains only fork identity changes to upstream SuperFW files, each marked `slot:`).

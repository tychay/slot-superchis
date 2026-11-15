# 0005. EWRAM is reserved for stock SuperFW; slot memory and assets live in SDRAM

- Status: accepted
- Date: 2026-11-14

## Context

Stock SuperFW `BOARD=chis` uses 234,848 / 257,024 B of EWRAM. slot. advance's art is about
360 KB of const arrays, and its back buffer is 76.8 KB. `rom_boot.S` already unpacks compressed
asset blobs into SDRAM regions (fonts at `0x08F00000`, assets at `0x09E00000`). The flashed
image is capped at 2,048 KB for chis.

## Decision

- Firmware-side additions are capped at **≤ 2 KB EWRAM**.
- Payload IWRAM use is capped at **≤ 8 KB**.
- All other slot state goes in SDRAM: back buffer, art cache, paths, settings, payload statics.
- slot. art and sounds ship as a compressed SDRAM blob (`res/slot_assets.pack`), unpacked by
  `rom_boot.S` into a reserved SDRAM window. That window is declared in `src/common.h` and
  guarded by `_Static_assert`s against `t_sdram_state`, fonts, hiscratch and assets.
- The build fails if `superfw.gba` exceeds 2,048 KB.

## Consequences

- Stock behaviour and stability margins are preserved.
- Every new buffer needs an explicit placement decision, checked against the build's
  `--print-memory-usage`.
- The SDRAM window must be re-verified whenever upstream changes its SDRAM map.

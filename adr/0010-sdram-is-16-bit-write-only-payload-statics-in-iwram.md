# 0010. SuperCard SDRAM takes only 16/32-bit writes; payload statics live in IWRAM

- Status: accepted, supersedes ADR-0005 (payload IWRAM cap, statics placement) and ADR-0009 (statics sub-region)
- Date: 2026-11-15
<!-- Supersedes: ADR-0005 -->
<!-- Supersedes: ADR-0009 -->

## Context

The first M1 hardware boot showed the payload's splash, then a white (force-blanked) screen
that never cleared. The hang sits between the splash and the first shelf: settings read,
`/Games` stat and the shelf scan.

The SuperCard/SuperChis SDRAM sits on the 16-bit cartridge bus, and stock SuperFW never
byte-stores into it:
- `src/patcher.c:162`: "Write a byte to a buffer ensuring that only 16 bit accesses are performed".
- `src/heapsort.c:36`: "suitable to use in 16 bit buses".
- `src/cheats.h:25`: padding "helps with 16 bit writes".
- Browser file names are copied into SDRAM with `dma_memcpy16`.

ADR-0005 and ADR-0009 placed the payload's `.data`/`.bss` in SDRAM, and the shelf name pool
is in SDRAM scratch. FatFs (`FILINFO.fname`), `slot_scan`, the settings parser and the string
helpers all use `strb`, which corrupts neighbouring bytes on hardware. The preview ROM puts
statics and scratch in EWRAM, so the emulator could not see this.

## Decision

- Payload `.data` and `.bss` are linked into IWRAM, together with `.iwram`. The window grows from
  8 KiB to **12 KiB**: `0x03003000–0x03005FFF`. The firmware user stack keeps
  `0x03006000–0x03007F00` (about 7.9 KiB) while the payload runs. Nothing in the window
  persists across payload entries, which is unchanged from ADR-0005.
- The SDRAM statics sub-region (`0x08BC1000`, 64 KiB) is retired. Its range joins scratch.
- **Rule:** payload and firmware code store into SDRAM (scratch, mailbox) only with 16- or
  32-bit accesses. Byte data destined for SDRAM is built in IWRAM or on the stack, then copied
  with a halfword helper. Byte *reads* from SDRAM are fine.
- FatFs objects (`FIL`, `DIR`, `FILINFO`) and their buffers are never in SDRAM.
- Everything else in ADR-0005 (EWRAM ≤ 2 KB, assets in SDRAM, 2 MB image cap) and ADR-0009
  (window location, browser cap 12,288) stays in force.

## Consequences

- Any new SDRAM buffer needs a write-width check in review: `strb` into scratch is a hardware-only bug.
- The payload's IWRAM budget now includes its statics. Linker asserts guard 12 KiB.
- Stack headroom while the carousel runs drops from about 11.7 KiB to about 7.9 KiB.
- The preview ROM cannot detect byte stores into SDRAM, so this class of bug is caught only by
  review or on hardware.

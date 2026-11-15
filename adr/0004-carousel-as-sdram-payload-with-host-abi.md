# 0004. Carousel runs as an SDRAM payload behind a narrow host ABI

- Status: accepted
- Date: 2026-11-14

## Context

Stock SuperFW runs from EWRAM and uses 91.4 % of it (~22 KB free). IWRAM has ~20 KB free. The
slot. frontend code is roughly 2.5k dense lines plus a renderer, so it cannot fit in either.
SuperFW already builds separately linked payloads (`ingamemenu.payload`, `directsave.payload`)
and embeds them as blobs. The SuperChis SDRAM is the cart's ROM bus, where games execute.

Alternatives considered:
- link into the EWRAM image (does not fit);
- IWRAM only (too small);
- shrink the stock firmware to make room (invasive, fights upstream).

## Decision

- The carousel is built as `slot.payload`, linked at a fixed SDRAM address, copied there at
  boot by `rom_boot.S`, and run from SDRAM.
- The firmware calls `slot_main(const struct slot_host_api *)`, a table of host services
  (FatFs, input, vblank, open-file hand-off, enter-stock-UI, scratch memory). It gets back an
  action code.
- The payload never links against firmware symbols.
- Hot inner loops may be placed in IWRAM, within the budget in ADR-0005.

## Consequences

- The firmware and payload are coupled only through the ABI struct. A layout change must be
  made on both sides together, guarded by a version field.
- The same payload links against a stub host to make the emulator preview ROM.
- Execution and DMA from SDRAM are slower than from EWRAM. Rendering must stay dirty-band and
  be measured on hardware.
- Future features (settings screen, clock, return animation) go in the payload, not the
  EWRAM image.

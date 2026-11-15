# 0011. The asset pack sits at the top of the SDRAM window; the insert sound streams from an IWRAM buffer

- Status: accepted, supersedes ADR-0009 (scratch sub-region size) and amends ADR-0005 (pack location)
- Date: 2026-11-15
<!-- Supersedes: ADR-0009 -->

## Context

M2 adds Dark/Light themes, flat carts, Cart Studio art and the insert sound. The art and the PCM
are about 280 KiB uncompressed. ADR-0005 planned a compressed pack unpacked at boot but did not
place it, and ADR-0009's window had no room reserved for it. Two hardware facts bound the design
(ADR-0010 and the 2026-11-15 SLOT_DIAG run): SuperCard SDRAM takes only 16/32-bit stores, and DMA
from an SDRAM source returns wrong data or hangs. DirectSound is DMA from memory to the FIFO, the
clip lives in the pack (SDRAM), IWRAM for the payload is a fixed 12 KiB (9.7 KiB used), and EWRAM is
the firmware's.

## Decision

- **Pack location.** `res/slot_assets.pack` (built by `tools/slot_pack.py`, apultra-compressed,
  about 37 KB of the image) is unpacked by `rom_boot.S` into `SLOT_PACK_ADDR` = `0x08E00000`,
  the top 512 KiB of the window, using the same copy-to-hiscratch-then-`apunpack16` sequence the
  stock font pack uses (halfword stores only). Scratch shrinks to `0x08BC1000-0x08DFFFFF`
  (2.25 MiB); `common.h` asserts the new boundaries. The payload checks the pack header and falls
  back to the stock UI if it is absent.
- **Art decode.** Cart BMPs are read with `f_read` one row at a time into an IWRAM row buffer
  (FatFs never writes SDRAM), converted there, and copied to the SDRAM cache with `Slot_Copy`; each
  cart's opaque-span table (any outline is possible) is built from the same rows and stored in
  SDRAM with `slot_sdram_store`.
- **Sound.** The PCM (resampled by the packer to 21,024 Hz, exactly 352 samples per 280,896-cycle
  frame, Timer 0 period 798) is never a DMA source. A 704-byte IWRAM buffer holds the current and
  next frame; every VBlank (`Slot_WaitFrame`) the payload stops DMA1 and Timer 0, resets FIFO A,
  refills the buffer with `Slot_Copy` and restarts. The frame is chosen from the VBlank count, so a
  late wake-up still plays the right audio and one skipped frame plays the queued next frame.
  DMA1 reads only IWRAM. The payload has no IRQ, so everything is paced by the VBlank wait.
- **Stop.** The launch path waits for the clip to end; `slot_main` calls `Slot_AudioShutdown()` on
  every return, which disables DMA1 and Timer 0, resets the FIFO and restores SOUNDCNT_X/H and
  SOUNDBIAS.
- Nothing slow (SD writes) may run while the clip plays: settings are saved before the insert
  animation starts.

## Consequences

- IWRAM use grows by 704 B (buffer) plus statics; ADR-0010's 12 KiB cap still holds with margin.
- A stall of more than two VBlanks while the clip plays would let DMA1 read past the buffer
  (noise); nothing in the carousel stalls that long while it plays.
- The pack region and the scratch size are layout constants in `slot_layout.h`; changing them needs
  a new ADR. Pack growth beyond 512 KiB needs a new layout.
- The unpack runs at every boot (after the payload copy); it is the same code path as the font
  pack on the SD and Lite boards but has not been exercised on a SuperChis before M2.

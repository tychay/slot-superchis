# 0002. Deliver by SD chain-load only; never flash

- Status: accepted
- Date: 2026-11-14

## Context

SuperFW can boot another SuperFW build from the SD card as if it were a game (`is_superfw()`).
This was verified on the author's SuperChis on 2026-11-14 (Info showed `0.22 (dabbf490) Chis`).
Flashing to the cart's NOR is the only irreversible step in the project. Recovering from a bad
flash needs an NDS/DS Lite running superfw-nds-flasher-tool, and the author owns only a GBA.

## Decision

- The deliverable is a `.gba` file. The user copies it to the SD card and launches it from stock
  flashed SuperFW.
- The project never flashes, and contains no automated or default flashing step.
- README carries a "Flashing (untested — help wanted)" section for DS Lite owners. It states that
  the author has not flashed the build, and links the recovery tool.

## Consequences

- Rollback is deleting one file, so any build is safe to try.
- After an in-game exit, the cart resets into the *flashed stock* SuperFW, not the carousel. Any
  return-to-carousel feature must work within that constraint, or wait for a supersession of
  this ADR.
- Flash-only features (SDHC/exFAT benefits of a flashed install, NOR games) are out of scope
  until someone with recovery hardware validates flashing.

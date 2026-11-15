# Hardware test checklist

For testers and for checking a release on real hardware. The emulator can't catch SuperCard
memory problems (see [ADR-0010](../adr/0010-sdram-is-16-bit-write-only-payload-statics-in-iwram.md)),
so the release `.gba` must pass these on a real cart.

Put a few unzipped games in each of `Games/GBA`, `Games/GB` and `Games/GBC`, add
[label art](../README.md#label-art) if you want to test it, then boot the release `.gba` from stock
SuperFW.

## Carousel

- [ ] The "slot." splash, then "Reading shelf...", then the carousel. Each shelf is in alphabetical
      order (case-insensitive).
- [ ] Left/Right moves between carts, holding scrolls, L/R switches shelf. Note any stutter.
- [ ] Carts with art show their labels and cart colours; carts without art show a plain cart with
      the name. Hold Right across a shelf full of art: no long freezes.
- [ ] Hold B and press Start: the theme cycles Wood, Dark, Light. The footer is readable in all three.
- [ ] Change shelf, cart and theme, wait a few seconds, power-cycle and relaunch: all three are
      restored.
- [ ] Select → stock SuperFW menu → Select on the Settings, UI, Tools or Info tab returns to the
      carousel. Info shows the version with a `slot.` suffix.
- [ ] The stock Browse tab still lists files correctly.

## Launching

- [ ] A on a GBA game: insert animation and insert sound, then SuperFW's load screen. B there
      returns to the carousel on the same cart.
- [ ] Start the GBA game. **Its own audio is clean** after the insert sound (no hiss, crackle or
      silence).
- [ ] Save in-game, quit through the in-game menu, relaunch the carousel and the game: the save is
      intact.
- [ ] A on a GB and on a GBC game: each starts straight away in SuperFW's emulator, with clean audio.
- [ ] After quitting a game, stock SuperFW's Recent tab has the carousel `.gba` at the top.

## If it fails

Run the `-diag.gba` from the release and photograph the screen. Top row: startup steps (grey =
stuck there, green = done). Second row: SDRAM tests (green = pass, red = fail, grey = hung). A blue
square on the third row means it reached the end; press A to continue to the carousel.

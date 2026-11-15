# TODO (deferred features)

Not part of the `add-slot-carousel` change (M0 to M2). Source: `openspec/changes/add-slot-carousel/proposal.md`.

- [ ] Return-to-carousel / eject animation after in-game exit (on SD-boot, exit resets into the
      flashed stock SuperFW). Stock SuperFW has no autoboot setting. Real fix needs either
      flashing (blocked by ADR-0002) or an upstream SuperFW "boot file at startup" setting
      (possible upstream PR). Interim mitigation: task 4.9 (Recent tab).
- [ ] slot.-style settings screen
- [ ] Footer clock
- [ ] `USERNAME.TXT`
- [ ] Hotkey remapping
- [ ] Eject sound
- [ ] NOR-flashed games shelf
- [ ] Other emulator shelves (NES, SMS, etc.)
- [ ] Translations
- [ ] Other SuperCard boards
- [ ] Flashed `.fw` install (documented only as untested, help wanted)
- [ ] Skip SuperFW's GBA load popup ("Load GBA game ROM") when launching from the carousel:
      start with the popup's defaults (save, patches, in-game menu) so A on a GBA cart boots
      directly, like GB/GBC. Keep a way to reach the popup (e.g. hold a key while pressing A).
- [ ] Optional: persist shelf/position on shelf switch, not only on launch (power-cycle without
      launching currently restores the last launched cart). Weigh against extra SD writes.

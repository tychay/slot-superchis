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
- [ ] **Before asking anyone to flash:** boot bypass. Holding Select (TBD) at power-on skips the
      carousel and opens the stock UI, so a carousel that hangs at boot (like the first M1
      hardware run) is still recoverable without a DS. Does not cover a broken firmware core or a
      bad flash; those need a DS/DS Lite + Slot-1 flashcart + superfw-nds-flasher-tool. README
      flash instructions must say: back up the stock flash with that tool first, and list the
      recovery steps.
- [ ] Skip SuperFW's GBA load popup ("Load GBA game ROM") when launching from the carousel:
      start with the popup's defaults (save, patches, in-game menu) so A on a GBA cart boots
      directly, like GB/GBC. The popup stays reachable through the core picker (next item).
- [ ] Core picker, like slot.'s Start-on-a-cart menu (https://slot-cfw.fyi/tour/#core-picker):
      Start on the selected cart opens a picker; one entry (Select) opens SuperFW's stock load
      screen for that file, with its ROM info. Uses: diagnostics for GB/GBC (which launch
      directly today, so their info screen is unreachable from the carousel) and the fallback
      route once GBA launches directly. Later entries could pick the GB core or patch options.
- [ ] Optional: persist shelf/position on shelf switch, not only on launch (power-cycle without
      launching currently restores the last launched cart). Weigh against extra SD writes.
- [ ] **Before going public:** `tests/fixtures/art/` and the preview ROM contain real Cart Studio
      label art (commercial box art) from the author's card. Replace with fictional labels
      (vendor `tools/make_examples.py` style) and purge them from git history before pushing.

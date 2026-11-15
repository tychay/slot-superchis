# 0007. Distribute as GPL-3.0-or-later with upstream notices retained

- Status: accepted
- Date: 2026-11-14

## Context

The inputs carry these licences:
- SuperFW source headers say GPL v3 "or (at your option) any later version". The repo has no
  LICENSE file.
- slot. and slot. advance are GPL-3.0-or-later.
- DS Style and EZ-FLASH-derived pieces in slot. advance are Apache-2.0.
- Fonts are OFL or DejaVu.

## Decision

- The combined work is GPL-3.0-or-later.
- Add a `LICENSE` file with the GPL-3.0 text.
- Add a `NOTICE.md` crediting:
  - SuperFW (David Guillen Fandos);
  - slot. (Brandon Kowalski and contributors; artwork by Markus);
  - slot. advance / DS Style (FrankieT19);
  - EZ-FLASH / SimpleLight, keeping their Apache-2.0 notices;
  - font licences.
- Original licence headers are never removed.

## Consequences

- Any binary release must ship with, or link to, the corresponding source.
- Code from GPL-incompatible sources cannot be brought in.

## ADDED Requirements

### Requirement: Label and shell art from /SYSTEM/SLOT
The carousel SHALL read per-game label art and shell colour from `/SYSTEM/SLOT` in the same
on-card format slot. advance reads (as written by slot. Cart Studio's "slot. advance" target or
slot. advance's Label Converter), matched by ROM file stem. Missing art MUST fall back to the
plain silhouette with name text.

#### Scenario: Cart Studio art shown (hardware)
- **WHEN** the user runs Cart Studio against the card, writes labels, and relaunches the fork
- **THEN** carts with art show their labels and shell colours; carts without art show the
  plain silhouette

#### Scenario: Art loading does not stall navigation (hardware)
- **WHEN** the user holds Right across a shelf where every cart has art
- **THEN** scrolling continues without multi-second freezes (art may stream in a frame or two
  late)

#### Scenario: Format parity (agent)
- **WHEN** the art loader is fed a sample `/SYSTEM/SLOT` tree produced by slot. advance's
  `tools/convert_labels.py`
- **THEN** a host test decodes each image to the expected dimensions without error

### Requirement: Themes
The carousel SHALL support slot. advance's Wood, Dark and Light themes, selected via a key
combination documented in README (no settings screen in this change) and persisted in
`/SYSTEM/SLOT/SETTINGS.TXT`.

#### Scenario: Cycle theme (agent, preview)
- **WHEN** the preview receives the theme-cycle key combination three times
- **THEN** screenshots show Wood, Dark, Light backgrounds in turn

### Requirement: Insert sound
Pressing A SHALL play slot.'s insert sound synchronized with the insert animation, and audio
hardware MUST be fully stopped before control passes to SuperFW's launch path.

#### Scenario: Sound then clean launch (hardware)
- **WHEN** the user launches a GBA game
- **THEN** the insert sound plays and the game's own audio is not corrupted

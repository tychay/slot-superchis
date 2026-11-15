## ADDED Requirements

### Requirement: Preview ROM build
The build SHALL provide a preview target (`./build.sh preview`) producing `slot-preview.gba`: the
same carousel code compiled against a stubbed library backend (fixed fictional titles across
all three shelves, built-in art, no SD/SDRAM/cart hardware access) so it runs in a stock
emulator.

#### Scenario: Preview builds (agent)
- **WHEN** `./build.sh preview` runs
- **THEN** it exits 0 and produces `slot-preview.gba`

#### Scenario: Preview runs in mGBA (agent)
- **WHEN** `slot-preview.gba` runs headless in mGBA for 300 frames
- **THEN** a screenshot shows the carousel (not a blank or crash screen)

### Requirement: Scripted screenshot harness
A script (`tools/preview_shots.sh` or equivalent) SHALL run the preview ROM in mGBA with a
scripted key sequence and save PNG screenshots to `build/shots/`, so the agent can inspect UI
changes without hardware.

#### Scenario: Navigation screenshots (agent)
- **WHEN** the harness runs the sequence boot → Right → Right → R → L
- **THEN** it writes one PNG per step, each showing the expected shelf and selection

### Requirement: Preview is not shipped
The preview ROM MUST NOT be presented as an installable firmware; README labels it a
developer tool.

#### Scenario: README labels preview (agent)
- **WHEN** README mentions `slot-preview.gba`
- **THEN** it is under a developer section, not install steps

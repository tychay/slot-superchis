## ADDED Requirements

### Requirement: Carousel is the home screen
After boot (and splash), the firmware SHALL show the cartridge carousel instead of SuperFW's
file browser.

#### Scenario: Boot to carousel (agent + hardware)
- **WHEN** the firmware finishes booting with a mounted SD card
- **THEN** the carousel is displayed with the last-used shelf (GBA by default)

### Requirement: Library scan
The carousel SHALL build three shelves from `/Games/GBA` (`.gba`), `/Games/GB` (`.gb`) and
`/Games/GBC` (`.gbc`), non-recursive, sorted case-insensitively by file name, ignoring hidden
files and macOS `._*` files. A missing folder MUST yield an empty shelf, not an error.

#### Scenario: Test card scan (hardware)
- **WHEN** the card has 10 files in each of `Games/GBA`, `Games/GB`, `Games/GBC`
- **THEN** each shelf shows exactly 10 carts in alphabetical order

#### Scenario: Scan logic (agent, host unit test)
- **WHEN** the scan/sort/filter function is given a directory listing containing `._x.gba`,
  `.hidden.gba`, `b.gba`, `A.gba`, `notes.txt`
- **THEN** it returns `A.gba`, `b.gba` only

#### Scenario: Empty shelf (agent, preview)
- **WHEN** a shelf has no files
- **THEN** the shelf shows an empty-slot message and A does nothing

### Requirement: Navigation
Left/Right SHALL move between carts (hold to repeat), L/R SHALL switch shelf (GBA ↔ GB ↔ GBC,
wrapping), with slot. advance's spring motion. The selected cart's name MUST be shown as text.

#### Scenario: Navigate and switch shelf (agent, preview)
- **WHEN** the preview ROM receives Right ×2 then R
- **THEN** screenshots show the third GBA cart selected, then the GB shelf

#### Scenario: Navigate on hardware (hardware)
- **WHEN** the user presses Left/Right and L/R on the test card
- **THEN** selection and shelf change smoothly with no visible tearing or freeze

### Requirement: Launch hand-off to SuperFW
Pressing A SHALL play the insert animation, then pass the selected file's full path and size
to SuperFW's existing launch path (`browser_open()`): GBA files open SuperFW's GBA load popup
(saves, patches, in-game menu unchanged); GB/GBC files start the bundled Goombacolor.

#### Scenario: Launch GBA with save (hardware)
- **WHEN** the user selects Advance Wars and presses A, confirms the load popup, saves in-game,
  then exits via the in-game menu and relaunches
- **THEN** the game boots and the save persists in `SAVEGAME/`

#### Scenario: Launch GB and GBC (hardware)
- **WHEN** the user launches Tetris (GB) and Link's Awakening DX (GBC)
- **THEN** each starts in Goombacolor

#### Scenario: Cancel load popup (hardware)
- **WHEN** the user presses B in SuperFW's GBA load popup
- **THEN** the firmware returns to the carousel with the same cart selected

### Requirement: Select escape hatch to stock UI
Select SHALL open SuperFW's stock UI (browser, recent, settings, tools, info tabs). A
documented key in the stock UI (B on the top-level tab, or Select) MUST return to the carousel.
No SuperFW feature may become unreachable.

#### Scenario: Reach stock settings (hardware)
- **WHEN** the user presses Select on the carousel
- **THEN** the stock SuperFW UI appears and its Settings and Info tabs work

#### Scenario: Return to carousel (hardware)
- **WHEN** the user presses the documented return key in the stock UI
- **THEN** the carousel reappears with the previous selection

### Requirement: Remember position
The carousel SHALL persist the last shelf and selected cart per shelf to
`/SYSTEM/SLOT/SETTINGS.TXT` before launching a game, and restore them at boot.

#### Scenario: Position restored (hardware)
- **WHEN** the user launches the 5th GB cart, then power-cycles and re-launches the fork
- **THEN** the carousel opens on the GB shelf with the 5th cart selected

### Requirement: SD card errors are visible
If the SD card fails to mount or `/Games` is unreadable, the carousel SHALL show a readable
error message and offer Select → stock UI, never hang on a blank screen.

#### Scenario: No Games folder (agent, preview)
- **WHEN** the stubbed library reports no `/Games` folder
- **THEN** the screen shows a "No Games folder" message and the Select hint

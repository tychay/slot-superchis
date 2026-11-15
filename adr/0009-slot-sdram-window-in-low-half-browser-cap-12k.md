# 0009. The slot SDRAM window lives in the 0x08 half; stock browser capped at 12,288 entries

- Status: accepted
- Date: 2026-11-15

## Context

ADR-0005 put slot memory in a reserved SDRAM window but did not fix where. design.md D5 proposed
`0x09800000–0x09DFFFFF`. The M1 SDRAM audit (design.md "SDRAM map (audited)") found that range
unusable:

- While the SD interface is mapped, which is the menu's normal state, the SuperCard SD registers
  shadow the whole `0x09xxxxxx` half: `0x09000000`, `0x09100000`, `0x09200000`, `0x09800000`, and
  the mode register at `0x09FFFFFE`.
- Stock SuperFW wraps every `0x09` access in `set_supercard_mode(..., false)`.
- `0x09D00000` holds the built-in patch DB.

The `0x08` half had no free 3 MiB gap: `t_sdram_state` (browser entries) runs up to the fonts.

## Decision

- The slot window is `0x08B80000–0x08E7FFFF` (3 MiB), directly below the fonts at `0x08E80000`.
  Its sub-regions are:
  - payload image at `0x08B80000` (256 KiB max);
  - mailbox at `0x08BC0000` (4 KiB);
  - payload statics at `0x08BC1000` (64 KiB);
  - scratch at `0x08BD1000` (about 2.69 MiB).
- The constants live in `src/slot/slot_layout.h` and are guarded by `_Static_assert`s in
  `src/common.h` and `src/menu.c`.
- To make room, `BROWSER_MAXFN_CNT` drops from 16,384 to 12,288 entries per folder, in one
  `// slot:` line in `src/config.h`.

Rejected alternative: use the free 4 MiB at `0x09800000–0x09BFFFFF`, with the FatFs wrappers
toggling the SD interface and copying every buffer outside `0x09xxxxxx`. It is easy to break
silently, and a mistake would corrupt SD I/O.

## Consequences

- The stock browser lists at most 12,288 entries per folder (documented in README). The
  carousel scan needs its own cap at or below that.
- Payload data passed to FatFs is never in `0x09xxxxxx`.
- Any upstream growth of `t_sdram_state` or the fonts area trips the static asserts. The window
  must then be re-planned under a new ADR.

# 0006. Adapt slot. advance code as attributed copies; submodule is read-only

- Status: accepted
- Date: 2026-11-14

## Context

slot. advance's `common/` is shipped as headers included into the DS Style kernel. It depends on
about 40 kernel globals and libgba-style names. It changes almost daily. Options considered:

- include it unmodified behind a compatibility layer that fakes every DS Style symbol;
- patch it at build time;
- copy the needed files into this repo and adapt them.

## Decision

- `vendor/slot-advance` is a git submodule pinned to a release tag (initially `v1.2.4`,
  `a2a2fe9`). It is never edited.
- It serves as reference, and as the asset source for `tools/slot_pack.py`.
- Needed files are copied into `src/slot/` and adapted to the host ABI (ADR-0004).
- Each copied file gets a header naming its source path, the pinned commit and its licence.
- `NOTICE.md` lists the copied files.

## Consequences

- Provenance stays clear, and nothing is faked to imitate DS Style internals.
- Pulling improvements from slot. advance is a deliberate manual re-sync: re-pin, diff the source
  files between the old and new commits, port the changes.
- Divergence from slot. advance grows over time. That is accepted.

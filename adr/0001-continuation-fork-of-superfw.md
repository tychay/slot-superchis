# 0001. Build as a continuation fork of SuperFW

- Status: accepted
- Date: 2026-11-14

## Context

The goal is a slot.-style cartridge carousel on the SuperChis Prime. The carousel has to read the
SD card, load ROMs into SDRAM, patch saves and start emulators. SuperFW (davidgfnet/superfw)
already does all of that, and on a Supercard the firmware *is* the launcher. Four shapes were
considered:

- (a) a git fork of SuperFW with full history;
- (b) a patch-file repo that applies diffs to pinned upstreams at build time, as slot. advance
  does;
- (c) contributing a `kernels/superfw` backend to slot. advance;
- (d) a separate GBA "launcher app" that SuperFW boots.

## Decision

The repository is a git fork of davidgfnet/superfw with full history.
- `upstream` remote → davidgfnet/superfw; work happens on `main`.
- Changes to SuperFW files are ordinary commits, kept small and marked `// slot:`.
- New code lives in `src/slot/`.
- The project says openly that it is a continuation of SuperFW.

Why the others lose:
- **(b):** agents would have to edit diff files, which is error-prone and hard to review.
- **(c):** ties progress to another maintainer and their Windows-centric tooling. It stays
  possible later.
- **(d):** SuperFW has no way to accept "boot this file" from another program, and in-game exit
  would land in stock SuperFW.

## Consequences

- Upstream fixes arrive via `git merge upstream/master`. Conflicts are limited to the hook sites.
- `git diff upstream/master -- src/ Makefile rom_boot.S` is the review surface for upstream impact
  and must stay small.
- An upstream PR (e.g. an optional frontend hook) stays feasible because the hooks are isolated.

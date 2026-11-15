# ADR Review Manifest

## ADR Review Completed

- Date: 2026-11-14
- Reviewer: Claude (Opus 5.5), for Terry Chay
- Change: add-slot-carousel

## In-Force ADR Context Reviewed

- None: no existing repository-level ADRs were present.

## Repository-Level ADRs Created

- adr/0001-continuation-fork-of-superfw.md - git fork of SuperFW with upstream remote; not a patch repo, slot. advance backend, or separate app
- adr/0002-sd-chainload-only-never-flash.md - deliver a chain-loaded `.gba`; never flash; flashing documented as untested/help-wanted
- adr/0003-docker-build-mirroring-upstream-ci.md - canonical build is `./build.sh` in a Docker image mirroring upstream CI
- adr/0004-carousel-as-sdram-payload-with-host-abi.md - carousel is a separately linked SDRAM payload behind a `slot_host_api` table
- adr/0005-ewram-reserved-for-stock-slot-memory-in-sdram.md - EWRAM ≤ 2 KB / IWRAM ≤ 8 KB additions; assets in a reserved SDRAM window; 2 MB image cap
- adr/0006-adapt-slot-advance-code-as-attributed-copies.md - submodule pinned read-only; adapted copies in `src/slot/` with provenance headers
- adr/0007-license-gpl-3-or-later.md - GPL-3.0-or-later with upstream notices retained
- adr/0008-on-card-layout-compatible-with-slot-advance.md - `/Games/*`, `/SYSTEM/SLOT` compatible with Cart Studio

- adr/0009-slot-sdram-window-in-low-half-browser-cap-12k.md - slot window at 0x08B80000–0x08E7FFFF (0x09 half is SD-register-shadowed); stock browser cap 16k→12k

## Notes

- design.md D3 (main-loop hooks), D4 (video save/restore), D6 (preview harness) and D9 (budget numbers) are implementation tactics. They are not ADRs; D9's caps are carried by ADR-0005.

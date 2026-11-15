# 0008. On-card layout is compatible with slot. advance

- Status: accepted
- Date: 2026-11-14

## Context

Users create label and shell art with slot. Cart Studio (a web app that writes to the SD root)
or with slot. advance's Label Converter. Both target slot. advance's on-card layout. A custom
layout would need its own art tooling.

## Decision

The fork uses slot. advance's paths and formats:
- library in `/Games/GBA`, `/Games/GB` and `/Games/GBC`;
- art in `/SYSTEM/SLOT/…`, in the format slot. advance reads;
- carousel settings in `/SYSTEM/SLOT/SETTINGS.TXT`.

SuperFW's own `SAVEGAME/` and `.superfw/` stay as they are.

## Consequences

- Cart Studio's "slot. advance" export works unchanged, so the project needs no art tooling of
  its own.
- A format change in slot. advance means a re-pin and a loader update (ADR-0006).
- SD cards can move between this fork and stock SuperFW without losing saves.

/*
 * slot. superchis: /SYSTEM/SLOT/SETTINGS.TXT parser and serializer (task 4.7, ADR-0008).
 * Pure functions (no FatFs), shared by the payload and tests/slot_settings_test.c.
 *
 * The file format is slot. advance's ("SlotOmega=3" snapshot, key=value lines, "End=1"), so the
 * same card works with Cart Studio / slot. advance.  M1 uses Platform, GBA, GB, GBC, Animations,
 * Wrap and Remember; every other key is preserved verbatim across a read/write cycle.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef SLOT_SETTINGS_H
#define SLOT_SETTINGS_H

#include "slot/slot_scan.h"

/* Key order is slot. advance's (Slot_config_keys[]). */
enum {
  SLOT_CFG_THEME, SLOT_CFG_ANIMATIONS, SLOT_CFG_WRAP, SLOT_CFG_SOUNDS, SLOT_CFG_REMEMBER,
  SLOT_CFG_ADDONS, SLOT_CFG_RESET, SLOT_CFG_STATES, SLOT_CFG_SLEEP, SLOT_CFG_PLATFORM,
  SLOT_CFG_GBA, SLOT_CFG_GB, SLOT_CFG_GBC, SLOT_CFG_SKEUO, SLOT_CFG_CLOCK,
  SLOT_CFG_RETURN_KEY, SLOT_CFG_MENU_KEYS,
  SLOT_CFG_COUNT
};

struct slot_settings {
  unsigned v[SLOT_CFG_COUNT];
};

/* Largest text slot_settings_format() can produce (include NUL). */
#define SLOT_SETTINGS_MAX_TEXT 256

void slot_settings_defaults(struct slot_settings *s);
/* Parse a whole file. A snapshot is applied only if it is complete and valid (header, known
 * keys in range, "End=1", all required keys present); otherwise *s is left at the defaults and
 * 0 is returned. */
int  slot_settings_parse(const char *text, unsigned len, struct slot_settings *s);
/* Serialize to `out` (cap bytes); returns the length (no NUL counted) or 0 if it did not fit. */
unsigned slot_settings_format(const struct slot_settings *s, char *out, unsigned cap);

#endif

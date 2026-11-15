/*
 * slot. superchis: SETTINGS.TXT parser/serializer (task 4.7).
 *
 * Provenance (ADR-0006): the format, key list, validation rules and defaults follow
 * Slot_ReadConfigFile()/Slot_SaveConfig() in vendor/slot-advance/common/slot_frontend.h (slot. advance
 * v1.2.4, commit a2a2fe9, GPL-3.0-or-later).  Rewritten to work on a memory buffer (no FatFs, no
 * snprintf) and to keep unknown-to-M1 keys intact.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "slot/slot_settings.h"

#include <string.h>

#include "slot/slot_format.h"
#include "slot/slot_hotkeys.h"

static const char *const keys[SLOT_CFG_COUNT] = {
  "Theme", "Animations", "Wrap", "Sounds", "Remember", "Addons", "Reset", "States", "Sleep",
  "Platform", "GBA", "GB", "GBC", "Skeuomorphic", "Clock", "ReturnKey", "MenuKeys",
};

void slot_settings_defaults(struct slot_settings *s) {
  static const unsigned defaults[SLOT_CFG_COUNT] = {
    0, 1, 1, 1, 1, 1, 1, 0, 0,          /* Theme..Sleep */
    0, 0, 0, 0,                         /* Platform, GBA, GB, GBC */
    1, 1, SLOT_DEFAULT_RETURN_KEY, SLOT_DEFAULT_MENU_KEYS,
  };
  memcpy(s->v, defaults, sizeof(defaults));
}

static int key_index(const char *name) {
  for (unsigned i = 0; i < SLOT_CFG_COUNT; i++)
    if (!strcmp(name, keys[i]))
      return (int)i;
  return -1;
}

static int value_ok(int k, unsigned value) {
  switch (k) {
  case SLOT_CFG_RETURN_KEY: return Slot_SingleKey(value);
  case SLOT_CFG_MENU_KEYS:  return Slot_MenuKeysValid(value);
  case SLOT_CFG_THEME: case SLOT_CFG_PLATFORM: return value < 3;
  case SLOT_CFG_GBA: case SLOT_CFG_GB: case SLOT_CFG_GBC: return value < SLOT_SHELF_MAX;
  default: return value < 2;
  }
}

int slot_settings_parse(const char *text, unsigned len, struct slot_settings *out) {
  struct slot_settings tmp;
  unsigned seen = 0, ended = 0, version = 0, pos = 0, first = 1;
  slot_settings_defaults(out);
  slot_settings_defaults(&tmp);
  while (pos < len) {
    char line[48];
    unsigned n = 0;
    while (pos < len && text[pos] != '\n') {
      if (n < sizeof(line) - 1)
        line[n++] = text[pos];
      else
        n = sizeof(line);                    /* over-long line: poison it */
      pos++;
    }
    pos++;                                   /* the '\n' */
    if (n >= sizeof(line))
      return 0;
    line[n] = 0;
    n = (unsigned)strcspn(line, "\r");
    line[n] = 0;
    if (first) {
      first = 0;
      version = !strcmp(line, "SlotOmega=1") ? 1 : !strcmp(line, "SlotOmega=2") ? 2 :
                !strcmp(line, "SlotOmega=3") ? 3 : 0;
      if (!version)
        return 0;
      continue;
    }
    char *eq = strchr(line, '=');
    if (!eq || line[0] == '#' || line[0] == ';')
      continue;
    *eq++ = 0;
    unsigned value;
    int k = key_index(line);
    if (k >= 0) {
      if (!Slot_ParseNatural(eq, &value) || !value_ok(k, value))
        return 0;
      seen |= 1u << k;
      tmp.v[k] = value;
    } else if (!strcmp(line, "End")) {
      if (!Slot_ParseNatural(eq, &value) || value != 1)
        return 0;
      ended = 1;
    }
  }
  /* Clock and hotkeys are optional in existing snapshots (13 keys in v1/v2, 14 in v3). */
  unsigned required = version < 3 ? (1u << 13) - 1 : (1u << 14) - 1;
  if (!ended || (seen & required) != required)
    return 0;
  for (unsigned i = 0; i < SLOT_CFG_COUNT; i++)
    if (seen & (1u << i))
      out->v[i] = tmp.v[i];
  if (version < 3)
    out->v[SLOT_CFG_SKEUO] = 1;
  return 1;
}

static unsigned put_str(char *out, unsigned cap, unsigned n, const char *s) {
  while (*s && n < cap)
    out[n++] = *s++;
  return n;
}

static unsigned put_uint(char *out, unsigned cap, unsigned n, unsigned v) {
  char tmp[11];
  unsigned t = 0;
  do { tmp[t++] = (char)('0' + v % 10); v /= 10; } while (v);
  while (t && n < cap)
    out[n++] = tmp[--t];
  return n;
}

unsigned slot_settings_format(const struct slot_settings *s, char *out, unsigned cap) {
  unsigned n = put_str(out, cap, 0, "SlotOmega=3\n");
  for (unsigned i = 0; i < SLOT_CFG_COUNT; i++) {
    n = put_str(out, cap, n, keys[i]);
    n = put_str(out, cap, n, "=");
    n = put_uint(out, cap, n, s->v[i]);
    n = put_str(out, cap, n, "\n");
  }
  n = put_str(out, cap, n, "End=1\n");
  if (n >= cap)
    return 0;
  out[n] = 0;
  return n;
}

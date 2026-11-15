/*
 * slot. superchis: host unit test for the SETTINGS.TXT parser/serializer (task 4.7).
 * Build and run: ./build.sh slot-tests
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "slot/slot_settings.h"
#include "slot/slot_format.h"
#include "slot/slot_hotkeys.h"

static int parse(const char *text, struct slot_settings *s) {
  return slot_settings_parse(text, (unsigned)strlen(text), s);
}

static void test_roundtrip(void) {
  struct slot_settings a, b;
  char buf[SLOT_SETTINGS_MAX_TEXT];
  slot_settings_defaults(&a);
  a.v[SLOT_CFG_PLATFORM] = 1;
  a.v[SLOT_CFG_GBA] = 2047;
  a.v[SLOT_CFG_GB] = 4;
  a.v[SLOT_CFG_GBC] = 0;
  a.v[SLOT_CFG_ANIMATIONS] = 0;
  a.v[SLOT_CFG_WRAP] = 0;
  a.v[SLOT_CFG_THEME] = 2;                           /* preserved even though M1 ignores it */
  a.v[SLOT_CFG_RETURN_KEY] = 512;
  a.v[SLOT_CFG_MENU_KEYS] = 0x300;
  unsigned n = slot_settings_format(&a, buf, sizeof(buf));
  assert(n && n == strlen(buf) && n < SLOT_SETTINGS_MAX_TEXT);
  assert(!strncmp(buf, "SlotOmega=3\n", 12) && !strcmp(buf + n - 6, "End=1\n"));
  assert(parse(buf, &b));
  assert(!memcmp(&a, &b, sizeof(a)));
  /* and again from the re-serialized text: byte-identical */
  char buf2[SLOT_SETTINGS_MAX_TEXT];
  assert(slot_settings_format(&b, buf2, sizeof(buf2)) == n && !strcmp(buf, buf2));
  puts("ok round trip (every key preserved, stable text)");
}

static void test_format_overflow(void) {
  struct slot_settings a;
  char buf[40];
  slot_settings_defaults(&a);
  assert(slot_settings_format(&a, buf, sizeof(buf)) == 0);   /* does not fit: refused, no overrun */
  puts("ok format refuses a too-small buffer");
}

static void test_slot_advance_file(void) {
  /* A file as slot. advance writes it (Slot_SaveConfig), with CRLF line endings and extra spaces. */
  const char *text =
    "SlotOmega=3\r\nTheme=1\r\nAnimations=0\r\nWrap=1\r\nSounds=1\r\nRemember=1\r\nAddons=0\r\nReset=1\r\n"
    "States=1\r\nSleep=0\r\nPlatform=2\r\nGBA=12\r\nGB=3\r\nGBC=40\r\nSkeuomorphic=1\r\nClock=0\r\n"
    "ReturnKey=4\r\nMenuKeys=12\r\nEnd=1\r\n";
  struct slot_settings s;
  assert(parse(text, &s));
  assert(s.v[SLOT_CFG_THEME] == 1 && s.v[SLOT_CFG_ANIMATIONS] == 0 && s.v[SLOT_CFG_PLATFORM] == 2);
  assert(s.v[SLOT_CFG_GBA] == 12 && s.v[SLOT_CFG_GB] == 3 && s.v[SLOT_CFG_GBC] == 40);
  assert(s.v[SLOT_CFG_ADDONS] == 0 && s.v[SLOT_CFG_CLOCK] == 0);
  /* version 1 files lack Skeuomorphic/Clock/hotkeys and get the defaults */
  const char *v1 =
    "SlotOmega=1\nTheme=0\nAnimations=1\nWrap=1\nSounds=1\nRemember=1\nAddons=1\nReset=1\nStates=0\n"
    "Sleep=0\nPlatform=1\nGBA=5\nGB=6\nGBC=7\nEnd=1\n";
  assert(parse(v1, &s));
  assert(s.v[SLOT_CFG_GB] == 6 && s.v[SLOT_CFG_SKEUO] == 1 && s.v[SLOT_CFG_CLOCK] == 1 &&
         s.v[SLOT_CFG_RETURN_KEY] == SLOT_DEFAULT_RETURN_KEY);
  puts("ok slot. advance style files (v3 with CRLF, v1 without optional keys)");
}

static void test_invalid_files(void) {
  struct slot_settings s;
  const char *good =
    "SlotOmega=3\nTheme=0\nAnimations=1\nWrap=1\nSounds=1\nRemember=1\nAddons=1\nReset=1\nStates=0\n"
    "Sleep=0\nPlatform=0\nGBA=1\nGB=0\nGBC=0\nSkeuomorphic=1\nEnd=1\n";
  assert(parse(good, &s) && s.v[SLOT_CFG_GBA] == 1);
  struct { const char *what, *text; } bad[] = {
    { "empty", "" },
    { "wrong header", "Slot=3\nEnd=1\n" },
    { "no End (truncated write)", "SlotOmega=3\nTheme=0\nAnimations=1\nWrap=1\nSounds=1\nRemember=1\nAddons=1\nReset=1\nStates=0\nSleep=0\nPlatform=0\nGBA=1\nGB=0\nGBC=0\nSkeuomorphic=1\n" },
    { "missing required key", "SlotOmega=3\nTheme=0\nEnd=1\n" },
    { "platform out of range", "SlotOmega=3\nTheme=0\nAnimations=1\nWrap=1\nSounds=1\nRemember=1\nAddons=1\nReset=1\nStates=0\nSleep=0\nPlatform=3\nGBA=1\nGB=0\nGBC=0\nSkeuomorphic=1\nEnd=1\n" },
    { "index out of range", "SlotOmega=3\nTheme=0\nAnimations=1\nWrap=1\nSounds=1\nRemember=1\nAddons=1\nReset=1\nStates=0\nSleep=0\nPlatform=0\nGBA=2048\nGB=0\nGBC=0\nSkeuomorphic=1\nEnd=1\n" },
    { "not a number", "SlotOmega=3\nTheme=0\nAnimations=x\nWrap=1\nSounds=1\nRemember=1\nAddons=1\nReset=1\nStates=0\nSleep=0\nPlatform=0\nGBA=1\nGB=0\nGBC=0\nSkeuomorphic=1\nEnd=1\n" },
    { "huge number", "SlotOmega=3\nTheme=99999999999999999999\nEnd=1\n" },
    { "bad hotkey", "SlotOmega=3\nTheme=0\nAnimations=1\nWrap=1\nSounds=1\nRemember=1\nAddons=1\nReset=1\nStates=0\nSleep=0\nPlatform=0\nGBA=1\nGB=0\nGBC=0\nSkeuomorphic=1\nClock=1\nReturnKey=3\nMenuKeys=12\nEnd=1\n" },
    { "over-long line", "SlotOmega=3\nTheme=0000000000000000000000000000000000000000000000000000000000000\nEnd=1\n" },
  };
  for (unsigned i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
    s.v[SLOT_CFG_GBA] = 77;
    assert(!parse(bad[i].text, &s));
    struct slot_settings d;
    slot_settings_defaults(&d);
    assert(!memcmp(&s, &d, sizeof(s)));                      /* an invalid file leaves the defaults */
  }
  puts("ok invalid files rejected, defaults kept");
}

static void test_natural(void) {
  unsigned v;
  assert(Slot_ParseNatural("42", &v) && v == 42);
  assert(Slot_ParseNatural(" 7 \r\n", &v) && v == 7);
  assert(!Slot_ParseNatural("", &v) && !Slot_ParseNatural("1x", &v) && !Slot_ParseNatural("-1", &v));
  assert(!Slot_ParseNatural("4294967296", &v));
  assert(Slot_IndexForCount(-1, 5, 1) == 4 && Slot_IndexForCount(5, 5, 1) == 0 && Slot_IndexForCount(-1, 5, 0) == -1);
  assert(Slot_IndexForCount(0, 0, 1) == -1);
  puts("ok Slot_ParseNatural / Slot_IndexForCount");
}

int main(void) {
  test_roundtrip();
  test_format_overflow();
  test_slot_advance_file();
  test_invalid_files();
  test_natural();
  puts("slot_settings_test: all passed");
  return 0;
}

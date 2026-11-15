/*
 * slot. superchis: host unit test for the carousel library scan (tasks 4.3 / 4.3b).
 * Build and run: ./build.sh slot-tests   (host gcc + AddressSanitizer in the build image).
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slot/slot_scan.h"

/* A fake directory: entries produced by a generator callback. */
struct fake {
  unsigned pos, total;
  int fail_at;                                    /* read error when pos reaches this (-1 = never) */
  void (*gen)(unsigned i, char *name, unsigned *flags);
};

static int next(void *ctx, char *name, uint32_t cap, uint32_t *size, unsigned *flags) {
  struct fake *f = ctx;
  (void)cap;
  if ((int)f->pos == f->fail_at) return -1;
  if (f->pos >= f->total) return 0;
  *flags = 0;
  f->gen(f->pos, name, flags);
  *size = 1000 + f->pos;
  f->pos++;
  return 1;
}

/* Shelf memory with guard bytes on both sides so any overflow is caught (and ASan sees it too). */
#define GUARD 64
struct arena { uint8_t *base; uint8_t *mem; uint32_t size; };
static struct arena arena_new(uint32_t size) {
  struct arena a;
  a.size = size;
  a.base = malloc(size + 2 * GUARD);
  assert(a.base);
  memset(a.base, 0xA5, size + 2 * GUARD);
  a.mem = a.base + GUARD;
  return a;
}
static void arena_check(const struct arena *a) {
  for (int i = 0; i < GUARD; i++) {
    assert(a->base[i] == 0xA5);
    assert(a->mem[a->size + i] == 0xA5);
  }
}

static const char *const spec_names[] = { "._x.gba", ".hidden.gba", "b.gba", "A.gba", "notes.txt" };
static void gen_spec(unsigned i, char *name, unsigned *flags) { (void)flags; strcpy(name, spec_names[i]); }

static void test_spec_scenario(void) {
  struct arena a = arena_new(SLOT_SHELF_BYTES);
  struct slot_shelf s;
  assert(slot_shelf_init(&s, a.mem, a.size));
  struct fake f = { 0, 5, -1, gen_spec };
  slot_scan(&s, SLOT_PLAT_GBA, 10, next, &f);
  assert(s.count == 2 && !s.truncated && !s.error && !s.skipped);
  assert(!strcmp(slot_shelf_name(&s, 0), "A.gba"));
  assert(!strcmp(slot_shelf_name(&s, 1), "b.gba"));
  assert(slot_shelf_size(&s, 0) == 1003);            /* A.gba was entry 3 */
  arena_check(&a);
  free(a.base);
  puts("ok spec scenario: A.gba, b.gba");
}

static void gen_mixed(unsigned i, char *name, unsigned *flags) {
  static const char *const n[] = { "x.GBA", "y.gb", "z.GBC", "Dir.gba", "sys.gba", "w.gbc", "noext", "v.agb", "u.gba.txt", "t.gB" };
  strcpy(name, n[i]);
  if (i == 3) *flags = SLOT_DE_DIR;
  if (i == 4) *flags = SLOT_DE_SYSTEM;
}
static void test_extensions_per_shelf(void) {
  struct arena a = arena_new(SLOT_SHELF_BYTES);
  struct slot_shelf s;
  assert(slot_shelf_init(&s, a.mem, a.size));
  struct fake f = { 0, 10, -1, gen_mixed };
  slot_scan(&s, SLOT_PLAT_GBA, 10, next, &f);
  assert(s.count == 1 && !strcmp(slot_shelf_name(&s, 0), "x.GBA"));
  f.pos = 0; slot_scan(&s, SLOT_PLAT_GB, 9, next, &f);
  assert(s.count == 2 && !strcmp(slot_shelf_name(&s, 0), "t.gB") && !strcmp(slot_shelf_name(&s, 1), "y.gb"));
  f.pos = 0; slot_scan(&s, SLOT_PLAT_GBC, 10, next, &f);
  assert(s.count == 2 && !strcmp(slot_shelf_name(&s, 0), "w.gbc") && !strcmp(slot_shelf_name(&s, 1), "z.GBC"));
  arena_check(&a);
  free(a.base);
  puts("ok per-shelf extensions, dirs and system files rejected");
}

static void gen_case(unsigned i, char *name, unsigned *flags) {
  static const char *const n[] = { "beta.gba", "Alpha.gba", "alpha.gba", "ALPHA2.gba", "Zeta.gba", "_under.gba", "10.gba", "9.gba" };
  (void)flags; strcpy(name, n[i]);
}
static void test_case_insensitive_sort(void) {
  struct arena a = arena_new(SLOT_SHELF_BYTES);
  struct slot_shelf s;
  assert(slot_shelf_init(&s, a.mem, a.size));
  struct fake f = { 0, 8, -1, gen_case };
  slot_scan(&s, SLOT_PLAT_GBA, 10, next, &f);
  const char *want[] = { "10.gba", "9.gba", "_under.gba", "Alpha.gba", "alpha.gba", "ALPHA2.gba", "beta.gba", "Zeta.gba" };
  assert(s.count == 8);
  for (unsigned i = 0; i < 8; i++) assert(!strcmp(slot_shelf_name(&s, i), want[i]));
  arena_check(&a);
  free(a.base);
  puts("ok case-insensitive sort (ties in byte order)");
}

static void test_empty_and_error(void) {
  struct arena a = arena_new(SLOT_SHELF_BYTES);
  struct slot_shelf s;
  assert(slot_shelf_init(&s, a.mem, a.size));
  struct fake f = { 0, 0, -1, gen_spec };
  slot_scan(&s, SLOT_PLAT_GBA, 10, next, &f);
  assert(s.count == 0 && !s.error);
  struct fake g = { 0, 5, 3, gen_spec };            /* read error after 3 entries: keep what was read */
  slot_scan(&s, SLOT_PLAT_GBA, 10, next, &g);
  assert(s.error && s.count == 1 && !strcmp(slot_shelf_name(&s, 0), "b.gba"));
  assert(!slot_shelf_init(&s, a.mem, 100));         /* too small memory is refused, not overrun */
  free(a.base);
  puts("ok empty folder and read error");
}

/* 4.3b: 5,000 entries, short names -> capped at SLOT_SHELF_MAX with a truncation flag. */
static void gen_many_short(unsigned i, char *name, unsigned *flags) {
  (void)flags;
  sprintf(name, "Game %05u.gba", (i * 7919u) % 5000u);
}
static void test_5000_entries(void) {
  struct arena a = arena_new(SLOT_SHELF_BYTES);
  struct slot_shelf s;
  assert(slot_shelf_init(&s, a.mem, a.size));
  struct fake f = { 0, 5000, -1, gen_many_short };
  slot_scan(&s, SLOT_PLAT_GBA, 10, next, &f);
  assert(s.count == SLOT_SHELF_MAX && s.truncated);
  assert(f.pos == SLOT_SHELF_MAX + 1);              /* it stopped reading at the cap */
  for (unsigned i = 1; i < s.count; i++)
    assert(slot_name_cmp(slot_shelf_name(&s, i - 1), slot_shelf_name(&s, i)) < 0);
  arena_check(&a);
  free(a.base);
  printf("ok 5,000 short entries: kept %u, truncated, sorted, no overflow\n", SLOT_SHELF_MAX);
}

/* 4.3b: 5,000 entries with 255-byte names. Every one is too long for the 255-char launch path:
   none are kept, all are counted as skipped, and nothing overflows. */
static void gen_255(unsigned i, char *name, unsigned *flags) {
  (void)flags;
  memset(name, 'a' + (i % 26), 251);
  sprintf(name + 246, "%05u.gba", i);               /* 246 + 5 + 4 = 255 bytes */
  assert(strlen(name) == 255);
}
static void test_5000_entries_255_byte_names(void) {
  struct arena a = arena_new(SLOT_SHELF_BYTES);
  struct slot_shelf s;
  assert(slot_shelf_init(&s, a.mem, a.size));
  struct fake f = { 0, 5000, -1, gen_255 };
  slot_scan(&s, SLOT_PLAT_GBA, 10, next, &f);
  assert(s.count == 0 && s.skipped == 5000 && !s.truncated);
  arena_check(&a);
  free(a.base);
  puts("ok 5,000 x 255-byte names: all skipped (path would not fit), no overflow");
}

/* 244-byte names are the longest that still fit "/Games/GBA/<name>" in 255 chars; 5,000 of
   them overflow the name pool long before the entry cap: stop with the truncation flag. */
static void gen_244(unsigned i, char *name, unsigned *flags) {
  (void)flags;
  memset(name, 'a' + (i % 26), 235);
  sprintf(name + 235, "%05u.gba", i);               /* 235 + 5 + 4 = 244 bytes */
}
static void test_5000_entries_244_byte_names(void) {
  struct arena a = arena_new(SLOT_SHELF_BYTES);
  struct slot_shelf s;
  assert(slot_shelf_init(&s, a.mem, a.size));
  struct fake f = { 0, 5000, -1, gen_244 };
  slot_scan(&s, SLOT_PLAT_GBA, 10, next, &f);
  assert(s.count > 1000 && s.count < SLOT_SHELF_MAX && s.truncated && s.skipped == 0);
  assert(s.pool_used <= s.pool_cap);
  for (unsigned i = 1; i < s.count; i++)
    assert(slot_name_cmp(slot_shelf_name(&s, i - 1), slot_shelf_name(&s, i)) < 0);
  arena_check(&a);
  /* a tiny shelf memory (the preview's EWRAM) truncates earlier but just as safely */
  struct arena b = arena_new(SLOT_SHELF_INDEX_BYTES + 5000);
  struct slot_shelf t;
  assert(slot_shelf_init(&t, b.mem, b.size));
  f.pos = 0;
  slot_scan(&t, SLOT_PLAT_GBA, 10, next, &f);
  assert(t.truncated && t.count == 5000 / 245);
  arena_check(&b);
  printf("ok 5,000 x 244-byte names: kept %u of 5000 (name pool full), truncated, no overflow\n", s.count);
  free(a.base);
  free(b.base);
}

/* A mix: UTF-8 names, an over-long path and a hostile reader that never terminates its name. */
static void gen_utf8(unsigned i, char *name, unsigned *flags) {
  (void)flags;
  static const char *const n[] = { "Pok\xC3\xA9mon Rouge.gba", "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.gba", "caf\xC3\xA9.gba", "plain.gba" };
  strcpy(name, n[i]);
}
static void test_utf8_and_titles(void) {
  struct arena a = arena_new(SLOT_SHELF_BYTES);
  struct slot_shelf s;
  assert(slot_shelf_init(&s, a.mem, a.size));
  struct fake f = { 0, 4, -1, gen_utf8 };
  slot_scan(&s, SLOT_PLAT_GBA, 10, next, &f);
  assert(s.count == 4);
  char t[SLOT_NAME_MAX];
  slot_title_from_name("Pok\xC3\xA9mon Rouge.gba", t, sizeof(t));
  assert(!strcmp(t, "Pok?mon Rouge"));
  slot_title_from_name("\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E.gba", t, sizeof(t));
  assert(!strcmp(t, "???"));
  slot_title_from_name("a\tb\x80" "c.GB", t, sizeof(t));
  assert(!strcmp(t, "a?b?c"));
  slot_title_from_name(".gba", t, sizeof(t));               /* a leading dot is not an extension */
  assert(!strcmp(t, ".gba"));
  slot_title_from_name("noext", t, sizeof(t));
  assert(!strcmp(t, "noext"));
  slot_title_from_name("x.y.z.gba", t, 4);                  /* cut to the output size */
  assert(!strcmp(t, "x.y"));
  char big[SLOT_NAME_MAX];
  memset(big, 'q', 255); strcpy(big + 251, ".gba");
  slot_title_from_name(big, t, sizeof(t));
  assert(strlen(t) == 251);
  char e[SLOT_NAME_MAX];
  slot_ellipsize(t, e, 38);
  assert(strlen(e) == 38 && !strcmp(e + 36, ".."));
  slot_ellipsize("short", e, 38);
  assert(!strcmp(e, "short"));
  arena_check(&a);
  free(a.base);
  puts("ok UTF-8 and title helpers");
}

static void gen_unterminated(unsigned i, char *name, unsigned *flags) {
  (void)flags; (void)i;
  memset(name, 'x', SLOT_NAME_MAX);                       /* no NUL at all */
}
static void test_hostile_reader(void) {
  struct arena a = arena_new(SLOT_SHELF_BYTES);
  struct slot_shelf s;
  assert(slot_shelf_init(&s, a.mem, a.size));
  struct fake f = { 0, 3, -1, gen_unterminated };
  slot_scan(&s, SLOT_PLAT_GBA, 10, next, &f);              /* must not read past the name buffer */
  assert(s.count == 0);
  arena_check(&a);
  free(a.base);
  puts("ok unterminated name from the reader");
}

int main(void) {
  test_spec_scenario();
  test_extensions_per_shelf();
  test_case_insensitive_sort();
  test_empty_and_error();
  test_5000_entries();
  test_5000_entries_255_byte_names();
  test_5000_entries_244_byte_names();
  test_utf8_and_titles();
  test_hostile_reader();
  puts("slot_scan_test: all passed");
  return 0;
}

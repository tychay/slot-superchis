/*
 * slot. superchis: library scan, filter and sort (tasks 4.3, 4.3b).
 *
 * Provenance (ADR-0006): the filter rules and sort order follow Slot_Scan()/Slot_SelectDirectory()
 * in vendor/slot-advance/common/slot_frontend.h (slot. advance v1.2.4, commit a2a2fe9,
 * GPL-3.0-or-later), rewritten as pure functions with fixed, checked storage instead of the DS Style
 * file-record store.  Changes: only .gba/.gb/.gbc (spec), a shelf cap with a truncation flag,
 * path-length check, bounded heapsort.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "slot/slot_scan.h"
#include "slot/slot_sdram.h"

#include <string.h>

static unsigned fold(unsigned char c) {
  return (c >= 'A' && c <= 'Z') ? c + 32u : c;
}

int SLOT_HOT slot_name_cmp(const char *a, const char *b) {
  const unsigned char *x = (const unsigned char *)a, *y = (const unsigned char *)b;
  for (; *x && *y; x++, y++) {
    unsigned fx = fold(*x), fy = fold(*y);
    if (fx != fy)
      return fx < fy ? -1 : 1;
  }
  if (*x != *y)
    return *x ? 1 : -1;
  return strcmp(a, b);            /* same text ignoring case: plain order, so the sort is total */
}

static int ext_is(const char *ext, const char *want) {
  for (; *ext && *want; ext++, want++)
    if (fold((unsigned char)*ext) != (unsigned)*want)
      return 0;
  return !*ext && !*want;
}

int slot_name_accept(const char *name, unsigned flags, unsigned platform) {
  if (flags & (SLOT_DE_DIR | SLOT_DE_HIDDEN | SLOT_DE_SYSTEM))
    return 0;
  if (name[0] == '.')              /* hidden files and macOS AppleDouble "._x.gba" */
    return 0;
  const char *ext = strrchr(name, '.');
  if (!ext)
    return 0;
  switch (platform) {
  case SLOT_PLAT_GBA: return ext_is(ext, ".gba");
  case SLOT_PLAT_GB:  return ext_is(ext, ".gb");
  case SLOT_PLAT_GBC: return ext_is(ext, ".gbc");
  }
  return 0;
}

int slot_shelf_init(struct slot_shelf *s, void *mem, uint32_t mem_size) {
  const uint32_t min_pool = 4096;
  if (mem_size < SLOT_SHELF_INDEX_BYTES + min_pool || ((uintptr_t)mem & 3))
    return 0;
  uint8_t *p = (uint8_t *)mem;
  s->name_off = (uint32_t *)p;  p += SLOT_SHELF_MAX * 4u;
  s->size     = (uint32_t *)p;  p += SLOT_SHELF_MAX * 4u;
  s->order    = (uint16_t *)p;  p += SLOT_SHELF_MAX * 2u;
  s->pool     = (char *)p;
  s->pool_cap = mem_size - SLOT_SHELF_INDEX_BYTES;
  if (s->pool_cap > SLOT_POOL_MAX)
    s->pool_cap = SLOT_POOL_MAX;
  slot_shelf_clear(s);
  return 1;
}

void slot_shelf_clear(struct slot_shelf *s) {
  s->count = 0;
  s->skipped = 0;
  s->truncated = 0;
  s->error = 0;
  s->pool_used = 0;
}

/* In-place heapsort of the display order (no recursion, no allocation, O(n log n)). */
static int SLOT_HOT less(const struct slot_shelf *s, uint16_t a, uint16_t b) {
  return slot_name_cmp(s->pool + s->name_off[a], s->pool + s->name_off[b]) < 0;
}

static void SLOT_HOT sift_down(struct slot_shelf *s, uint32_t root, uint32_t end) {
  uint16_t *o = s->order;
  for (;;) {
    uint32_t child = 2 * root + 1;
    if (child >= end)
      return;
    if (child + 1 < end && less(s, o[child], o[child + 1]))
      child++;
    if (!less(s, o[root], o[child]))
      return;
    uint16_t t = o[root]; o[root] = o[child]; o[child] = t;
    root = child;
  }
}

static void sort_shelf(struct slot_shelf *s) {
  uint32_t n = s->count;
  for (uint32_t i = 0; i < n; i++)
    s->order[i] = (uint16_t)i;
  if (n < 2)
    return;
  for (uint32_t i = n / 2; i-- > 0; )
    sift_down(s, i, n);
  for (uint32_t end = n - 1; end > 0; end--) {
    uint16_t t = s->order[0]; s->order[0] = s->order[end]; s->order[end] = t;
    sift_down(s, 0, end);
  }
}

void slot_scan(struct slot_shelf *s, unsigned platform, unsigned dir_len, slot_next_fn next, void *ctx) {
  char name[SLOT_NAME_MAX];
  slot_shelf_clear(s);
  for (;;) {
    uint32_t size = 0;
    unsigned flags = 0;
    int r = next(ctx, name, sizeof(name), &size, &flags);
    if (r < 0) { s->error = 1; break; }
    if (r == 0) break;
    name[SLOT_NAME_MAX - 1] = 0;                 /* never trust the reader's terminator */
    if (!slot_name_accept(name, flags, platform))
      continue;
    uint32_t len = (uint32_t)strlen(name);
    if (dir_len + 1 + len >= SLOT_PATH_LIMIT) {   /* "<dir>/<name>" must fit the launch path */
      s->skipped++;
      continue;
    }
    uint32_t stored = (len + 2) & ~1u;           /* NUL + pad to even: pool takes 16-bit stores only (ADR-0010) */
    if (s->count >= SLOT_SHELF_MAX || s->pool_used + stored > s->pool_cap) {
      s->truncated = 1;                           /* cap reached: stop, do not read the rest */
      break;
    }
    name[len + 1] = 0;                           /* len < SLOT_PATH_LIMIT-1, so inside `name` */
    slot_sdram_store(s->pool + s->pool_used, name, stored);
    s->name_off[s->count] = s->pool_used;
    s->size[s->count] = size;
    s->pool_used += stored;
    s->count++;
  }
  sort_shelf(s);
}

void slot_title_from_name(const char *name, char *out, unsigned out_cap) {
  unsigned n = 0, len = (unsigned)strlen(name), end = len;
  const char *dot = strrchr(name, '.');
  if (dot && dot != name)
    end = (unsigned)(dot - name);
  for (unsigned i = 0; i < end && n + 1 < out_cap; ) {
    unsigned char c = (unsigned char)name[i];
    if (c >= 0xC0) {                              /* UTF-8 lead byte: swallow its continuation bytes */
      i++;
      while (i < end && ((unsigned char)name[i] & 0xC0) == 0x80)
        i++;
      out[n++] = '?';
    } else {
      out[n++] = (c < 32 || c > 126) ? '?' : (char)c;
      i++;
    }
  }
  if (out_cap)
    out[n] = 0;
}

void slot_ellipsize(const char *s, char *out, unsigned cols) {
  unsigned len = (unsigned)strlen(s);
  if (len <= cols) {
    memcpy(out, s, len + 1);
    return;
  }
  memcpy(out, s, cols - 2);
  out[cols - 2] = '.';
  out[cols - 1] = '.';
  out[cols] = 0;
}

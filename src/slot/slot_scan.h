/*
 * slot. superchis: library scan, filter and sort for the cartridge shelves (tasks 4.3, 4.3b).
 * Pure functions: no FatFs, no hardware; the directory is read through a callback so the same
 * code runs in the payload and in the host unit tests (tests/slot_scan_test.c).
 *
 * Memory is bounded by construction: at most SLOT_SHELF_MAX entries and at most pool_cap bytes
 * of names are kept per shelf; scanning stops at either limit and flags the shelf as truncated.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef SLOT_SCAN_H
#define SLOT_SCAN_H

#include <stdint.h>

/* The sort comparison is the carousel's hottest non-pixel loop: on the console the payload puts
 * it in its IWRAM section (ARM code, ADR-0005 budget).  Host tests compile it as plain C. */
#if defined(__arm__) && !defined(SLOT_NO_IWRAM)
#define SLOT_HOT __attribute__((section(".iwram_code"), target("arm"), long_call, noinline, optimize("O2")))
#else
#define SLOT_HOT
#endif

#define SLOT_SHELF_MAX      2048u          /* carts kept per shelf (spec: large libraries never crash) */
#define SLOT_NAME_MAX       256u           /* name buffer incl. NUL; == FILINFO.fname == MAX_FN_LEN */
#define SLOT_PATH_LIMIT     256u           /* launch path buffer incl. NUL (struct slot_mailbox) */
#define SLOT_POOL_MAX       (256u * 1024u) /* name bytes kept per shelf */
/* Bytes of the three index arrays carved from a shelf's memory (order, name_off, size). */
#define SLOT_SHELF_INDEX_BYTES (SLOT_SHELF_MAX * (2u + 4u + 4u))
/* Worst case memory for one shelf. */
#define SLOT_SHELF_BYTES    (SLOT_SHELF_INDEX_BYTES + SLOT_POOL_MAX)

enum { SLOT_PLAT_GBA = 0, SLOT_PLAT_GB = 1, SLOT_PLAT_GBC = 2, SLOT_PLAT_COUNT = 3 };

/* flags reported by a directory reader */
#define SLOT_DE_DIR         1u
#define SLOT_DE_HIDDEN      2u
#define SLOT_DE_SYSTEM      4u

struct slot_shelf {
  uint32_t count;           /* carts kept */
  uint32_t skipped;         /* matching files not shown: launch path would not fit */
  uint8_t  truncated;       /* stopped at SLOT_SHELF_MAX / pool capacity: more files exist */
  uint8_t  error;           /* the directory reader reported an error (list is partial) */
  uint16_t *order;          /* [SLOT_SHELF_MAX] display position -> entry number */
  uint32_t *name_off;       /* [SLOT_SHELF_MAX] entry number -> offset in pool */
  uint32_t *size;           /* [SLOT_SHELF_MAX] entry number -> file size */
  char     *pool;           /* names, each NUL padded to an even length; may be SDRAM (16-bit stores only) */
  uint32_t pool_cap, pool_used;
};

/* Next directory entry: copy the (NUL terminated, < name_cap bytes) file name to `name`.
 * Returns 1 = entry, 0 = end of directory, < 0 = read error. */
typedef int (*slot_next_fn)(void *ctx, char *name, uint32_t name_cap, uint32_t *size, unsigned *flags);

/* Carve a shelf out of `mem` (4-byte aligned). Returns 0 if mem_size is too small. The pool is
 * capped at SLOT_POOL_MAX. */
int  slot_shelf_init(struct slot_shelf *s, void *mem, uint32_t mem_size);
void slot_shelf_clear(struct slot_shelf *s);

/* Does `name` belong on the shelf of `platform`? (.gba / .gb / .gbc, case-insensitive; hidden,
 * system, directories and names starting with '.' -- including macOS "._x" -- are rejected) */
int  slot_name_accept(const char *name, unsigned flags, unsigned platform);

/* Case-insensitive (ASCII) name order; ties broken by plain byte order for determinism. */
int  slot_name_cmp(const char *a, const char *b) SLOT_HOT;

/* Fill and sort the shelf. dir_len = strlen of the directory path (for the path length limit). */
void slot_scan(struct slot_shelf *s, unsigned platform, unsigned dir_len, slot_next_fn next, void *ctx);

static inline const char *slot_shelf_name(const struct slot_shelf *s, unsigned pos) {
  return s->pool + s->name_off[s->order[pos]];
}
static inline uint32_t slot_shelf_size(const struct slot_shelf *s, unsigned pos) {
  return s->size[s->order[pos]];
}

/* Display helpers. All outputs are NUL terminated and ASCII only. */
/* File name -> title: drop the extension, one '?' per non-ASCII UTF-8 sequence or control byte.
 * out_cap must be > 0; the result is cut to out_cap-1 characters. */
void slot_title_from_name(const char *name, char *out, unsigned out_cap);
/* Shorten `s` to at most `cols` characters, ending in ".." when cut (cols >= 3). */
void slot_ellipsize(const char *s, char *out, unsigned cols);

#endif

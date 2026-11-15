/*
 * slot. superchis: the built-in art the carousel draws.  Since M2 (task 6.1) it lives in the asset
 * pack res/slot_assets.pack (tools/slot_pack.py): apultra-compressed in the image, unpacked by
 * rom_boot.S into the pack region of the SDRAM window (SLOT_PACK_ADDR, ADR-0011) and read from
 * there.  The preview ROM links the uncompressed pack into its ROM instead (slot_pack_blob).
 * The pack is read-only for the payload (reads of any width are fine, nothing is written).
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef SLOT_ART_H
#define SLOT_ART_H
#include <stdint.h>
#include "slot/slot_layout.h"

#define SLOT_PACK_MAGIC 0x4B504C53u   /* "SLPK" */

/* item ids == the order of `items` in tools/slot_pack.py */
enum {
  SLOT_PK_BACKGROUND,                 /* wood, 240x160 RGB555 */
  SLOT_PK_BACK_DARK, SLOT_PK_FRONT_DARK, SLOT_PK_BACK_LIGHT, SLOT_PK_FRONT_LIGHT,
  SLOT_PK_BACK_FLAT_DARK, SLOT_PK_FRONT_FLAT_DARK, SLOT_PK_BACK_FLAT_LIGHT, SLOT_PK_FRONT_FLAT_LIGHT,
  SLOT_PK_MOULD,                      /* +platform (GBA, GB, GBC): plain skeuomorphic cartridge, 0x8000 = key */
  SLOT_PK_MOULD_GB = SLOT_PK_MOULD + 1, SLOT_PK_MOULD_GBC,
  SLOT_PK_FLAT,                       /* +platform: flat cartridge */
  SLOT_PK_FLAT_GB = SLOT_PK_FLAT + 1, SLOT_PK_FLAT_GBC,
  SLOT_PK_FONT,                       /* 128 x 12 bytes, 6x12 cell, MSB = leftmost column */
  SLOT_PK_SPLASH,                     /* (count, pixel) u16 pairs, 240x160 */
  SLOT_PK_INSERT,                     /* signed 8-bit mono, whole frames, zero padded */
  SLOT_PK_COUNT
};

struct slot_pack {
  uint32_t magic, version, count, total_size;
  uint32_t insert_samples, samples_per_frame, timer_period, reserved;
  struct { uint32_t offset, size; } item[SLOT_PK_COUNT];
};

#ifdef SLOT_PREVIEW
extern const struct slot_pack slot_pack_blob;
#define SLOT_PACK ((const struct slot_pack *)&slot_pack_blob)
#else
#define SLOT_PACK ((const struct slot_pack *)SLOT_PACK_ADDR)
#endif

static inline int slot_pack_valid(void) {
  const struct slot_pack *p = SLOT_PACK;
  return p->magic == SLOT_PACK_MAGIC && p->version == 1 && p->count == SLOT_PK_COUNT &&
         p->item[SLOT_PK_INSERT].offset + p->item[SLOT_PK_INSERT].size <= p->total_size;
}
static inline const void *slot_pack_item(unsigned id) {
  return (const uint8_t *)SLOT_PACK + SLOT_PACK->item[id].offset;
}
static inline const uint16_t *slot_pack_u16(unsigned id) { return (const uint16_t *)slot_pack_item(id); }

#endif

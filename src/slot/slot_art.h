/*
 * slot. superchis: the built-in art the M1 carousel uses (task 4.2).  The arrays are generated
 * at build time from the pinned submodule by tools/slot_assets_gen.py (src/slot/slot_art_gen.c);
 * they live in the payload's SDRAM image (ADR-0005), never in EWRAM.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef SLOT_ART_H
#define SLOT_ART_H
#include <stdint.h>

extern const uint16_t slot_background[240 * 160];   /* wood, RGB555 (bit 15 clear) */
extern const uint16_t slot_back_dark[240 * 19];     /* 19-row bands drawn at y=141; 0x8000 = key */
extern const uint16_t slot_front_dark[240 * 19];
extern const uint16_t slot_fallback_gba[120 * 71];  /* plain cartridge silhouettes, 0x8000 = key */
extern const uint16_t slot_fallback_gb[104 * 112];
extern const uint16_t slot_fallback_gbc[104 * 112];
extern const unsigned char slot_font[128][12];      /* 6x12 cell, MSB = leftmost column */
extern const uint16_t slot_splash_rle[];            /* (count, pixel) pairs, 240x160 */
extern const unsigned slot_splash_rle_len;          /* in u16 units */

#endif

/*
 * slot. superchis: byte-granular copy into SuperCard SDRAM using 16-bit stores only (ADR-0010).
 * The SDRAM sits on the 16-bit cartridge bus: an 8-bit store (strb) corrupts the neighbouring
 * byte.  Build byte data (names, paths) in IWRAM or on the stack, then copy it here.
 * Reads from SDRAM may be any width.  Header-only so host unit tests can exercise it.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef SLOT_SDRAM_H
#define SLOT_SDRAM_H

#include <stdint.h>

/* Copy n bytes from src (any memory) to dst (SDRAM, any alignment) with aligned halfword
 * read-modify-write stores only.  Little-endian, like the GBA. */
static inline void slot_sdram_store(void *dst, const void *src, uint32_t n) {
  const uint8_t *s = (const uint8_t *)src;
  volatile uint16_t *h = (volatile uint16_t *)((uintptr_t)dst & ~(uintptr_t)1);
  if (n && ((uintptr_t)dst & 1)) {          /* odd start: keep the low byte, replace the high one */
    *h = (uint16_t)((*h & 0x00FFu) | ((uint16_t)s[0] << 8));
    h++; s++; n--;
  }
  for (; n >= 2; n -= 2, s += 2, h++)
    *h = (uint16_t)(s[0] | ((uint16_t)s[1] << 8));
  if (n)                                    /* odd end: replace the low byte, keep the high one */
    *h = (uint16_t)((*h & 0xFF00u) | s[0]);
}

#endif

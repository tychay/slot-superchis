/*
 * slot. superchis: hardware diagnostic build (make SLOT_DIAG=1). Not part of normal builds.
 * Paints 14x14 blocks straight into Mode 3 VRAM so a hang leaves a readable trail:
 *   row 0 (y=8):  boot steps, grey = "about to run", green = "returned".
 *   row 1 (y=40): SDRAM self-tests, grey = running, green = pass, red = fail.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef SLOT_DIAG_H
#define SLOT_DIAG_H
#ifdef SLOT_DIAG
#include "slot/slot_layout.h"

#define DIAG_GREY  0x4210u
#define DIAG_GREEN 0x03E0u
#define DIAG_RED   0x001Fu
#define DIAG_BLUE  0x7C00u

static void Slot_DiagBlock(unsigned row, unsigned n, uint16_t c) {
  volatile uint16_t *v = (volatile uint16_t *)0x06000000;
  unsigned x0 = 4 + n * 16, y0 = 8 + row * 32, x, y;
  for (y = y0; y < y0 + 14; y++)
    for (x = x0; x < x0 + 14 && x < 240; x++)
      v[y * 240 + x] = c;
}
#define SLOT_DIAG_BEGIN(n) Slot_DiagBlock(0, (n), DIAG_GREY)
#define SLOT_DIAG_END(n)   Slot_DiagBlock(0, (n), DIAG_GREEN)

static void Slot_DiagResult(unsigned n, int ok) { Slot_DiagBlock(1, n, ok ? DIAG_GREEN : DIAG_RED); }

/* SDRAM self-test at the start of scratch and near the window end. */
static void Slot_DiagSelfTest(const uint16_t *rodata_src) {
  volatile uint32_t *w = (volatile uint32_t *)SLOT_SCRATCH_ADDR;
  volatile uint16_t *h = (volatile uint16_t *)SLOT_SCRATCH_ADDR;
  volatile uint8_t  *b = (volatile uint8_t  *)SLOT_SCRATCH_ADDR;
  volatile uint32_t *end = (volatile uint32_t *)(SLOT_SCRATCH_ADDR + SLOT_SCRATCH_SIZE - 16);
  int ok;
  unsigned i;

  /* 0: 32-bit write/read */
  Slot_DiagBlock(1, 0, DIAG_GREY);
  w[0] = 0x12345678u; w[1] = 0xCAFEBABEu;
  Slot_DiagResult(0, w[0] == 0x12345678u && w[1] == 0xCAFEBABEu);

  /* 1: 16-bit write/read */
  Slot_DiagBlock(1, 1, DIAG_GREY);
  h[0] = 0xA55Au; h[1] = 0x5AA5u;
  Slot_DiagResult(1, h[0] == 0xA55Au && h[1] == 0x5AA5u);

  /* 2: 8-bit store; red = byte stores corrupt neighbours (expected on SuperCard) */
  Slot_DiagBlock(1, 2, DIAG_GREY);
  w[0] = 0x11223344u;
  b[1] = 0xEE;
  Slot_DiagResult(2, w[0] == 0x1122EE44u);

  /* 3: CPU write/read near the end of the window */
  Slot_DiagBlock(1, 3, DIAG_GREY);
  end[0] = 0xDEADBEEFu;
  Slot_DiagResult(3, end[0] == 0xDEADBEEFu);

  /* 4: CPU copy (Slot_CopyMem), payload rodata (SDRAM) -> scratch (SDRAM), 64 halfwords */
  Slot_DiagBlock(1, 4, DIAG_GREY);
  Slot_Copy((void *)SLOT_SCRATCH_ADDR, rodata_src, 128);
  ok = 1;
  for (i = 0; i < 64; i++) if (h[i] != rodata_src[i]) ok = 0;
  Slot_DiagResult(4, ok);

  /* 5: CPU copy, full-screen-sized SDRAM -> SDRAM (like Slot_Background) */
  Slot_DiagBlock(1, 5, DIAG_GREY);
  Slot_Copy((void *)SLOT_SCRATCH_ADDR, rodata_src, 240 * 160 * 2);
  Slot_DiagResult(5, h[100] == rodata_src[100] && h[38000] == rodata_src[38000]);

  /* 6: CPU copy, scratch (SDRAM) -> VRAM rows 120..135 (shows background pixels) */
  Slot_DiagBlock(1, 6, DIAG_GREY);
  Slot_Copy((void *)(0x06000000 + 120 * 240 * 2), (const void *)SLOT_SCRATCH_ADDR, 16 * 240 * 2);
  Slot_DiagResult(6, ((volatile uint16_t *)0x06000000)[120 * 240 + 5] == h[5]);

  /* 7: CPU loop write of 76.8 KB into scratch (like a full back-buffer redraw) */
  Slot_DiagBlock(1, 7, DIAG_GREY);
  for (i = 0; i < 240 * 160; i++) h[i] = (uint16_t)i;
  Slot_DiagResult(7, h[1234] == 1234 && h[38399] == (uint16_t)38399);
}
#else
#define SLOT_DIAG_BEGIN(n) ((void)0)
#define SLOT_DIAG_END(n)   ((void)0)
#endif
#endif

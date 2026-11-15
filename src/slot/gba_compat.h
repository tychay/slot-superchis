/*
 * slot. superchis: the few GBA hardware definitions the payload uses, in place of the libgba
 * names slot. advance relies on (REG_*, KEY_*, dmaCopy, SetMode, VBlankIntrWait).  Register
 * offsets come from SuperFW's gba_regs.h.  The payload must not touch IE/IME/DISPSTAT IRQ bits
 * (slot_host_api.h), so there is no IRQ code here: frame sync goes through the host API.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef SLOT_GBA_COMPAT_H
#define SLOT_GBA_COMPAT_H

#include <stdint.h>
#include "gba_regs.h"

#define SLOT_IO16(off)   (*(volatile uint16_t *)(0x04000000u + (off)))
#define SLOT_IO32(off)   (*(volatile uint32_t *)(0x04000000u + (off)))

#define SLOT_DISPCNT     SLOT_IO16(REG_DISPCNT)
#define SLOT_BLDCNT      SLOT_IO16(REG_BLDCNT)
#define SLOT_BLDY        SLOT_IO16(REG_BLDY)
#define SLOT_KEYINPUT    SLOT_IO16(0x130)

#define SLOT_MODE3_BG2   0x0403            /* Mode 3, BG2 on, nothing else */
#define SLOT_FORCE_BLANK 0x0080
#define SLOT_VRAM        ((volatile uint16_t *)0x06000000)

/* KEYINPUT bit order == the host get_keypress() order (gbahw.h KEY_BUTT*). */
#define SLOT_KEY_A       0x0001u
#define SLOT_KEY_B       0x0002u
#define SLOT_KEY_SELECT  0x0004u
#define SLOT_KEY_START   0x0008u
#define SLOT_KEY_RIGHT   0x0010u
#define SLOT_KEY_LEFT    0x0020u
#define SLOT_KEY_UP      0x0040u
#define SLOT_KEY_DOWN    0x0080u
#define SLOT_KEY_R       0x0100u
#define SLOT_KEY_L       0x0200u

/* Bulk copy (src/slot/slot_blit.s, IWRAM ARM). Not DMA: DMA3 from SuperCard SDRAM corrupts data
 * or hangs on hardware. bytes even, src/dst halfword aligned; 16/32-bit stores only (ADR-0010). */
void Slot_CopyMem(void *dst, const void *src, unsigned bytes) __attribute__((long_call));
#define Slot_Copy(dst, src, bytes) Slot_CopyMem((dst), (src), (bytes))

#endif

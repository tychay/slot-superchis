/*
 * slot. superchis: fixed memory layout shared by the firmware (common.h), the payload linker
 * script (ldscripts/slot_payload.ld) and rom_boot.S.  Pure #defines so it can be consumed by
 * cpp for linker scripts; no casts or C constructs here.  Rationale: design.md "SDRAM map (audited)".
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef SLOT_LAYOUT_H
#define SLOT_LAYOUT_H

/*
 * SDRAM window.  It must live in the 0x08xxxxxx half of SDRAM: with the SD interface mapped
 * into the ROM space (the menu's normal state) the whole 0x09xxxxxx half is shadowed by the
 * SuperCard SD registers.  It sits directly below the font pack (0x08E80000) and above
 * t_sdram_state (asserted in menu.c).
 */
#define SLOT_OFF_WINDOW           0x00B80000     /* window at 0x08B80000 */
#define SLOT_WINDOW_SIZE          0x00300000     /* 3MiB, ends where the fonts begin */

#define SLOT_OFF_PAYLOAD          0x00B80000     /* slot.payload image: text+rodata+IWRAM LMA */
#define SLOT_PAYLOAD_MAX          0x00040000     /* 256KiB */
#define SLOT_OFF_MAILBOX          0x00BC0000     /* host<->payload mailbox */
#define SLOT_MAILBOX_SIZE         0x00001000     /* 4KiB */
#define SLOT_OFF_STATICS          0x00BC1000     /* payload .bss / statics */
#define SLOT_STATICS_SIZE         0x00010000     /* 64KiB */
#define SLOT_OFF_SCRATCH          0x00BD1000     /* payload buffers: back buffer, art cache, ... */
#define SLOT_SCRATCH_SIZE         0x002AF000     /* up to the window end */

#define SLOT_PAYLOAD_ADDR         0x08B80000
#define SLOT_MAILBOX_ADDR         0x08BC0000
#define SLOT_STATICS_ADDR         0x08BC1000
#define SLOT_SCRATCH_ADDR         0x08BD1000

/*
 * IWRAM range the payload's hot code is copied into at entry.  It sits above the firmware's
 * .data/.bss/.iwram (asserted by gba_ewram.ld) and inside the stack headroom: the payload is
 * re-copied on every entry, so nothing persists there while the stock UI runs.
 */
#define SLOT_IWRAM_BASE           0x03003000
#define SLOT_IWRAM_SIZE           0x00002000     /* 8KiB (ADR-0005) */

#endif

/*
 * slot. superchis: linker script for slot.payload (ADR-0004).
 * Preprocessed with cpp (see the .ld.i rule in the Makefile); layout comes from slot_layout.h.
 *
 * Image layout (contiguous, objcopy -O binary): header, text, rodata, .iwram LMA, .data LMA.
 * Runtime: text/rodata execute in place from SDRAM; slot_entry() copies .iwram to
 * IWRAM[SLOT_IWRAM_BASE..], .data and zeroes .bss in the statics region of the window.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "../src/slot/slot_layout.h"

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(slot_entry)

MEMORY {
    SDRAM   : ORIGIN = SLOT_PAYLOAD_ADDR, LENGTH = SLOT_PAYLOAD_MAX
    STATICS : ORIGIN = SLOT_STATICS_ADDR, LENGTH = SLOT_STATICS_SIZE
    IWRAM   : ORIGIN = SLOT_IWRAM_BASE,   LENGTH = SLOT_IWRAM_SIZE
}

SECTIONS
{
    .header : ALIGN(4)
    {
        KEEP (*(.slot_header))
    } > SDRAM

    .text : ALIGN(4)
    {
        *(.text)
        *(.text.*)
        *(.glue_7)
        *(.glue_7t)
        *(.gnu.linkonce.t.*)
    } > SDRAM

    .rodata : ALIGN(4)
    {
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)
    } > SDRAM

    /* Hot code/data (ARM blitter, ...): runs from IWRAM, stored after rodata. */
    .iwram : ALIGN(4)
    {
        __iwram_start = .;
        *(.iwram_code)
        *(.iwram_code.*)
        *(.iwram)
        *(.iwram.*)
        . = ALIGN(4);
        __iwram_end = .;
    } > IWRAM AT > SDRAM

    /* Initialised statics live in the window's statics region, stored after .iwram. */
    .data : ALIGN(4)
    {
        __data_start = .;
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        __data_end = .;
    } > STATICS AT > SDRAM

    .bss (NOLOAD) : ALIGN(4)
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > STATICS

    .slot_end : ALIGN(4)
    {
        LONG(0)
    } > STATICS AT > SDRAM

    __iwram_lma  = LOADADDR(.iwram);
    __data_lma   = LOADADDR(.data);
    __iwram_size = __iwram_end - __iwram_start;
    __data_size  = __data_end - __data_start;
    __bss_size   = __bss_end - __bss_start;
    __image_size = LOADADDR(.slot_end) + 4 - ORIGIN(SDRAM);

    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) *(.comment) }
}

ASSERT(__iwram_size <= SLOT_IWRAM_SIZE, "slot payload IWRAM section exceeds 8KiB")
ASSERT(__image_size <= SLOT_PAYLOAD_MAX, "slot payload image exceeds its SDRAM region")

/*
 * slot. superchis: linker script for the emulator preview ROM (slot-preview.gba, task 5.1).
 * Preprocessed with cpp (see the .ld.i rule in the Makefile).
 *
 * Unlike the real payload (ldscripts/slot_payload.ld), which lives in the writable SDRAM window,
 * this image is an ordinary ROM at 0x08000000 (read-only in an emulator): code and rodata stay in
 * ROM, the payload's statics (.data/.bss) and the scratch buffers live in EWRAM, and the hot-code
 * .iwram section is copied to the same IWRAM range as on hardware by slot_entry().
 * The preview host's own variables go in .hostbss, which slot_entry() does not zero.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "../src/slot/slot_layout.h"

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(boot)

MEMORY {
    ROM   : ORIGIN = 0x08000000,        LENGTH = 1M
    EWRAM : ORIGIN = 0x02000000,        LENGTH = 256K
    IWRAM : ORIGIN = SLOT_IWRAM_BASE,   LENGTH = SLOT_IWRAM_SIZE
}

SECTIONS
{
    .boot : ALIGN(4) { KEEP (*(.boot)) } > ROM

    .text : ALIGN(4)
    {
        *(.text)
        *(.text.*)
        *(.glue_7)
        *(.glue_7t)
    } > ROM

    .rodata : ALIGN(4)
    {
        *(.rodata)
        *(.rodata.*)
        KEEP (*(.slot_header))
    } > ROM

    .iwram : ALIGN(4)
    {
        __iwram_start = .;
        *(.iwram_code)
        *(.iwram_code.*)
        *(.iwram)
        *(.iwram.*)
        . = ALIGN(4);
        __iwram_end = .;
    } > IWRAM AT > ROM

    .hostbss (NOLOAD) : ALIGN(4)
    {
        *(.hostbss)
    } > EWRAM

    .data : ALIGN(4)
    {
        __data_start = .;
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        __data_end = .;
    } > EWRAM AT > ROM

    .bss (NOLOAD) : ALIGN(4)
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > EWRAM

    /* Everything left of EWRAM is the scratch memory handed to the payload. */
    .scratch (NOLOAD) : ALIGN(4)
    {
        __scratch_start = .;
    } > EWRAM

    .slot_end : ALIGN(4) { LONG(0) } > ROM

    __iwram_lma  = LOADADDR(.iwram);
    __data_lma   = LOADADDR(.data);
    __iwram_size = __iwram_end - __iwram_start;
    __data_size  = __data_end - __data_start;
    __bss_size   = __bss_end - __bss_start;
    __image_size = 0;
    __scratch_size = ORIGIN(EWRAM) + LENGTH(EWRAM) - __scratch_start;

    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) *(.comment) }
}

ASSERT(__iwram_size <= SLOT_IWRAM_SIZE, "slot payload IWRAM section exceeds 8KiB")

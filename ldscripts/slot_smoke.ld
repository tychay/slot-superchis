/* slot. superchis: linker script for tools/slot_smoke_host.c (emulator smoke host). */
OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(boot)
MEMORY { ROM : ORIGIN = 0x08000000, LENGTH = 64K }
SECTIONS {
    .text : ALIGN(4) { KEEP(*(.boot)) *(.text) *(.text.*) *(.rodata) *(.rodata.*) } > ROM
    .data : ALIGN(4) { *(.data) *(.data.*) } > ROM
    .bss  : ALIGN(4) { *(.bss) *(.bss.*) } > ROM
}

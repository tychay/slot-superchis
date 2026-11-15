@ slot. superchis: preview ROM only (slot-preview.gba, developer tool).  Links the uncompressed asset
@ pack (the real build unpacks it into SDRAM at boot) and a few Cart Studio BMPs that preview_host.c
@ serves as fictional /SYSTEM/SLOT/CARTS files (tests/fixtures/art: converter output of real labels).
@ SPDX-License-Identifier: GPL-3.0-or-later
.section .rodata
.balign 4
.global slot_pack_blob
slot_pack_blob:      .incbin "res/slot_assets.pack"
.balign 4
.global pv_bmp_gba_a, pv_bmp_gba_b, pv_bmp_gba_flat, pv_bmp_gb, pv_bmp_gbc_a, pv_bmp_gbc_b
pv_bmp_gba_a:        .incbin "tests/fixtures/art/SYSTEM/SLOT/CARTS/GBA/gba_advance_wars.bmp"
pv_bmp_gba_b:        .incbin "tests/fixtures/art/SYSTEM/SLOT/CARTS/GBA/gba_golden_sun.bmp"
pv_bmp_gba_flat:     .incbin "tests/fixtures/art/SYSTEM/SLOT/CARTS_FLAT/GBA/gba_flat_advance_wars.bmp"
pv_bmp_gb:           .incbin "tests/fixtures/art/SYSTEM/SLOT/CARTS/GB/gb_kirby.bmp"
pv_bmp_gbc_a:        .incbin "tests/fixtures/art/SYSTEM/SLOT/CARTS/GBC/gbc_crystal.bmp"
pv_bmp_gbc_b:        .incbin "tests/fixtures/art/SYSTEM/SLOT/CARTS/GBC/gbc_shantae.bmp"

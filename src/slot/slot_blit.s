/*
 * slot. superchis -- Provenance (ADR-0006): copied from slot_blit.s; Slot_DecodeBMP555 dropped
 * (BMP art loader is task 6.2).  Slot_BlitOpaqueRow is the payload's hot pixel loop and lives in
 * the .iwram section (ldscripts/slot_payload.ld).
 * Source: vendor/slot-advance/common/slot_blit.s (slot. advance v1.2.4, commit a2a2fe9).
 * Licence: GPL-3.0-or-later (see NOTICE.md).
 */
/* Opaque span of a scaled RGB555 sprite. Maps contain byte offsets into row.
   Eight pixels per branch; kept in IWRAM for the GBA's limited frame budget. */
.section .iwram,"ax",%progbits
.arm
.align 2
.global Slot_BlitOpaqueRow
.type Slot_BlitOpaqueRow,%function
Slot_BlitOpaqueRow:
    stmfd sp!,{r4-r11,lr}
    cmp r3,#8
    blo 2f
1:
    ldmia r2!,{r4-r11}
    ldrh r4,[r0,r4]
    ldrh r5,[r0,r5]
    ldrh r6,[r0,r6]
    ldrh r7,[r0,r7]
    ldrh r8,[r0,r8]
    ldrh r9,[r0,r9]
    ldrh r10,[r0,r10]
    ldrh r11,[r0,r11]
    strh r4,[r1],#2
    strh r5,[r1],#2
    strh r6,[r1],#2
    strh r7,[r1],#2
    strh r8,[r1],#2
    strh r9,[r1],#2
    strh r10,[r1],#2
    strh r11,[r1],#2
    sub r3,r3,#8
    cmp r3,#8
    bhs 1b
2:
    cmp r3,#4
    blo 5f
    ldmia r2!,{r4-r7}
    ldrh r4,[r0,r4]
    ldrh r5,[r0,r5]
    ldrh r6,[r0,r6]
    ldrh r7,[r0,r7]
    strh r4,[r1],#2
    strh r5,[r1],#2
    strh r6,[r1],#2
    strh r7,[r1],#2
    sub r3,r3,#4
5:
    cmp r3,#0
    beq 4f
3:
    ldr r4,[r2],#4
    ldrh r4,[r0,r4]
    strh r4,[r1],#2
    subs r3,r3,#1
    bne 3b
4:
    ldmfd sp!,{r4-r11,lr}
    bx lr
.size Slot_BlitOpaqueRow,.-Slot_BlitOpaqueRow

.ltorg

/* slot: Slot_CopyMem(dst, src, bytes) -- CPU copy for SDRAM sources (ADR-0010 follow-up).
   DMA3 with a SuperCard SDRAM source corrupts data or hangs on hardware (diagnostic 2026-11-15),
   so every bulk copy in the payload goes through this loop. bytes must be even and dst/src
   halfword aligned; stores are 16/32-bit only (SDRAM takes no byte stores). */
.syntax unified
.section .iwram,"ax",%progbits
.arm
.align 2
.global Slot_CopyMem
.type Slot_CopyMem,%function
Slot_CopyMem:
    cmp     r2, #0
    bxeq    lr
    eor     r3, r0, r1
    tst     r3, #2
    bne     .Lcm_half
    tst     r0, #2
    beq     .Lcm_words
    ldrh    r3, [r1], #2
    strh    r3, [r0], #2
    subs    r2, r2, #2
    bxeq    lr
.Lcm_words:
    push    {r4-r10}
    subs    r2, r2, #32
    blt     .Lcm_tail
.Lcm_32:
    ldmia   r1!, {r3-r10}
    stmia   r0!, {r3-r10}
    subs    r2, r2, #32
    bge     .Lcm_32
.Lcm_tail:
    adds    r2, r2, #32
.Lcm_4:
    cmp     r2, #4
    blt     .Lcm_wdone
    ldr     r3, [r1], #4
    str     r3, [r0], #4
    sub     r2, r2, #4
    b       .Lcm_4
.Lcm_wdone:
    pop     {r4-r10}
    cmp     r2, #2
    ldrhge  r3, [r1]
    strhge  r3, [r0]
    bx      lr
.Lcm_half:
    ldrh    r3, [r1], #2
    strh    r3, [r0], #2
    subs    r2, r2, #2
    bgt     .Lcm_half
    bx      lr
.size Slot_CopyMem, .-Slot_CopyMem

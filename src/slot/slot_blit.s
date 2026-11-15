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

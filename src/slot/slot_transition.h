/*
 * slot. superchis -- Provenance (ADR-0006): adapted from slot_transition.h.
 * Source: vendor/slot-advance/common/slot_transition.h (slot. advance v1.2.4, commit a2a2fe9).
 * Licence: GPL-3.0-or-later (see NOTICE.md).
 *
 * Changes: libgba/UIAudio calls replaced by gba_compat.h and Slot_WaitFrame(); the splash is the
 * RLE-packed art from slot_art.h; Slot_FadeBlack added for the launch hand-off.  The firmware
 * rewrites BLDCNT/BLDALPHA when it restores the stock video state; Slot_FadeReset() also leaves
 * BLDY at 0.
 */
#ifndef SLOT_TRANSITION_H
#define SLOT_TRANSITION_H
#include "slot/slot_art.h"
static void Slot_FadeBlack(void)
{
    unsigned i;SLOT_BLDCNT=0x00ff;
    for(i=0;i<=16;i+=2){Slot_WaitFrame();SLOT_BLDY=i;}
}
static void Slot_FadeIn(void)
{
    int i;SLOT_BLDCNT=0x00ff;
    for(i=16;i>=0;i-=2){Slot_WaitFrame();SLOT_BLDY=(uint16_t)i;}
    SLOT_BLDCNT=0;SLOT_BLDY=0;
}
static void Slot_FadeReset(void){SLOT_BLDCNT=0;SLOT_BLDY=0;}
/* Unpack the splash (from the asset pack) straight into VRAM (Mode 3). */
static void Slot_DrawSplash(void)
{
    volatile uint16_t *out=SLOT_VRAM;unsigned i,n;
    const uint16_t *rle=slot_pack_u16(SLOT_PK_SPLASH);
    unsigned len=SLOT_PACK->item[SLOT_PK_SPLASH].size/2;
    for(i=0;i+1<len;i+=2) {
        uint16_t pixel=rle[i+1];
        for(n=rle[i];n;n--)*out++=pixel;
    }
}
#endif

/*
 * slot. superchis -- Provenance (ADR-0006): adapted from slot_timing.h.
 * Source: vendor/slot-advance/common/slot_timing.h (slot. advance v1.2.4, commit a2a2fe9).
 * Licence: GPL-3.0-or-later (see NOTICE.md).
 *
 * Changes: slot. advance ticks the spring from its own VBlank IRQ.  The payload may not install
 * IRQ handlers (slot_host_api.h), so Slot_SyncTicks() replays every VBlank that elapsed since the
 * last call, using the host's IRQ-driven frame counter; the result is identical because the
 * navigation tick only depends on the frame number and the key state.  The IME lock is gone
 * (no concurrent IRQ writer).
 */
#ifndef SLOT_TIMING_H
#define SLOT_TIMING_H
#include "slot/gba_compat.h"
#include "slot/slot_host_api.h"
#include "slot_motion.h"
#include "slot_navigation.h"
static unsigned slot_vblanks;
static unsigned slot_tick_base;         /* host frame_count when the payload started */
static SlotNavigation slot_navigation;
static const volatile unsigned *slot_frame_counter;
static void Slot_ClearNav(void) {slot_navigation.enabled=0;slot_navigation.direction=0;}
static void Slot_ResumeNav(unsigned count,unsigned index,unsigned wrap,unsigned animated)
{SlotNav_Reset(&slot_navigation,count,index,wrap,animated);}
static void Slot_NavSnapshot(unsigned *index,int *pose,unsigned *steps)
{*index=slot_navigation.index;*pose=SlotNav_Pose(slot_navigation.offset);*steps=slot_navigation.steps;}
static unsigned Slot_Held(void) {return (~SLOT_KEYINPUT)&0x3ff;}
/* Catch up with the VBlanks that happened since the last call (usually exactly one). */
static void Slot_SyncTicks(void) {
    unsigned target=*slot_frame_counter-slot_tick_base;
    while(slot_vblanks!=target) {
        slot_vblanks++;
        SlotNav_Tick(&slot_navigation,Slot_Held(),(unsigned)(((uint64_t)slot_vblanks*4389000)>>18));
    }
}
static void Slot_TimingInit(const struct slot_host_api *api) {
    slot_frame_counter=api->frame_count;slot_tick_base=*api->frame_count;slot_vblanks=0;
    /* A direction held while entering is not a new press. */
    slot_navigation.previous=Slot_Held()&48;if(slot_navigation.previous==48)slot_navigation.previous=0;
    slot_navigation.enabled=0;
}
/* Wait for the next VBlank edge, then run the ticks for it. */
static const struct slot_host_api *slot_host;
static void Slot_AudioFrame(unsigned vblank_now);     /* slot_audio.h, defined after this header */
static void Slot_WaitFrame(void) {
    unsigned f=*slot_frame_counter;
    do slot_host->wait_for_vblank(); while(*slot_frame_counter==f);
    Slot_SyncTicks();
    Slot_AudioFrame(slot_vblanks);
}
#define SLOT_INSERT_FRAMES 27
#define SLOT_EJECT_FRAMES SLOT_INSERT_FRAMES
#endif

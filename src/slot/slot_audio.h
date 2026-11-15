/*
 * slot. superchis -- the insert sound (task 6.4).
 *
 * Provenance (ADR-0006): the register values (FIFO A on Timer 0, DMA1 in sound-FIFO mode, signed 8-bit
 * mono PCM, reset the FIFO on every restart) follow the DirectSound driver in
 * vendor/slot-advance/common/slot_audio_driver.h (slot. advance v1.2.4, commit a2a2fe9,
 * GPL-3.0-or-later).  Everything else is new: that driver bounces the clip through a 8 KiB buffer
 * and stops it from a Timer 1 IRQ, and the payload has neither the memory nor an IRQ.
 *
 * Hardware rules this design is built around (hardware-verified 2026-11-15, ADR-0010, ADR-0011):
 *  - the clip lives in SDRAM (the asset pack) and DMA from SDRAM is wrong data or a hang, so the DMA
 *    source is a small IWRAM buffer that the CPU refills with Slot_Copy;
 *  - IWRAM is 12 KiB for everything and EWRAM belongs to the firmware, so the buffer is two frames
 *    (704 B), not the 5 KB clip;
 *  - the payload owns no IRQs, so the stream is paced by the VBlank the carousel already waits for.
 *
 * Streaming: Timer 0 period T and samples per frame N satisfy N*T = 280896 = cycles per GBA frame
 * (the packer resamples the clip to that rate, same duration).  Each frame Slot_AudioFrame() stops
 * DMA1 and Timer 0, resets FIFO A, refills the buffer with frame c of the clip (first N bytes) and
 * frame c+1 (next N bytes) and restarts both.  Frame c is chosen from the VBlank count, so a late
 * wake-up still plays the right audio, and a skipped frame plays the already-queued frame c+1.
 * The clip ends on its own padding of zeros; after the last frame everything is stopped.
 *
 * Stop path: Slot_AudioHalt() disables DMA1 and both timers, resets the FIFO and restores
 * SOUNDCNT_X/H and SOUNDBIAS.  slot_main() calls Slot_AudioShutdown() on EVERY return, and the launch
 * path waits for the clip to end first, so the game never starts with the FIFO running.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef SLOT_AUDIO_H
#define SLOT_AUDIO_H
#include "slot/gba_compat.h"
#include "slot/slot_art.h"

#define SLOT_SND_N      352u          /* samples per frame; == pack samples_per_frame */
#define SLOT_SND_T      798u          /* timer period in cycles; == pack timer_period */

#define SLOT_SOUNDCNT_L  SLOT_IO16(0x080)
#define SLOT_SOUNDCNT_H  SLOT_IO16(0x082)
#define SLOT_SOUNDCNT_X  SLOT_IO16(0x084)
#define SLOT_SOUNDBIAS   SLOT_IO16(0x088)
#define SLOT_FIFO_A_ADDR 0x040000A0u
#define SLOT_DMA1SAD     SLOT_IO32(0x0BC)
#define SLOT_DMA1DAD     SLOT_IO32(0x0C0)
#define SLOT_DMA1CNT_L   SLOT_IO16(0x0C4)
#define SLOT_DMA1CNT_H   SLOT_IO16(0x0C6)
#define SLOT_TM0D        SLOT_IO16(0x100)
#define SLOT_TM0CNT      SLOT_IO16(0x102)
#define SLOT_DMA1_FIFO   0xB640u      /* enable | special timing | repeat | 32-bit | dest fixed */
#define SLOT_FIFO_A_ON   0x0304u      /* A right+left, Timer 0, 100% */
#define SLOT_FIFO_A_RESET 0x0800u

static int8_t slot_snd_buf[2*SLOT_SND_N] __attribute__((aligned(4)));   /* IWRAM: DMA source (never SDRAM) */
static unsigned slot_snd_on;          /* 1 while the hardware is claimed (saved state valid) */
static unsigned slot_snd_start;       /* vblank count at which clip frame 0 starts */
static unsigned slot_snd_frames;      /* clip length in frames */
static unsigned slot_snd_saved_x,slot_snd_saved_h,slot_snd_saved_bias;

static unsigned Slot_AudioBusy(void){return slot_snd_on;}

/* Stop timers, DMA and FIFO, put the sound registers back. Idempotent. */
static void Slot_AudioHalt(void)
{
    if(!slot_snd_on)return;
    SLOT_DMA1CNT_H=0;SLOT_TM0CNT=0;
    SLOT_DMA1SAD=0;SLOT_DMA1DAD=0;SLOT_TM0D=0;
    SLOT_SOUNDCNT_H=(uint16_t)(slot_snd_saved_h|SLOT_FIFO_A_RESET);   /* flush the FIFO */
    SLOT_SOUNDCNT_H=(uint16_t)(slot_snd_saved_h&~SLOT_FIFO_A_RESET);
    SLOT_SOUNDCNT_X=(uint16_t)slot_snd_saved_x;
    SLOT_SOUNDBIAS=(uint16_t)slot_snd_saved_bias;
    slot_snd_on=0;
}
static void Slot_AudioShutdown(void){Slot_AudioHalt();}

/* Start the clip with its first frame at the next VBlank. No-op without a valid pack or when muted. */
static void Slot_AudioPlay(unsigned vblank_now)
{
    const struct slot_pack *pk=SLOT_PACK;
    if(slot_snd_on || !pk->insert_samples || pk->samples_per_frame!=SLOT_SND_N || pk->timer_period!=SLOT_SND_T)return;
    slot_snd_saved_x=SLOT_SOUNDCNT_X;slot_snd_saved_h=SLOT_SOUNDCNT_H;slot_snd_saved_bias=SLOT_SOUNDBIAS;
    slot_snd_frames=(pk->insert_samples+SLOT_SND_N-1)/SLOT_SND_N;
    slot_snd_start=vblank_now+1;
    slot_snd_on=1;
    SLOT_SOUNDCNT_X=0x0080;SLOT_SOUNDBIAS=0x0200;
    SLOT_DMA1CNT_H=0;SLOT_TM0CNT=0;
}
/* Called once per VBlank (from Slot_WaitFrame). */
static void Slot_AudioFrame(unsigned vblank_now)
{
    unsigned frame,i;uint32_t *w=(uint32_t*)slot_snd_buf;
    if(!slot_snd_on || vblank_now<slot_snd_start)return;
    frame=vblank_now-slot_snd_start;
    if(frame>=slot_snd_frames){Slot_AudioHalt();return;}
    SLOT_DMA1CNT_H=0;SLOT_TM0CNT=0;
    SLOT_SOUNDCNT_H=SLOT_FIFO_A_ON|SLOT_FIFO_A_RESET;
    Slot_Copy(slot_snd_buf,(const uint8_t*)slot_pack_item(SLOT_PK_INSERT)+frame*SLOT_SND_N,SLOT_SND_N);
    if(frame+1<slot_snd_frames)Slot_Copy(slot_snd_buf+SLOT_SND_N,(const uint8_t*)slot_pack_item(SLOT_PK_INSERT)+(frame+1)*SLOT_SND_N,SLOT_SND_N);
    else for(i=0;i<SLOT_SND_N/4;i++)w[SLOT_SND_N/4+i]=0;
    SLOT_DMA1SAD=(uint32_t)(uintptr_t)slot_snd_buf;SLOT_DMA1DAD=SLOT_FIFO_A_ADDR;
    SLOT_DMA1CNT_L=4;SLOT_DMA1CNT_H=SLOT_DMA1_FIFO;
    SLOT_TM0D=(uint16_t)(65536u-SLOT_SND_T);SLOT_TM0CNT=0x0080;
}
#endif

/*
 * slot. superchis -- Provenance (ADR-0006): adapted from slot_fast_cache.h.
 * Source: vendor/slot-advance/common/slot_fast_cache.h (slot. advance v1.2.4, commit a2a2fe9).
 * Licence: GPL-3.0-or-later (see NOTICE.md).
 *
 * Changes: slot. advance copies each cart image from slow PSRAM into four EWRAM "fast" banks
 * before drawing.  Here the cart images already sit in the SDRAM window (the same bus speed as
 * any copy would have) and EWRAM is reserved (ADR-0005), so the fast-bank machinery is dropped
 * and Slot_DrawShelfCarts() draws straight from the cache slots.  The partial-redraw band now
 * always reaches the title row (y=141); `footer` extends it to the bottom of the screen.
 */
#ifndef SLOT_FAST_CACHE_H
#define SLOT_FAST_CACHE_H
static void Slot_DrawShelfCarts(const uint16_t *const sources[5],int progress,int drop,unsigned footer)
{
    const uint16_t *pixels[5]={0};
    int centres[5],scales[5],i,pass;
    for(i=0;i<5;i++)if(sources[i] && (!drop || i==2)) {
        int relative=(i-2)*256-progress;
        int centre=Slot_CarouselCentre(slot_platform,relative);
        int scale=Slot_CarouselScale(relative);
        int width=Slot_ScaledExtent(slot_widths[slot_platform],scale);
        int left=centre-width/2;
        if(left+width>0 && left<240) {
            centres[i]=centre;scales[i]=scale;pixels[i]=sources[i];
        }
    }
    Slot_BeginShelf(slot_partial_motion && !drop,slot_partial_motion?(slot_platform==0?44:13):0,
                    slot_partial_motion?(drop || footer?160:SLOT_BAND_Y):160);
    for(pass=0;pass<2;pass++) {
        if(pass)Slot_BeginShelfPaint();
        for(i=0;i<5;i++)if(pixels[i])Slot_CartQ8(pixels[i],slot_platform,centres[i],scales[i],i==2?drop:0);
    }
    Slot_EndShelf();
}
#endif

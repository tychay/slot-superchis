/*
 * slot. superchis -- Provenance (ADR-0006): adapted from slot_render.h.
 * Source: vendor/slot-advance/common/slot_render.h (slot. advance v1.2.4, commit a2a2fe9).
 * Licence: GPL-3.0-or-later (see NOTICE.md).
 *
 * Changes for M2 (tasks 6.1-6.3): the art comes from the SDRAM asset pack (slot_art.h); Wood, Dark and
 * Light themes plus the flat ("Skeuomorphic=0") variants are back (slot_theme / slot_skeuo); the
 * opaque-span tables are per cache slot, computed from each loaded cart image (a Cart Studio cart
 * can have any outline) and kept in SDRAM, written with 16-bit stores (ADR-0010).
 *
 * Changes for M1 (design D7):
 *  - libgba (dmaCopy, IWRAM_CODE/ARM_CODE, EWRAM_BSS) replaced by gba_compat.h; hot loops are
 *    SLOT_FAST = ARM code in the payload's .iwram section (ADR-0005, <= 8 KiB).
 *  - Only the Wood theme with the dark skeuomorphic front/back bands and the three plain
 *    cartridge silhouettes exist (art arrays generated into slot_art_gen.c); the Dark/Light
 *    themes, flat variants, settings pages, About/Help, the cart-back board, the clock and the
 *    username are dropped (themes: task 6.3).
 *  - The frame buffer lives in the SDRAM window (slot_frame is set by slot_frontend.c).
 *  - The opaque-span tables are computed once per platform and copied per cache slot.
 *  - Slot_Text / the footer / the title row are new: M1 shows the selected cart's name as text.
 *  - The partial-redraw band always includes the title row.
 */
#ifndef SLOT_RENDER_H
#define SLOT_RENDER_H
#include <stdint.h>
#include <string.h>
#include "slot/gba_compat.h"
#include "slot/slot_art.h"
#include "slot/slot_sdram.h"
#include "slot_motion.h"
#define SLOT_FAST __attribute__((section(".iwram_code"),target("arm"),long_call,noinline,optimize("O2")))
#define SLOT_COPY(dst,src,n) Slot_Copy((dst),(src),(n))
#define SLOT_RGB(r,g,b) ((uint16_t)(((r)>>3)|(((g)>>3)<<5)|(((b)>>3)<<10)))
#define SLOT_KEY 0x8000
#define SLOT_W 240
#define SLOT_H 160
#define SLOT_TITLE_Y 129          /* selected cart's name; ink rows 131..140, above the 141 band */
#define SLOT_TITLE_TOP 130        /* first row repainted for the title on partial redraws */
#define SLOT_BAND_Y 141
#define SLOT_CACHE_SLOTS 5
extern void Slot_BlitOpaqueRow(const unsigned char *row,uint16_t *destination,const int *offsets,int count);
static uint16_t *slot_frame;
static unsigned slot_platform;
static unsigned slot_theme;               /* 0 wood, 1 dark, 2 light (SETTINGS.TXT "Theme") */
static unsigned slot_skeuo=1;             /* 1 skeuomorphic carts/bands, 0 flat (SETTINGS.TXT "Skeuomorphic") */
static int slot_redraw_top=0,slot_redraw_bottom=160;
static unsigned slot_partial_motion=0;
typedef struct {unsigned char left[112],right[112];} SlotOpaqueSpans;
typedef struct {const uint16_t *pixels;const SlotOpaqueSpans *spans;} SlotOpaque;
static SlotOpaque slot_opaque[SLOT_CACHE_SLOTS];
static SlotOpaqueSpans *slot_spans_base;       /* SLOT_CACHE_SLOTS tables in the SDRAM scratch (16-bit stores only) */
static uint16_t slot_row[120];                 /* IWRAM row buffer: art decode and span scans */
typedef struct {unsigned char count;unsigned char left[4],right[4];} SlotCoverage;
static SlotCoverage slot_coverage_banks[2][160];
static SlotCoverage *slot_covered=slot_coverage_banks[0],*slot_next_covered=slot_coverage_banks[1];
static unsigned slot_layout_only,slot_difference,slot_coverage_safe,slot_old_coverage_safe;
static const char *const slot_platform_names[3] = {"GBA", "GB", "GBC"};
static const uint16_t *Slot_Fallback(unsigned platform) {return slot_pack_u16((slot_skeuo?SLOT_PK_MOULD:SLOT_PK_FLAT)+platform);}
#define slot_font ((const unsigned char (*)[12])slot_pack_item(SLOT_PK_FONT))
static const unsigned slot_widths[3] = {120,104,104};
static const unsigned slot_heights[3] = {71,112,112};
static uint16_t Slot_Ink(void){return slot_theme==2?SLOT_RGB(40,40,48):SLOT_RGB(244,239,226);}
static uint16_t Slot_Warn(void){return slot_theme==2?SLOT_RGB(150,60,0):SLOT_RGB(255,211,143);}
static uint16_t Slot_GroundColour(void){return slot_theme==1?0:SLOT_RGB(235,235,231);}
#define SLOT_INK Slot_Ink()
#define SLOT_WARN Slot_Warn()
/* Opaque column span of one image row: left/right edge; left=255 when the row has holes inside. */
static void Slot_SpanRow(const uint16_t *p,unsigned w,unsigned char *left,unsigned char *right)
{
    unsigned l=0,r=w,x;
    while(l<r && p[l]==SLOT_KEY)l++;
    while(r>l && p[r-1]==SLOT_KEY)r--;
    *left=(unsigned char)l;*right=(unsigned char)r;
    for(x=l;x<r;x++)if(p[x]==SLOT_KEY){*left=255;break;}
}
/* Store one row's span bytes into the bank's SDRAM table (halfword read-modify-write, ADR-0010). */
static void Slot_SpanStore(unsigned bank,unsigned y,const uint16_t *row,unsigned w)
{
    unsigned char l,r;
    Slot_SpanRow(row,w,&l,&r);
    slot_sdram_store(&slot_spans_base[bank].left[y],&l,1);
    slot_sdram_store(&slot_spans_base[bank].right[y],&r,1);
}
static void Slot_RegisterOpaque(unsigned bank,const uint16_t *pixels)
{
    slot_opaque[bank].pixels=pixels;slot_opaque[bank].spans=&slot_spans_base[bank];
}
/* Spans from the finished image in SDRAM (the plain moulds): row by row through the IWRAM buffer. */
static void Slot_SpansFromImage(unsigned bank,const uint16_t *pixels,unsigned w,unsigned h)
{
    unsigned y;
    for(y=0;y<h;y++){SLOT_COPY(slot_row,pixels+y*w,w*2);Slot_SpanStore(bank,y,slot_row,w);}
}
static const SlotOpaque *Slot_OpaqueInfo(const uint16_t *pixels)
{
    unsigned i;for(i=0;i<SLOT_CACHE_SLOTS;i++)if(slot_opaque[i].pixels==pixels)return &slot_opaque[i];return 0;
}
static void SLOT_FAST Slot_Rect(int x,int y,int w,int h,uint16_t colour)
{
    int x1=x<0?0:x,y1=y<0?0:y,x2=x+w>240?240:x+w,y2=y+h>160?160:y+h;
    int xx,yy;
    for(yy=y1;yy<y2;yy++) for(xx=x1;xx<x2;xx++) slot_frame[yy*240+xx]=colour;
}
static unsigned slot_auto_ink;     /* Light theme footer: the band has a dark cart slot; flip dark glyphs to light over it */
static void SLOT_FAST Slot_Text(const char *text,int x,int y,uint16_t colour)
{
    int row,col;
    unsigned char c;
    const unsigned char (*font)[12]=slot_font;
    while((c=(unsigned char)*text++)) {
        if(c<32 || c>126) c='?';
        for(row=0;row<12;row++) for(col=0;col<8;col++)
            if((font[c][row]&(128>>col)) && (unsigned)(x+col)<240 && (unsigned)(y+row)<160) {
                uint16_t c2=colour,b=slot_frame[(y+row)*240+x+col];
                if(slot_auto_ink && ((b&31)+((b>>5)&31)+((b>>10)&31))<40)c2=SLOT_RGB(244,239,226);
                slot_frame[(y+row)*240+x+col]=c2;
            }
        x+=6;
        if(x>=240) break;
    }
}
static unsigned Slot_TextWidth(const char *text){return (unsigned)strlen(text)*6;}
static void Slot_Centred(const char *text,int y,uint16_t colour)
{
    Slot_Text(text,(240-(int)strlen(text)*6)/2,y,colour);
}
static inline void Slot_MapScaledEdges(unsigned char *inverse,unsigned sw,unsigned w,unsigned step)
{
    unsigned sx,dx=0,position=0;
    for(sx=0;sx<=sw;sx++) {
        while(dx<w && (position>>16)<sx){dx++;position+=step;}
        inverse[sx]=dx;
    }
}
static void SLOT_FAST Slot_Image(const uint16_t *src,int sw,int sh,int x,int y,int w,int h)
{
    int xx,yy,x1=x<0?-x:0,y1=y<0?-y:0,x2=w,y2=h;
    int map[120]; unsigned step_x,step_y;
    unsigned char inverse[121];
    const SlotOpaque *opaque=Slot_OpaqueInfo(src);
    if(x+w>240) x2=240-x;
    if(y+h>160) y2=160-y;
    if(w<=0 || w>120 || h<=0 || x1>=x2 || y1>=y2) return;
    if(w==sw && h==sh && !opaque) {
        for(yy=y1;yy<y2;yy++) {
            const uint16_t *s=src+yy*sw+x1;uint16_t *d=slot_frame+(y+yy)*240+x+x1;
            for(xx=x1;xx<x2;xx++){uint16_t c=*s++;if(c!=SLOT_KEY)*d=c;d++;}
        }
        return;
    }
    step_x=((unsigned)sw<<16)/(unsigned)w;step_y=((unsigned)sh<<16)/(unsigned)h;
    if(opaque)Slot_MapScaledEdges(inverse,sw,w,step_x);
    /* Byte offsets let ARM7 load a scaled pixel directly from its row base.
       Avoid an addition and shift inside every pixel iteration. */
    for(xx=x1;xx<x2;xx++) map[xx]=((xx*step_x)>>16)*2;
    for(yy=y1;yy<y2;yy++) {
        unsigned sy=((unsigned)yy*step_y)>>16;int first=x1,last=x2;
        const unsigned char *s=(const unsigned char*)(src+sy*sw);
        uint16_t *d;const int *offset;int remaining;
        if(opaque && opaque->spans->left[sy]!=255) {
            int left=inverse[opaque->spans->left[sy]],right=inverse[opaque->spans->right[sy]];
            if(first<left)first=left;
            if(last>right)last=right;
            if(first>=last)continue;
            d=slot_frame+(y+yy)*240+x+first;offset=map+first;remaining=last-first;
            if(w==sw && h==sh)SLOT_COPY(d,s+first*2,remaining*2);
            else Slot_BlitOpaqueRow(s,d,offset,remaining);
        } else {
            d=slot_frame+(y+yy)*240+x+x1;offset=map+x1;remaining=x2-x1;
            while(remaining--) {uint16_t c=*(const uint16_t*)(s+*offset++);if(c!=SLOT_KEY)*d=c;d++;}
        }
    }
}
static void SLOT_FAST Slot_Band(unsigned id,int y)
{
    int xx,yy;const uint16_t *src=slot_pack_u16(id);
    for(yy=0;yy<19;yy++) for(xx=0;xx<240;xx++) {
        uint16_t c=src[yy*240+xx];
        if(c!=SLOT_KEY && (unsigned)(y+yy)<160) slot_frame[(y+yy)*240+xx]=c;
    }
}
/* Pack id of the back (0) / front (1) band for the current theme (Wood uses the dark bands). */
static unsigned Slot_BandId(unsigned front){return (slot_skeuo?SLOT_PK_BACK_DARK:SLOT_PK_BACK_FLAT_DARK)+(slot_theme==2?2u:0u)+front;}
/* Paint the empty "desk" (wood picture, or the flat theme colour) over x..x+w, rows y..y+h. */
static void Slot_PaintGround(int x,int y,int w,int h)
{
    if(slot_theme==0) {
        const uint16_t *bg=slot_pack_u16(SLOT_PK_BACKGROUND);int row;
        if(w==240)SLOT_COPY(slot_frame+y*240,bg+y*240,h*240*2);
        else for(row=y;row<y+h;row++)SLOT_COPY(slot_frame+row*240+x,bg+row*240+x,w*2);
    } else Slot_Rect(x,y,w,h,Slot_GroundColour());
}
static void Slot_BackgroundRange(int top,int bottom)
{
    slot_redraw_top=top;slot_redraw_bottom=bottom;
    Slot_PaintGround(0,top,240,bottom-top);
    if(bottom>SLOT_BAND_Y) Slot_Band(Slot_BandId(0),SLOT_BAND_Y);
}
static void Slot_Front(void) {Slot_Band(Slot_BandId(1),SLOT_BAND_Y);}
static void Slot_Background(void) { Slot_BackgroundRange(0,160); }
static inline void Slot_RecordSpan(int y,int left,int right)
{
    SlotCoverage *row;
    if((unsigned)y>=160 || right<=0 || left>=240)return;
    if(left<0)left=0;
    if(right>240)right=240;
    if(left>=right)return;
    row=&slot_next_covered[y];
    if(row->count>=4){slot_coverage_safe=0;return;}
    row->left[row->count]=left;row->right[row->count]=right;row->count++;
}
static void SLOT_FAST Slot_RecordCart(const uint16_t *pixels,unsigned sw,unsigned sh,int x,int y,int w,int h)
{
    const SlotOpaque *o=Slot_OpaqueInfo(pixels);unsigned step_x,step_y;int yy;unsigned char inverse[121];
    if(!o){slot_coverage_safe=0;return;}
    step_x=(sw<<16)/w;step_y=(sh<<16)/h;
    Slot_MapScaledEdges(inverse,sw,w,step_x);
    for(yy=0;yy<h && y+yy<160;yy++)if(y+yy>=0) {
        unsigned sy=(yy*step_y)>>16;
        if(o->spans->left[sy]==255){slot_coverage_safe=0;return;}
        Slot_RecordSpan(y+yy,x+inverse[o->spans->left[sy]],x+inverse[o->spans->right[sy]]);
    }
}
static void SLOT_FAST Slot_BeginShelf(unsigned partial,int top,int bottom)
{
    int y;partial=partial && slot_old_coverage_safe;
    slot_redraw_top=top;slot_redraw_bottom=bottom;slot_difference=partial;slot_coverage_safe=1;
    for(y=0;y<160;y++)slot_next_covered[y].count=0;
    slot_layout_only=1;
    if(!partial)Slot_BackgroundRange(top,bottom);
}
static void SLOT_FAST Slot_BeginShelfPaint(void)
{
    int y;slot_layout_only=0;
    if(!slot_difference)return;
    if(!slot_coverage_safe){Slot_BackgroundRange(slot_redraw_top,slot_redraw_bottom);return;}
    for(y=slot_redraw_top;y<slot_redraw_bottom;y++) {
        unsigned i,j;SlotCoverage *old=&slot_covered[y],*next=&slot_next_covered[y];
        j=0;
        for(i=0;i<old->count;i++) {
            int cursor=old->left[i],right=old->right[i];
            while(cursor<right) {
                int gap=right;
                while(j<next->count && next->right[j]<=cursor)j++;
                if(j<next->count && next->left[j]<=cursor)cursor=next->right[j]<right?next->right[j]:right;
                else {
                    if(j<next->count && next->left[j]<gap)gap=next->left[j];
                    Slot_PaintGround(cursor,y,gap-cursor,1);
                    cursor=gap;
                }
            }
        }
    }
    /* The title row is not part of the cart coverage: repaint it from the background. */
    if(slot_redraw_bottom>SLOT_TITLE_TOP)
        Slot_PaintGround(0,SLOT_TITLE_TOP,240,SLOT_BAND_Y-SLOT_TITLE_TOP);
}
static void SLOT_FAST Slot_EndShelf(void){SlotCoverage *previous=slot_covered;slot_covered=slot_next_covered;slot_next_covered=previous;slot_old_coverage_safe=slot_coverage_safe;}
/* Footer (y=141..160): left = hint or notice, right = "<shelf> n/total". */
static void Slot_FooterText(const char *left,uint16_t left_colour,const char *right)
{
    slot_auto_ink=slot_theme==2;
    if(left && *left)Slot_Text(left,6,145,left_colour);
    if(right && *right)Slot_Text(right,234-(int)Slot_TextWidth(right),145,SLOT_INK);
    slot_auto_ink=0;
}
/* Repaint the whole footer (only valid when no cart overlaps it, i.e. not during the insert). */
static void Slot_Footer(const char *left,uint16_t left_colour,const char *right)
{
    Slot_PaintGround(0,SLOT_BAND_Y,240,160-SLOT_BAND_Y);
    Slot_Band(Slot_BandId(0),SLOT_BAND_Y);Slot_Front();
    Slot_FooterText(left,left_colour,right);
}
/* Plain full-screen message (empty shelf, errors). `hint` goes in the footer. */
static void Slot_Empty(const char *line1,const char *line2,const char *line3,const char *footer_right)
{
    Slot_Background();
    Slot_Centred(line1,55,SLOT_INK);
    if(line2)Slot_Centred(line2,73,SLOT_INK);
    if(line3)Slot_Centred(line3,85,SLOT_INK);
    Slot_Footer("Select: SuperFW menu",SLOT_INK,footer_right);
}
static int Slot_CarouselScale(int relative)
{int dist=relative<0?-relative:relative;return 25600-(dist>256?256:dist)*22;}
static int Slot_CarouselCentre(unsigned platform,int relative)
{
    int position=relative*(int)slot_widths[platform];
    return 120+((position+128-(position<0))>>8);
}
static int Slot_ScaledExtent(unsigned extent,int scale)
{return (extent*scale+12800)/25600;}
static void Slot_CartQ8(const uint16_t *pixels,unsigned platform,int centre,int scale,int drop)
{
    /* Round once, in output pixels. Whole-percent scale and truncation used to
       leave a final one-percent size jump when the spring reached rest. */
    int w=Slot_ScaledExtent(slot_widths[platform],scale),h=Slot_ScaledExtent(slot_heights[platform],scale);
    int y=platform==0 ? 116-h : 126-h;
    if(drop>0) {
        int target_w=80,target_h=platform==0?47:86;
        int foot=y+h,seated=143+target_h,catch=(141-foot)*256/(seated-foot),travel,k;
        if(drop>256)drop=256;
        /* Slot's travel/catch/creep/push path: recede to slot width first. */
        if(drop<108)travel=catch*(int)Slot_Smooth(drop*256/108)/256;
        else if(drop<159)travel=catch+8*(drop-108)/51;
        else travel=catch+8+(256-catch-8)*(int)Slot_Smooth((drop-159)*256/97)/256;
        k=travel<catch?travel*256/catch:256;
        w=w+(target_w-w)*k/256;h=h+(target_h-h)*k/256;
        y=foot+(seated-foot)*travel/256-h;
    }
    /* A small shadow anchors the selected cartridge to the desk. */
    if(slot_layout_only) {
        Slot_RecordCart(pixels,slot_widths[platform],slot_heights[platform],centre-w/2,y,w,h);
        if(slot_skeuo && scale>=95*256 && !drop){Slot_RecordSpan(y+h+2,centre-w/2+5,centre-w/2+w-5);Slot_RecordSpan(y+h+3,centre-w/2+5,centre-w/2+w-5);}
        return;
    }
    if(slot_skeuo && scale>=95*256 && drop==0) Slot_Rect(centre-w/2+5,y+h+2,w-10,2,slot_theme==2?SLOT_RGB(200,200,196):slot_theme==0?SLOT_RGB(24,21,21):0);
    Slot_Image(pixels,slot_widths[platform],slot_heights[platform],centre-w/2,y,w,h);
}
#endif

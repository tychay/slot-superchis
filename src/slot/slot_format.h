/*
 * slot. superchis: small parsing helpers.
 *
 * Provenance (ADR-0006): adapted from vendor/slot-advance/common/slot_format.h (slot. advance
 * v1.2.4, commit a2a2fe9), GPL-3.0-or-later.  Kept: Slot_IndexForCount, Slot_ParseNatural.
 * M2 (task 6.2): Slot_Le16/Le32/Slot_BMPValid and the RGB555 row decoder are back, so the BMP
 * art loader can validate and decode what tools/convert_labels.py writes (tests/slot_bmp_test.c).
 */
#ifndef SLOT_FORMAT_H
#define SLOT_FORMAT_H
#include <stdint.h>
#include <limits.h>
static inline uint16_t Slot_Le16(const unsigned char *p) { return (uint16_t)(p[0]|(uint16_t)p[1]<<8); }
static inline uint32_t Slot_Le32(const unsigned char *p) { return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static inline int Slot_IndexForCount(int index,unsigned count,unsigned wrap)
{
    if(!count) return -1;
    if(wrap) { index%=(int)count; if(index<0) index+=(int)count; return index; }
    return index<0 || index>=(int)count?-1:index;
}
static inline unsigned Slot_ParseNatural(const char *text,unsigned *out)
{
    unsigned value=0,digit,found=0;
    while(*text==' ' || *text=='\t') text++;
    while(*text>='0' && *text<='9') {
        digit=(unsigned)(*text++-'0');
        if(value>(UINT_MAX-digit)/10) return 0;
        value=value*10+digit; found=1;
    }
    while(*text==' ' || *text=='\t' || *text=='\r' || *text=='\n') text++;
    if(!found || *text) return 0;
    *out=value; return 1;
}
/* The only BMP the converter writes: 54-byte header, 16-bit BI_RGB RGB555, bottom-up, even width.
 * Nothing file-controlled reaches the image cache: dimensions must equal the shelf's cart size. */
static inline unsigned Slot_BMPValid(const unsigned char *h,unsigned bytes,unsigned expected_w,unsigned expected_h)
{
    unsigned w=Slot_Le32(h+18),height=Slot_Le32(h+22);
    return h[0]=='B' && h[1]=='M' && Slot_Le32(h+14)==40 && Slot_Le32(h+10)==54 &&
           Slot_Le16(h+26)==1 && Slot_Le16(h+28)==16 && Slot_Le32(h+30)==0 &&
           expected_w>0 && expected_h>0 && expected_w<=120 && expected_h<=112 && !(expected_w&1) &&
           w==expected_w && height==expected_h && bytes==54+w*height*2 && Slot_Le32(h+2)==bytes;
}
/* One row, in place: file order is B5G5R5 with magenta (0x7c1f) as the colour key; the screen wants
 * R in the low bits and bit 15 (0x8000) as the key. */
static inline void Slot_DecodeBMP555Row(uint16_t *pixels,unsigned count)
{
    while(count--){unsigned c=*pixels;*pixels++=(uint16_t)(c==0x7c1f?0x8000:((c&31)<<10)|(c&0x3e0)|((c>>10)&31));}
}
/* Art path of a ROM: /SYSTEM/SLOT/CARTS[_FLAT]/<GBA|GB|GBC>/<file name without its last extension>.bmp,
 * the layout tools/convert_labels.py writes (names are the ROM stems exactly). Returns the length, or
 * 0 if the name has no extension or the path would not fit in `cap` bytes (incl. NUL). */
static inline unsigned Slot_ArtPathFor(char *out,unsigned cap,unsigned skeuo,const char *platform,const char *name)
{
    const char *prefix=skeuo?"/SYSTEM/SLOT/CARTS/":"/SYSTEM/SLOT/CARTS_FLAT/",*dot=0,*c;
    unsigned n=0;
    for(c=prefix;*c;c++){if(n+1>=cap)return 0;out[n++]=*c;}
    for(c=platform;*c;c++){if(n+1>=cap)return 0;out[n++]=*c;}
    if(n+1>=cap)return 0;
    out[n++]='/';
    for(c=name;*c;c++)if(*c=='.')dot=c;
    if(!dot)return 0;
    for(c=name;c<dot;c++){if(n+1>=cap)return 0;out[n++]=*c;}
    for(c=".bmp";*c;c++){if(n+1>=cap)return 0;out[n++]=*c;}
    out[n]=0;
    return n;
}
#endif

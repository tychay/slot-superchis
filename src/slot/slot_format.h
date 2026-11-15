/*
 * slot. superchis: small parsing helpers.
 *
 * Provenance (ADR-0006): adapted from vendor/slot-advance/common/slot_format.h (slot. advance
 * v1.2.4, commit a2a2fe9), GPL-3.0-or-later.  Kept: Slot_IndexForCount, Slot_ParseNatural.
 * Dropped for M1: Slot_Le16/Le32/Slot_BMPValid (BMP art loader, task 6.2).
 */
#ifndef SLOT_FORMAT_H
#define SLOT_FORMAT_H
#include <stdint.h>
#include <limits.h>
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
#endif

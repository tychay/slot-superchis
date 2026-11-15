/*
 * slot. superchis -- the cartridge carousel (payload side, ADR-0004).
 *
 * Provenance (ADR-0006): adapted from vendor/slot-advance/common/slot_frontend.h (Slot_Run,
 * Slot_Scan, Slot_Play, Slot_ChangeShelf, Slot_DrawShelf, Slot_Present, cache and config code;
 * slot. advance v1.2.4, commit a2a2fe9, GPL-3.0-or-later).
 *
 * Changes for M1 (design D2/D3/D7):
 *  - all FatFs access goes through the host API; none of the DS Style kernel's globals, file
 *    records, audio or launcher calls remain (task 4.2 greps for them);
 *  - the library scan/sort/filter and the settings file are the pure modules slot_scan.c and
 *    slot_settings.c (unit tested on the host); the shelf is capped at SLOT_SHELF_MAX;
 *  - the back buffer, the cart cache and the shelf live in the SDRAM scratch the host provides;
 *  - the cart "art" is the plain silhouette with the name drawn on its label (Slot_LoadFallback);
 *    the BMP art loader (Slot_LoadBMP/RequestArt/PumpArt) is task 6.2;
 *  - Play returns SLOT_LAUNCH with the path in the mailbox instead of calling a loader; the
 *    firmware opens the stock load popup and re-enters the carousel when it is dismissed;
 *  - Select returns SLOT_STOCK_UI; Start/B settings pages, cart back, clock, username, sounds,
 *    hotkeys and eject-replay are dropped (post-M1).
 */
#include "slot/slot_host_api.h"
#include "slot/slot_scan.h"
#include "slot/slot_sdram.h"
#include "slot/slot_settings.h"
#include "slot/slot_format.h"
#include "slot_render.h"
#include "slot_timing.h"
#include "slot_fast_cache.h"
#include "slot_transition.h"
#include "slot/slot_diag.h"

#define SLOT_DIR "/SYSTEM/SLOT"
#define SLOT_CONFIG SLOT_DIR "/SETTINGS.TXT"
#define SLOT_CONFIG_TMP SLOT_DIR "/SETTINGS.TMP"
#define SLOT_CONFIG_BAK SLOT_DIR "/SETTINGS.BAK"
#define SLOT_GAMES_DIR "/Games"
#define SLOT_TITLE_COLS 38

/* Scratch layout (SDRAM window, 16-bit stores only, ADR-0010; EWRAM in the preview ROM): back buffer, cart cache, shelf. */
#define SLOT_FRAME_BYTES (240u*160u*2u)
#define SLOT_CACHE_STRIDE (104u*112u*2u)      /* the larger (GB/GBC) cart */
#define SLOT_CACHE_BYTES (SLOT_CACHE_SLOTS*SLOT_CACHE_STRIDE)
#define SLOT_SCRATCH_MIN (SLOT_FRAME_BYTES+SLOT_CACHE_BYTES+SLOT_SHELF_INDEX_BYTES+4096u)
_Static_assert(120u*71u*2u<=SLOT_CACHE_STRIDE && SLOT_CACHE_STRIDE%4==0,"cart cache stride");
/* Three full shelves (and more) would fit the window's scratch: the memory cap is by construction. */
_Static_assert(SLOT_FRAME_BYTES+SLOT_CACHE_BYTES+3u*SLOT_SHELF_BYTES<=SLOT_SCRATCH_SIZE,
               "frame + cache + 3 shelves must fit the SDRAM scratch");
_Static_assert(SLOT_SHELF_MAX<=65535u,"shelf order is uint16_t");

static char slot_launch[SLOT_MAX_PATH+2] __attribute__((aligned(4)));   /* launch path staging (IWRAM) */
typedef struct { int index; unsigned valid; } SlotCache;
static SlotCache slot_cache[SLOT_CACHE_SLOTS];
static uint16_t *slot_cache_base;
static struct slot_shelf slot_shelf;
static struct slot_settings slot_cfg;
static unsigned slot_cfg_dirty,slot_wrap=1,slot_animations=1;
static unsigned slot_count;
static unsigned slot_library_state;      /* 0 ok, 1 no /Games folder, 2 SD error */
static unsigned slot_scan_missing,slot_scan_error;
static unsigned slot_footer_dirty;
static FIL slot_file;
static DIR slot_dir;
static FILINFO slot_info;
static char slot_title[SLOT_NAME_MAX];
static char slot_line[SLOT_NAME_MAX];
static char slot_right[24];
static char slot_notice[28];
static char slot_cfg_text[SLOT_SETTINGS_MAX_TEXT+256];
static const char *const slot_dirs[3]={SLOT_GAMES_DIR "/GBA",SLOT_GAMES_DIR "/GB",SLOT_GAMES_DIR "/GBC"};

/* --- tiny string helpers (the payload links no libc) --- */
static unsigned Slot_Utoa(char *out,unsigned v)
{
    char tmp[11];unsigned n=0,i;
    do{tmp[n++]=(char)('0'+v%10);v/=10;}while(v);
    for(i=0;i<n;i++)out[i]=tmp[n-1-i];
    out[n]=0;return n;
}
static unsigned Slot_Append(char *out,unsigned cap,unsigned at,const char *text)
{
    while(*text && at+1<cap)out[at++]=*text++;
    out[at]=0;return at;
}

/* --- cache of the (plain) cart images, one per visible shelf position --- */
static int Slot_Normalize(int index)
{
    return Slot_IndexForCount(index,slot_count,slot_wrap);
}
static unsigned Slot_Index(void){return slot_cfg.v[SLOT_CFG_GBA+slot_platform];}
static void Slot_SetIndex(unsigned index)
{
    if(slot_cfg.v[SLOT_CFG_GBA+slot_platform]!=index){slot_cfg.v[SLOT_CFG_GBA+slot_platform]=index;slot_cfg_dirty=1;}
}
static void Slot_ResetCache(void)
{
    unsigned i;
    for(i=0;i<SLOT_CACHE_SLOTS;i++){slot_cache[i].valid=0;slot_cache[i].index=-1;slot_opaque[i].pixels=0;}
}
static void Slot_Stem(unsigned index,char *out,unsigned size)
{
    slot_title_from_name(slot_shelf_name(&slot_shelf,index),out,size);
}
static void Slot_ReadTitle(void)
{
    slot_title[0]=0;
    if(slot_count){Slot_Stem(Slot_Index(),slot_line,sizeof(slot_line));slot_ellipsize(slot_line,slot_title,SLOT_TITLE_COLS);}
}
static SlotCache *Slot_GetCache(int index)
{
    unsigned i;index=Slot_Normalize(index);
    if(index<0)return 0;
    for(i=0;i<SLOT_CACHE_SLOTS;i++)if(slot_cache[i].valid && slot_cache[i].index==index)return &slot_cache[i];
    return 0;
}
static const uint16_t *Slot_CachedPixels(SlotCache *cache)
{
    return slot_cache_base+(unsigned)(cache-slot_cache)*(SLOT_CACHE_STRIDE/2);
}
/* Plain silhouette with the cart's name on its label panel. */
static void Slot_LoadFallback(uint16_t *pixels,const char *name)
{
    unsigned w=slot_widths[slot_platform],h=slot_heights[slot_platform],row,col,n;
    unsigned panel_x=slot_platform==0?16:15,panel_y=slot_platform==0?17:slot_platform==1?32:37;
    unsigned panel_w=slot_platform==0?88:74,panel_h=slot_platform==0?45:66;
    unsigned cells=(panel_w-8)/6,len=(unsigned)strlen(name),lines=(len+cells-1)/cells;
    if(lines>(panel_h-4)/12)lines=(panel_h-4)/12;
    SLOT_COPY(pixels,Slot_Fallback(slot_platform),w*h*2);
    for(n=0;n<lines;n++) {
        unsigned count=len-n*cells;if(count>cells)count=cells;
        unsigned x=panel_x+(panel_w-count*6)/2,y=panel_y+(panel_h-lines*12)/2+n*12,i;
        for(i=0;i<count;i++) {
            unsigned c=(unsigned char)name[n*cells+i];if(c<32 || c>126)c='?';
            for(row=0;row<12;row++)for(col=0;col<6;col++)
                if(slot_font[c][row]&(128>>col))pixels[(y+row)*w+x+i*6+col]=SLOT_RGB(40,40,47);
        }
    }
}
static void Slot_FillCacheAround(int centre)
{
    int wanted[5],d; unsigned i,j;
    /* Keep neighbours during a move. */
    for(d=-2;d<=2;d++) wanted[d+2]=Slot_Normalize(centre+d);
    for(i=0;i<SLOT_CACHE_SLOTS;i++) {
        unsigned keep=0;
        for(j=0;j<5;j++) if(slot_cache[i].index==wanted[j]) keep=1;
        if(!keep) slot_cache[i].valid=0;
    }
    for(j=0;j<5;j++) {
        int index=wanted[j];
        if(index<0 || Slot_GetCache(index)) continue;
        for(i=0;i<SLOT_CACHE_SLOTS;i++) if(!slot_cache[i].valid) break;
        if(i==SLOT_CACHE_SLOTS) break;
        Slot_Stem((unsigned)index,slot_line,sizeof(slot_line));
        Slot_LoadFallback(slot_cache_base+i*(SLOT_CACHE_STRIDE/2),slot_line);
        Slot_RegisterOpaque(i,slot_cache_base+i*(SLOT_CACHE_STRIDE/2),slot_platform);
        slot_cache[i].index=index; slot_cache[i].valid=1;
    }
}
static void Slot_FillCache(void){Slot_FillCacheAround((int)Slot_Index());}

/* --- presenting --- */
static void Slot_Present(void)
{
    Slot_WaitFrame();
    Slot_DmaCopy((void*)(SLOT_VRAM+slot_redraw_top*240),slot_frame+slot_redraw_top*240,(unsigned)(slot_redraw_bottom-slot_redraw_top)*240*2);
}

/* --- settings (SETTINGS.TXT) --- */
static unsigned Slot_ReadConfigFile(const char *path)
{
    UINT got=0;FRESULT result;
    if(slot_host->f_open(&slot_file,path,FA_READ)!=FR_OK)return 0;
    result=slot_host->f_read(&slot_file,slot_cfg_text,SLOT_SETTINGS_MAX_TEXT+255,&got);
    slot_host->f_close(&slot_file);
    if(result!=FR_OK || got>=SLOT_SETTINGS_MAX_TEXT+255)return 0;   /* absent, unreadable or too big */
    return (unsigned)slot_settings_parse(slot_cfg_text,got,&slot_cfg);
}
static void Slot_ReadConfig(void)
{
    if(!Slot_ReadConfigFile(SLOT_CONFIG) && !Slot_ReadConfigFile(SLOT_CONFIG_BAK))slot_settings_defaults(&slot_cfg);
    slot_wrap=slot_cfg.v[SLOT_CFG_WRAP];slot_animations=slot_cfg.v[SLOT_CFG_ANIMATIONS];
    slot_platform=slot_cfg.v[SLOT_CFG_REMEMBER]?slot_cfg.v[SLOT_CFG_PLATFORM]:0;
    if(!slot_cfg.v[SLOT_CFG_REMEMBER]){
        slot_cfg.v[SLOT_CFG_GBA]=slot_cfg.v[SLOT_CFG_GB]=slot_cfg.v[SLOT_CFG_GBC]=0;
    }
    slot_cfg_dirty=0;
}
/* Same write order as slot. advance: temp file, then swap in via a backup, so a power cut never
   leaves a half-written primary file. */
static unsigned Slot_SaveConfig(void)
{
    unsigned length;UINT written=0;FRESULT result;
    slot_cfg.v[SLOT_CFG_PLATFORM]=slot_platform;
    if(!slot_cfg_dirty)return 1;
    length=slot_settings_format(&slot_cfg,slot_cfg_text,sizeof(slot_cfg_text));
    if(!length)return 0;
    slot_host->f_mkdir("/SYSTEM");slot_host->f_mkdir(SLOT_DIR);
    if(slot_host->f_open(&slot_file,SLOT_CONFIG_TMP,FA_CREATE_ALWAYS|FA_WRITE)!=FR_OK)return 0;
    result=slot_host->f_write(&slot_file,slot_cfg_text,length,&written);
    if(slot_host->f_close(&slot_file)!=FR_OK || result!=FR_OK || written!=length)return 0;
    slot_host->f_unlink(SLOT_CONFIG_BAK);
    result=slot_host->f_rename(SLOT_CONFIG,SLOT_CONFIG_BAK);
    if(result!=FR_OK && result!=FR_NO_FILE)return 0;
    if(slot_host->f_rename(SLOT_CONFIG_TMP,SLOT_CONFIG)!=FR_OK){slot_host->f_rename(SLOT_CONFIG_BAK,SLOT_CONFIG);return 0;}
    slot_cfg_dirty=0;return 1;
}

/* --- library scan (task 4.3/4.3b) --- */
static int Slot_NextEntry(void *ctx,char *name,uint32_t cap,uint32_t *size,unsigned *flags)
{
    unsigned n=0;(void)ctx;
    if(slot_host->f_readdir(&slot_dir,&slot_info)!=FR_OK)return -1;
    if(!slot_info.fname[0])return 0;
    while(n+1<cap && slot_info.fname[n]){name[n]=slot_info.fname[n];n++;}
    name[n]=0;
    *size=(uint32_t)slot_info.fsize;
    *flags=(slot_info.fattrib&AM_DIR?SLOT_DE_DIR:0)|(slot_info.fattrib&AM_HID?SLOT_DE_HIDDEN:0)|(slot_info.fattrib&AM_SYS?SLOT_DE_SYSTEM:0);
    return 1;
}
static void Slot_CheckLibrary(void)
{
    FRESULT result=slot_host->f_stat(SLOT_GAMES_DIR,&slot_info);
    slot_library_state=result==FR_OK?0:(result==FR_NO_FILE || result==FR_NO_PATH)?1:2;
}
/* The "Reading shelf..." screen: shown before any SD access so a hang there leaves a readable screen. */
static void Slot_ShowReading(void)
{
    Slot_Background();Slot_Front();Slot_Centred("Reading shelf...",73,SLOT_INK);
    slot_redraw_top=0;slot_redraw_bottom=160;Slot_Present();
}

static void Slot_Scan(void)
{
    FRESULT result;
    Slot_ResetCache();slot_footer_dirty=1;
    slot_count=0;slot_scan_missing=0;slot_scan_error=0;slot_shelf_clear(&slot_shelf);
    if(slot_library_state==0) {
        Slot_ShowReading();
        result=slot_host->f_opendir(&slot_dir,slot_dirs[slot_platform]);
        slot_scan_missing=result==FR_NO_PATH || result==FR_NO_FILE;
        if(result==FR_OK) {
            slot_scan(&slot_shelf,slot_platform,(unsigned)strlen(slot_dirs[slot_platform]),Slot_NextEntry,0);
            slot_host->f_closedir(&slot_dir);
            slot_scan_error=slot_shelf.error;
        } else slot_scan_error=!slot_scan_missing;
        slot_count=slot_shelf.count;
    }
    if(Slot_Index()>=slot_count)Slot_SetIndex(0);
    Slot_ReadTitle();Slot_FillCache();
}

/* --- drawing --- */
static void Slot_BuildFooter(const char **left,uint16_t *colour)
{
    unsigned n;
    *left="Select: SuperFW menu";*colour=SLOT_INK;
    if(slot_scan_error){*left="SD read error";*colour=SLOT_WARN;}
    else if(slot_shelf.truncated){
        n=Slot_Append(slot_notice,sizeof(slot_notice),0,"First ");
        n+=Slot_Utoa(slot_notice+n,slot_count);Slot_Append(slot_notice,sizeof(slot_notice),n," games shown");
        *left=slot_notice;*colour=SLOT_WARN;
    } else if(slot_shelf.skipped){
        n=Slot_Utoa(slot_notice,slot_shelf.skipped);Slot_Append(slot_notice,sizeof(slot_notice),n," names too long");
        *left=slot_notice;*colour=SLOT_WARN;
    }
    n=Slot_Append(slot_right,sizeof(slot_right),0,slot_platform_names[slot_platform]);
    if(slot_count) {
        n=Slot_Append(slot_right,sizeof(slot_right),n," ");
        n+=Slot_Utoa(slot_right+n,Slot_Index()+1);n=Slot_Append(slot_right,sizeof(slot_right),n,"/");
        n+=Slot_Utoa(slot_right+n,slot_count);
        if(slot_shelf.truncated)Slot_Append(slot_right,sizeof(slot_right),n,"+");
    }
}
static void Slot_DrawShelf(int centre_index,int progress,int drop,unsigned footer)
{
    const uint16_t *sources[5]={0};
    const char *left;uint16_t colour;
    int d;
    for(d=-2;d<=2;d++) {
        if((d==0 || (slot_count>1 && !drop)) && Slot_Normalize(centre_index+d)>=0) {
            SlotCache *cache=Slot_GetCache(centre_index+d);
            sources[d+2]=cache?Slot_CachedPixels(cache):Slot_Fallback(slot_platform);
        }
    }
    Slot_DrawShelfCarts(sources,progress,drop,footer);
    if(!drop)Slot_Centred(slot_title,SLOT_TITLE_Y,SLOT_INK);
    Slot_BuildFooter(&left,&colour);
    if(drop)Slot_Front();                   /* the sinking cart goes behind the front band; no text over it */
    else if(!slot_partial_motion){Slot_Front();Slot_FooterText(left,colour,slot_right);}
    else if(footer){Slot_Footer(left,colour,slot_right);}
    slot_footer_dirty=0;
}
static void Slot_ShowEmpty(void)
{
    Slot_Empty(slot_library_state==1?"No Games folder":slot_library_state==2?"SD read error":
               slot_scan_error?"SD read error":"Your shelf is empty",
               slot_library_state==1?"Put unzipped ROMs in":slot_library_state==2?"Check the card, then":
               slot_scan_error?"Reinsert the card or":"Put unzipped ROMs in",
               slot_library_state==1?"/Games/GBA, GB and GBC":slot_library_state==2?"press Select":
               slot_scan_error?"press Select":slot_dirs[slot_platform],slot_platform_names[slot_platform]);
}
static void Slot_Show(int base,int fraction,unsigned redraw)
{
    slot_partial_motion=!redraw;
    if(!slot_count)Slot_ShowEmpty();else Slot_DrawShelf(base,fraction,0,redraw|slot_footer_dirty);
    Slot_Present();slot_partial_motion=0;
}

/* --- shelves and launch --- */
static void Slot_ChangeShelf(int direction)
{
    Slot_ClearNav();
    slot_platform=(slot_platform+(direction>0?1u:2u))%3u;
    slot_cfg_dirty=1;
    Slot_Scan();
}
/* The insert animation, then hand the path to the firmware. Returns 1 when launching. */
static unsigned Slot_Play(void)
{
    unsigned frame,start,length,dir_len;
    struct slot_mailbox *mailbox=slot_host->mailbox;
    if(!slot_count)return 0;
    Slot_ClearNav();
    if(slot_animations) {
        slot_partial_motion=1;start=slot_vblanks;
        do {
            unsigned elapsed=slot_vblanks-start+1;
            frame=elapsed>=SLOT_INSERT_FRAMES?256:elapsed*256/SLOT_INSERT_FRAMES;
            Slot_DrawShelf((int)Slot_Index(),0,(int)frame,0);Slot_Present();
        }while(frame<256);
        slot_partial_motion=0;
    } else {Slot_DrawShelf((int)Slot_Index(),0,256,0);Slot_Present();}
    slot_cfg_dirty=1;
    Slot_SaveConfig();          /* a failed save only loses the remembered position, never the launch */
    /* ADR-0010: the mailbox is SDRAM (16-bit stores only): build the path in IWRAM, then copy it. */
    dir_len=Slot_Append(slot_launch,SLOT_MAX_PATH,0,slot_dirs[slot_platform]);
    dir_len=Slot_Append(slot_launch,SLOT_MAX_PATH,dir_len,"/");
    length=Slot_Append(slot_launch,SLOT_MAX_PATH,dir_len,slot_shelf_name(&slot_shelf,Slot_Index()));
    if(length!=dir_len+(unsigned)strlen(slot_shelf_name(&slot_shelf,Slot_Index())))return 0;   /* cannot happen: the scan checked it */
    slot_launch[length+1]=0;slot_sdram_store(mailbox->launch_path,slot_launch,(length+2)&~1u);
    mailbox->launch_size=slot_shelf_size(&slot_shelf,Slot_Index());
    Slot_FadeBlack();
    return 1;
}

static int Slot_Run(unsigned boot)
{
    unsigned nav_index,steps,redraw=1;int pose=0,last_pose=0,last_base=0,last_fill;
    /* Blank, black, Mode 3: nothing of the stock Mode 4 screen may show through. */
    SLOT_DISPCNT=SLOT_FORCE_BLANK|SLOT_MODE3_BG2;SLOT_BLDCNT=0x00ff;SLOT_BLDY=16;
    if(boot) {
        unsigned i;
        Slot_DrawSplash();SLOT_DISPCNT=SLOT_MODE3_BG2;
        Slot_FadeIn();for(i=0;i<18;i++)Slot_WaitFrame();Slot_FadeBlack();
        SLOT_DISPCNT=SLOT_FORCE_BLANK|SLOT_MODE3_BG2;
    }
    /* slot: show "Reading shelf..." (display on, no black) before the first SD access, so a hang
     * in settings/library/scan leaves a readable screen instead of white (task 5.3b). */
#ifdef SLOT_DIAG
    {   /* hardware diagnostic: display on, black, trail of blocks (slot_diag.h) */
        volatile uint16_t *v=SLOT_VRAM;unsigned i;
        SLOT_BLDCNT=0;SLOT_BLDY=0;for(i=0;i<240*160;i++)v[i]=0;
        SLOT_DISPCNT=SLOT_MODE3_BG2;
        SLOT_DIAG_BEGIN(0);Slot_DiagSelfTest(slot_background);SLOT_DIAG_END(0);
        SLOT_DIAG_BEGIN(1);Slot_Background();SLOT_DIAG_END(1);
        SLOT_DIAG_BEGIN(2);Slot_Front();SLOT_DIAG_END(2);
        SLOT_DIAG_BEGIN(3);Slot_Centred("Reading shelf...",73,SLOT_INK);SLOT_DIAG_END(3);
        SLOT_DIAG_BEGIN(4);Slot_ReadConfig();SLOT_DIAG_END(4);
        SLOT_DIAG_BEGIN(5);Slot_CheckLibrary();SLOT_DIAG_END(5);
        Slot_DiagBlock(0,6,slot_library_state==0?DIAG_GREEN:slot_library_state==1?DIAG_BLUE:DIAG_RED);
        SLOT_DIAG_BEGIN(7);Slot_ResetCache();SLOT_DIAG_END(7);
        SLOT_DIAG_BEGIN(8);slot_shelf_clear(&slot_shelf);SLOT_DIAG_END(8);
        SLOT_DIAG_BEGIN(9);i=slot_host->f_opendir(&slot_dir,slot_dirs[slot_platform]);SLOT_DIAG_END(9);
        Slot_DiagBlock(0,10,i==FR_OK?DIAG_GREEN:DIAG_RED);
        if(i==FR_OK){
            SLOT_DIAG_BEGIN(11);slot_scan(&slot_shelf,slot_platform,(unsigned)strlen(slot_dirs[slot_platform]),Slot_NextEntry,0);SLOT_DIAG_END(11);
            SLOT_DIAG_BEGIN(12);slot_host->f_closedir(&slot_dir);SLOT_DIAG_END(12);
            Slot_DiagBlock(0,13,slot_shelf.count?DIAG_GREEN:DIAG_RED);
        }
        Slot_DiagBlock(2,0,DIAG_BLUE);            /* row 2: press A to continue to the carousel */
        while(!(slot_host->get_keypress()&SLOT_KEY_A))Slot_WaitFrame();
        SLOT_DIAG_BEGIN(14);Slot_Present();SLOT_DIAG_END(14);   /* overwritten by the copy if it works */
    }
#endif
    Slot_ShowReading();SLOT_DISPCNT=SLOT_MODE3_BG2;Slot_FadeIn();
    Slot_ReadConfig();Slot_CheckLibrary();
    Slot_Scan();
    if(!slot_count && slot_library_state==0) {          /* land on a shelf that has games */
        unsigned original=slot_platform,n;
        for(n=1;n<3 && !slot_count;n++){slot_platform=(original+n)%3;Slot_Scan();}
        if(!slot_count){slot_platform=original;Slot_Scan();}
    }
    Slot_Show((int)Slot_Index(),0,1);
    Slot_ResumeNav(slot_count,Slot_Index(),slot_wrap,slot_animations);
    redraw=0;last_base=(int)Slot_Index();last_fill=last_base;
    while(1) {
        unsigned down;int base,fraction,modal=0;
        down=slot_host->get_keypress();
        Slot_SyncTicks();
        Slot_NavSnapshot(&nav_index,&pose,&steps);
        if(Slot_Index()!=nav_index){Slot_SetIndex(nav_index);Slot_ReadTitle();slot_footer_dirty=1;}
        if(down&SLOT_KEY_SELECT){
            Slot_SaveConfig();Slot_ClearNav();
            return SLOT_STOCK_UI;
        }
        if(slot_library_state==0 && (down&(SLOT_KEY_L|SLOT_KEY_R))){
            Slot_ChangeShelf(down&SLOT_KEY_L?-1:1);modal=1;
        } else if(down&SLOT_KEY_A){
            if(Slot_Play())return SLOT_LAUNCH;
            slot_partial_motion=0;modal=1;
        }
        if(modal){Slot_ResumeNav(slot_count,Slot_Index(),slot_wrap,slot_animations);nav_index=Slot_Index();pose=0;redraw=1;last_fill=-1;}
        base=(int)nav_index+(pose>>8);fraction=pose-(pose>>8)*256;
        if(redraw || pose!=last_pose || base!=last_base) {
            Slot_Show(base,fraction,redraw);
            redraw=0;last_pose=pose;last_base=base;
        } else Slot_WaitFrame();
        /* Keep the neighbours' labelled carts ready; cheap when nothing changed. */
        if(slot_count){int fill=base+(fraction>=128);if(fill!=last_fill){Slot_FillCacheAround(fill);last_fill=fill;}}
    }
}

int slot_main(const struct slot_host_api *api)
{
    uint8_t *scratch=(uint8_t*)api->scratch;
    int result;
    if(api->scratch_size<SLOT_SCRATCH_MIN || ((uintptr_t)scratch&3))return SLOT_STOCK_UI;
    slot_host=api;
    slot_frame=(uint16_t*)scratch;
    slot_cache_base=(uint16_t*)(scratch+SLOT_FRAME_BYTES);
    if(!slot_shelf_init(&slot_shelf,scratch+SLOT_FRAME_BYTES+SLOT_CACHE_BYTES,
                        api->scratch_size-SLOT_FRAME_BYTES-SLOT_CACHE_BYTES))return SLOT_STOCK_UI;
    Slot_TimingInit(api);
    result=Slot_Run(api->mailbox->entry_reason==SLOT_ENTER_BOOT);
    Slot_FadeReset();            /* the firmware rewrites the rest of the video state */
    return result;
}

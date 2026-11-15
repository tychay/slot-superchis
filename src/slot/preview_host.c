/*
 * slot. superchis: stub host for the emulator preview ROM (slot-preview.gba, task 5.1, design D6).
 *
 * A DEVELOPER TOOL, not firmware.  It plays the firmware's role for the same carousel code
 * (the files in src/slot/), with an in-memory fictional library instead of an SD card, so the carousel runs
 * in a stock emulator and tools/preview_shots.sh can screenshot it.  There is no SD, SDRAM-mode
 * or flash access.  What it stubs:
 *   - FatFs: directories /Games/GBA|GB|GBC with fictional titles (plus decoys the scan must
 *     ignore), f_stat("/Games"), and a three-file in-memory store for /SYSTEM/SLOT/SETTINGS.*;
 *   - SLOT_STOCK_UI -> a solid Mode 4 "stock UI" screen; Select goes back to the carousel
 *     (like Select on Settings/UI/Tools/Info in the firmware);
 *   - SLOT_LAUNCH   -> a stub "load popup" screen showing the launch path; B goes back to the
 *     carousel (like B in SuperFW's load popup).
 *
 * Library mode is chosen by the keys held while the ROM starts (the harness: "hold:Up:20,..."):
 *   nothing  normal: ~8 fictional titles per shelf
 *   Up       stress: 3,000 fictional titles on the GBA shelf (some 240-char, some UTF-8, some too
 *            long to launch)
 *   Down     empty: every shelf is empty after filtering
 *   Left     no /Games folder
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <string.h>

#include "slot/slot_art.h"
#include "slot/slot_host_api.h"

#define IO16(off) (*(volatile uint16_t *)(0x04000000u + (off)))
#define REG_DISPSTAT IO16(0x004)
#define REG_IE       IO16(0x200)
#define REG_IF       IO16(0x202)
#define REG_IME      IO16(0x208)
#define REG_KEYINPUT IO16(0x130)
#define IRQ_VECTOR   (*(volatile uint32_t *)0x03007FFC)
#define VRAM16       ((volatile uint16_t *)0x06000000)
#define PALETTE      ((volatile uint16_t *)0x05000000)

#define HOSTBSS __attribute__((section(".hostbss")))

extern uint8_t __scratch_start[], __scratch_size[];
int slot_entry(const struct slot_host_api *api);

/* ------------------------------------------------------------------ frame counter, input */
static volatile unsigned HOSTBSS frames;

__attribute__((target("arm"))) static void irq_handler(void) {
  REG_IF = 0xFFFF;
  frames++;
}

static void wait_vblank(void) {
  while (!(REG_DISPSTAT & 1));
}

static uint16_t HOSTBSS prev_keys;
static uint16_t keypress(void) {         /* newly pressed, no auto-repeat */
  uint16_t cur = ~REG_KEYINPUT & 0x3FF, n = cur & ~prev_keys;
  prev_keys = cur;
  return n;
}

/* ------------------------------------------------------------------ fictional library */
enum { MODE_NORMAL, MODE_STRESS, MODE_EMPTY, MODE_NOGAMES };
static unsigned HOSTBSS mode;

#define STRESS_COUNT 3000u

static const char *const titles[3][8] = {
  { "Aurora Quest", "Brick Breaker Advance", "captain comet", "Dungeon of Echoes",
    "Emerald Skies", "Frostbyte Racing", "Galaxy Gardener", "Harbor Heist" },
  { "Pocket Pinball", "Tiny Tennis", "Block Drop", "Moon Patrol Jr",
    "Sky Shooter", "Wizard Maze", "Frog Hop", "Tank Wars" },
  { "Crystal Caverns", "Neon Racer", "Pixel Farm", "Rhythm Rabbit",
    "Space Pirates", "Turbo Kart", "Zen Garden", "Candy Quest" },
};
static const char *const exts[3] = { ".gba", ".gb", ".gbc" };

/* Entries the scan must reject: hidden, AppleDouble, wrong extension, a folder, system. */
#define DECOYS 5

static unsigned put(char *out, unsigned n, const char *s) {
  while (*s && n < 255) out[n++] = *s++;
  return n;
}
static unsigned put_num4(char *out, unsigned n, unsigned v) {
  for (unsigned d = 1000; d; d /= 10) out[n++] = (char)('0' + (v / d) % 10);
  return n;
}

/* Fill *fi with entry `i` of directory `dir` (0..2 = GBA/GB/GBC); returns 0 past the end. */
static int dir_entry(unsigned dir, unsigned i, FILINFO *fi) {
  char *name = fi->fname;
  unsigned n = 0;
  fi->fattrib = 0;
  fi->fsize = (1u + (i * 37u) % 31u) * 131072u;
  unsigned total = (mode == MODE_EMPTY ? 0 : (mode == MODE_STRESS && dir == 0) ? STRESS_COUNT : 8) + DECOYS;
  if (i >= total) return 0;
  if (i < DECOYS) {                                        /* decoys first, so sorting is exercised */
    switch (i) {
    case 0: n = put(name, 0, ".hidden"); n = put(name, n, exts[dir]); break;
    case 1: n = put(name, 0, "._Aurora Quest"); n = put(name, n, exts[dir]); break;
    case 2: n = put(name, 0, "notes.txt"); break;
    case 3: n = put(name, 0, "Saves"); fi->fattrib = AM_DIR; break;
    default: n = put(name, 0, "System Volume"); n = put(name, n, exts[dir]); fi->fattrib = AM_SYS; break;
    }
  } else if (mode == MODE_STRESS && dir == 0) {
    unsigned k = ((i - DECOYS) * 7919u + 13u) % STRESS_COUNT;   /* scrambled directory order */
    if (k % 211 == 5) {                                    /* too long to launch: path > 255 */
      for (n = 0; n < 252 - 4; n++) name[n] = (char)('a' + (n + k) % 26);
      n = put(name, n, ".gba");
    } else if (k % 197 == 13) {                            /* long but launchable */
      n = put(name, 0, "Long Title ");
      n = put_num4(name, n, k);
      while (n < 240 - 4) { n = put(name, n, " lorem ipsum"); }
      n = 240 - 4;
      n = put(name, n, ".gba");
    } else if (k % 57 == 3) {                              /* non-ASCII UTF-8 */
      n = put(name, 0, "Pok\xC3\xA9mon \xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E ");
      n = put_num4(name, n, k);
      n = put(name, n, ".gba");
    } else {
      n = put(name, 0, "Game ");
      n = put_num4(name, n, k);
      n = put(name, n, ".gba");
    }
    fi->fsize = (1u + k % 31u) * 131072u;
  } else {
    unsigned t = i - DECOYS;
    n = put(name, 0, titles[dir][t]);
    n = put(name, n, (dir == 0 && t == 7) ? ".GBA" : exts[dir]);   /* mixed-case extension */
  }
  name[n] = 0;
  return 1;
}

/* ------------------------------------------------------------------ FatFs stand-ins */
#define FAKE_FILES 3
struct fake_file { uint16_t len; uint8_t exists; uint8_t data[512]; };
static struct fake_file HOSTBSS store[FAKE_FILES];
static const char *const store_paths[FAKE_FILES] = {
  "/SYSTEM/SLOT/SETTINGS.TXT", "/SYSTEM/SLOT/SETTINGS.TMP", "/SYSTEM/SLOT/SETTINGS.BAK",
};
/* FIL/DIR are FatFs' own types; the stub keeps its state in fields the payload never looks at. */
#define FIL_SLOT(fp)  ((fp)->clust)
#define FIL_SIZE(fp)  ((fp)->dir_sect)
#define DIR_ID(dp)    ((dp)->clust)
#define DIR_POS(dp)   ((dp)->dptr)

static int store_find(const char *path) {
  for (int i = 0; i < FAKE_FILES; i++)
    if (!strcmp(path, store_paths[i])) return i;
  return -1;
}

static FRESULT pv_open(FIL *fp, const TCHAR *path, BYTE mode_flags) {
  int i = store_find(path);
  if (i < 0) return FR_NO_PATH;
  if (mode_flags & FA_CREATE_ALWAYS) {
    store[i].exists = 1;
    store[i].len = 0;
  } else if (!store[i].exists) {
    return FR_NO_FILE;
  }
  FIL_SLOT(fp) = (DWORD)i;
  fp->fptr = 0;
  FIL_SIZE(fp) = store[i].len;
  return FR_OK;
}
static FRESULT pv_close(FIL *fp) { (void)fp; return FR_OK; }
static FRESULT pv_read(FIL *fp, void *buff, UINT btr, UINT *br) {
  uint16_t len = store[FIL_SLOT(fp)].len;
  UINT n = fp->fptr < len ? len - (UINT)fp->fptr : 0;
  if (n > btr) n = btr;
  memcpy(buff, store[FIL_SLOT(fp)].data + fp->fptr, n);
  fp->fptr += n;
  *br = n;
  return FR_OK;
}
static FRESULT pv_write(FIL *fp, const void *buff, UINT btw, UINT *bw) {
  unsigned i = FIL_SLOT(fp);
  if (fp->fptr + btw > sizeof(store[i].data)) { *bw = 0; return FR_DENIED; }
  memcpy(store[i].data + fp->fptr, buff, btw);
  fp->fptr += btw;
  store[i].len = (uint16_t)fp->fptr;
  *bw = btw;
  return FR_OK;
}
static FRESULT pv_lseek(FIL *fp, FSIZE_t ofs) { fp->fptr = ofs; return FR_OK; }
static FRESULT pv_sync(FIL *fp) { (void)fp; return FR_OK; }
static FRESULT pv_opendir(DIR *dp, const TCHAR *path) {
  if (mode == MODE_NOGAMES) return FR_NO_PATH;
  if (!strcmp(path, "/Games/GBA")) DIR_ID(dp) = 0;
  else if (!strcmp(path, "/Games/GB")) DIR_ID(dp) = 1;
  else if (!strcmp(path, "/Games/GBC")) DIR_ID(dp) = 2;
  else return FR_NO_PATH;
  DIR_POS(dp) = 0;
  return FR_OK;
}
static FRESULT pv_closedir(DIR *dp) { (void)dp; return FR_OK; }
static FRESULT pv_readdir(DIR *dp, FILINFO *fno) {
  if (!dir_entry(DIR_ID(dp), DIR_POS(dp)++, fno)) fno->fname[0] = 0;
  return FR_OK;
}
static FRESULT pv_stat(const TCHAR *path, FILINFO *fno) {
  (void)fno;
  if (!strcmp(path, "/Games")) return mode == MODE_NOGAMES ? FR_NO_FILE : FR_OK;
  return FR_NO_FILE;
}
static FRESULT pv_mkdir(const TCHAR *path) { (void)path; return FR_OK; }
static FRESULT pv_rename(const TCHAR *from, const TCHAR *to) {
  int a = store_find(from), b = store_find(to);
  if (a < 0 || !store[a].exists) return FR_NO_FILE;
  if (b < 0) return FR_NO_PATH;
  if (store[b].exists) return FR_EXIST;
  store[b] = store[a];
  store[b].exists = 1;
  store[a].exists = 0;
  return FR_OK;
}
static FRESULT pv_unlink(const TCHAR *path) {
  int i = store_find(path);
  if (i < 0 || !store[i].exists) return FR_NO_FILE;
  store[i].exists = 0;
  return FR_OK;
}

/* ------------------------------------------------------------------ stub screens (Mode 4) */
static void text8(int x, int y, const char *s, uint8_t colour) {
  for (; *s; s++, x += 6) {
    unsigned c = (unsigned char)*s;
    if (c < 32 || c > 126) c = '?';
    for (int row = 0; row < 12; row++)
      for (int col = 0; col < 6; col++)
        if (slot_font[c][row] & (128 >> col)) {
          volatile uint16_t *p = &VRAM16[((y + row) * 240 + x + col) / 2];
          unsigned shift = ((x + col) & 1) * 8;
          *p = (uint16_t)((*p & ~(0xFFu << shift)) | ((unsigned)colour << shift));
        }
  }
}

static void stub_screen(const char *line1, const char *line2, const char *line3, const char *line4) {
  *(volatile uint16_t *)0x04000000 = 0x0404;      /* Mode 4, BG2: like the stock UI */
  PALETTE[0] = 0x4A52;                            /* steel blue-grey */
  PALETTE[1] = 0x7FFF;                            /* white */
  PALETTE[2] = 0x03FF;                            /* amber */
  for (unsigned i = 0; i < 240 * 160 / 2; i++) VRAM16[i] = 0;
  text8(12, 30, line1, 1);
  if (line2) text8(12, 56, line2, 2);
  if (line3) text8(12, 70, line3, 2);
  if (line4) text8(12, 130, line4, 1);
}

static void wait_key(uint16_t key) {
  for (;;) {
    wait_vblank();
    if (keypress() & key) return;
  }
}

/* ------------------------------------------------------------------ main */
static struct slot_mailbox HOSTBSS mailbox;

/* In .hostbss and filled at run time: slot_entry() re-copies the payload's .data on every entry. */
static struct slot_host_api HOSTBSS api;

void host_main(void) {
  /* The ROM's hardware setup: VBlank IRQ counting frames, like the firmware's main_gba(). */
  REG_DISPSTAT |= 8;
  IRQ_VECTOR = (uint32_t)(uintptr_t)irq_handler;
  REG_IF = 0xFFFF;
  REG_IE = 1;
  REG_IME = 1;

  /* Library mode from the keys held at power-on (sampled over the first 15 frames). */
  uint16_t held = 0;
  for (unsigned i = 0; i < 15; i++) { wait_vblank(); held |= ~REG_KEYINPUT & 0x3FF; while (REG_DISPSTAT & 1); }
  mode = (held & 0x40) ? MODE_STRESS : (held & 0x80) ? MODE_EMPTY : (held & 0x20) ? MODE_NOGAMES : MODE_NORMAL;
  prev_keys = ~REG_KEYINPUT & 0x3FF;

  api = (struct slot_host_api){
    .abi_version = SLOT_ABI_VERSION, .struct_size = sizeof(struct slot_host_api),
    .fil_size = sizeof(FIL), .dir_size = sizeof(DIR), .filinfo_size = sizeof(FILINFO),
    .f_open = pv_open, .f_close = pv_close, .f_read = pv_read, .f_write = pv_write,
    .f_lseek = pv_lseek, .f_sync = pv_sync, .f_opendir = pv_opendir, .f_closedir = pv_closedir,
    .f_readdir = pv_readdir, .f_stat = pv_stat, .f_mkdir = pv_mkdir, .f_rename = pv_rename,
    .f_unlink = pv_unlink,
    .get_keypress = keypress, .wait_for_vblank = wait_vblank, .frame_count = &frames,
  };
  api.mailbox = &mailbox;
  api.scratch = __scratch_start;
  api.scratch_size = (uint32_t)(uintptr_t)__scratch_size;
  mailbox.entry_reason = SLOT_ENTER_BOOT;

  for (;;) {
    mailbox.launch_size = 0;
    mailbox.launch_path[0] = 0;
    int act = slot_entry(&api);
    mailbox.entry_reason = SLOT_ENTER_RESUME;
    if (act == SLOT_LAUNCH) {
      char line[SLOT_MAX_PATH + 1];
      unsigned n = put(line, 0, "size ");
      n = put_num4(line, n, mailbox.launch_size / 1024);
      n = put(line, n, " KiB");
      line[n] = 0;
      /* the path wraps over two lines of 36 characters */
      char p1[40], p2[40];
      unsigned len = (unsigned)strlen(mailbox.launch_path), a = len > 36 ? 36 : len;
      unsigned b = len - a > 36 ? 36 : len - a;
      memcpy(p1, mailbox.launch_path, a); p1[a] = 0;
      memcpy(p2, mailbox.launch_path + a, b); p2[b] = 0;
      stub_screen("STUB LOAD POPUP (launch)", p1, len > 36 ? p2 : line, "B: cancel (back to the carousel)");
      wait_key(2);                              /* B */
    } else if (act == SLOT_STOCK_UI) {
      stub_screen("STUB STOCK UI", "(SuperFW's stock menu)", 0, "Select: back to the carousel");
      wait_key(4);                              /* Select */
    } else {
      stub_screen("PAYLOAD ERROR", 0, 0, 0);
      for (;;) wait_vblank();
    }
  }
}

__attribute__((naked, section(".boot"))) void boot(void) {
  __asm__(".arm\n b 1f\n .space 0xBC\n 1: ldr sp, =0x03007F00\n ldr r0, =host_main\n bx r0\n .pool\n");
}

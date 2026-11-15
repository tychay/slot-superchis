/*
 * slot. superchis: ABI between the firmware (host) and slot.payload (ADR-0004).
 *
 * The payload never links against firmware symbols; everything it needs from the firmware
 * is in struct slot_host_api, passed to slot_main().  Any layout change MUST bump
 * SLOT_ABI_VERSION; both sides check it (firmware: slot_host.c, payload: slot_entry()).
 *
 * Rules for the payload:
 *  - pointers handed to the FatFs entries must be in IWRAM/EWRAM/0x08xxxxxx (never 0x09xxxxxx).
 *  - it runs with the stock IRQ setup (VBlank IRQ on); it must not change IE/IME/DISPSTAT IRQ
 *    bits, must leave the SD mapping alone, must restore WAITCNT if it changes it, and must not
 *    touch OAM or OBJ VRAM (0x06014000+) or the SDRAM outside its window.
 *  - it owns the screen while it runs (Mode 3); the firmware re-initialises the stock video
 *    state after it returns (menu_slot_restore_video()).
 *  - ARM/thumb: slot_main is called through the header's entry address, thumb bit set.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef SLOT_HOST_API_H
#define SLOT_HOST_API_H

#include <stdint.h>
#include <stddef.h>
#include "fatfs/ff.h"
#include "slot/slot_layout.h"

#define SLOT_ABI_VERSION        2     /* v2: f_unlink, mailbox->entry_reason */

/* Action codes returned by slot_main() */
#define SLOT_STOCK_UI           0     /* leave the carousel, show the stock SuperFW UI */
#define SLOT_LAUNCH             1     /* launch mailbox->launch_path (size in mailbox->launch_size) */
#define SLOT_ERR_ABI           (-1)   /* payload refused the host's ABI (version/size mismatch) */

#define SLOT_MAX_PATH           256   /* == MAX_FN_LEN */

/* Why the firmware is entering the payload (mailbox->entry_reason). */
#define SLOT_ENTER_BOOT         0     /* first entry after power-on: show the splash */
#define SLOT_ENTER_RESUME       1     /* back from the stock UI or a dismissed load popup */

/* Lives at SLOT_MAILBOX_ADDR; the firmware sets entry_reason, the payload sets the launch fields. */
struct slot_mailbox {
  uint32_t launch_size;               /* file size in bytes, for SLOT_LAUNCH */
  char     launch_path[SLOT_MAX_PATH];/* absolute path on the SD card, NUL terminated */
  uint32_t entry_reason;              /* SLOT_ENTER_* */
};

struct slot_host_api {
  uint32_t abi_version;               /* SLOT_ABI_VERSION of the firmware */
  uint32_t struct_size;               /* sizeof(struct slot_host_api) of the firmware */
  uint32_t fil_size;                  /* sizeof(FIL), sizeof(DIR), sizeof(FILINFO) as built */
  uint32_t dir_size;                  /*   by the firmware, so the payload can verify its  */
  uint32_t filinfo_size;              /*   FatFs types match.                              */

  /* FatFs (firmware's instance; drive "0:") */
  FRESULT (*f_open)(FIL *fp, const TCHAR *path, BYTE mode);
  FRESULT (*f_close)(FIL *fp);
  FRESULT (*f_read)(FIL *fp, void *buff, UINT btr, UINT *br);
  FRESULT (*f_write)(FIL *fp, const void *buff, UINT btw, UINT *bw);
  FRESULT (*f_lseek)(FIL *fp, FSIZE_t ofs);
  FRESULT (*f_sync)(FIL *fp);
  FRESULT (*f_opendir)(DIR *dp, const TCHAR *path);
  FRESULT (*f_closedir)(DIR *dp);
  FRESULT (*f_readdir)(DIR *dp, FILINFO *fno);
  FRESULT (*f_stat)(const TCHAR *path, FILINFO *fno);
  FRESULT (*f_mkdir)(const TCHAR *path);
  FRESULT (*f_rename)(const TCHAR *path_old, const TCHAR *path_new);
  FRESULT (*f_unlink)(const TCHAR *path);

  /* Input / timing */
  uint16_t (*get_keypress)(void);     /* newly pressed keys, GBA KEYINPUT bit order: A=0x1 B=0x2
                                         Select=0x4 Start=0x8 Right=0x10 Left=0x20 Up=0x40
                                         Down=0x80 R=0x100 L=0x200 (gbahw.h KEY_BUTT*);
                                         auto-repeat on A/B/dpad; call once per frame */
  void     (*wait_for_vblank)(void);  /* spin until the VBlank flag is set */
  const volatile unsigned *frame_count; /* incremented by the VBlank IRQ */

  /* Memory */
  struct slot_mailbox *mailbox;       /* SLOT_MAILBOX_ADDR */
  void    *scratch;                   /* SLOT_SCRATCH_ADDR: big payload buffers (SDRAM) */
  uint32_t scratch_size;              /* SLOT_SCRATCH_SIZE */
};

/*
 * Header at offset 0 of slot.payload (SLOT_PAYLOAD_ADDR).  The firmware validates it, then
 * calls entry(api).  entry (slot_entry) checks the ABI, copies the .iwram section from its
 * load address into IWRAM, zeroes its statics, calls slot_main() and returns its result.
 */
#define SLOT_PAYLOAD_MAGIC      0x544F4C53u   /* "SLOT" little endian */

struct slot_payload_header {
  uint32_t magic;                     /* SLOT_PAYLOAD_MAGIC */
  uint32_t abi_version;               /* SLOT_ABI_VERSION the payload was built against */
  uint32_t image_size;                /* bytes copied by rom_boot: header+text+rodata+iwram LMA */
  uint32_t entry;                     /* int slot_entry(const struct slot_host_api *), |1 */
  uint32_t link_base;                 /* address the image is linked at == SLOT_PAYLOAD_ADDR */
  uint32_t iwram_base;                /* where .iwram runs; must equal SLOT_IWRAM_BASE */
  uint32_t iwram_size;                /* bytes; <= SLOT_IWRAM_SIZE */
  uint32_t reserved;
};

/* Implemented by the payload (src/slot/). */
int slot_main(const struct slot_host_api *api);

#endif

/*
 * slot. superchis: firmware side of the carousel payload (ADR-0004, design D2/D3/D4).
 * Builds the host ABI table, validates and runs slot.payload from its SDRAM window, and
 * turns its action codes into stock UI behaviour.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <string.h>

#include "common.h"
#include "gbahw.h"
#include "slot/slot_host.h"
#include "slot/slot_host_api.h"

// Mirrors the value main_gba() programs before the menu starts (src/main.c).
#define SLOT_STOCK_WAITCNT   0x40c0

static bool slot_wanted;     // Select pressed on a stock tab: go back to the carousel
static bool slot_resume;     // a carousel-initiated popup is open: resume when dismissed
static bool slot_booted;     // the payload has been entered at least once since power-on
static bool slot_from_carousel;  // the file now being opened was picked in the carousel (task 4.9)

static void slot_wait_vblank(void) {
  while (!(REG_DISPSTAT & DISPSTAT_VBLANK));
}

static const struct slot_host_api slot_api = {
  .abi_version  = SLOT_ABI_VERSION,
  .struct_size  = sizeof(struct slot_host_api),
  .fil_size     = sizeof(FIL),
  .dir_size     = sizeof(DIR),
  .filinfo_size = sizeof(FILINFO),
  .f_open     = f_open,
  .f_close    = f_close,
  .f_read     = f_read,
  .f_write    = f_write,
  .f_lseek    = f_lseek,
  .f_sync     = f_sync,
  .f_opendir  = f_opendir,
  .f_closedir = f_closedir,
  .f_readdir  = f_readdir,
  .f_stat     = f_stat,
  .f_mkdir    = f_mkdir,
  .f_rename   = f_rename,
  .f_unlink   = f_unlink,
  .get_keypress    = get_keypress,
  .wait_for_vblank = slot_wait_vblank,
  .frame_count     = &frame_count,
  .mailbox      = (struct slot_mailbox *)SLOT_MAILBOX_ADDR,
  .scratch      = (void *)SLOT_SCRATCH_ADDR,
  .scratch_size = SLOT_SCRATCH_SIZE,
};

// Returns a SLOT_* action code. Anything unusable (no payload, bad header, ABI mismatch)
// degrades to SLOT_STOCK_UI so the stock firmware always remains reachable.
static int slot_run_payload(void) {
  const struct slot_payload_header *h = (const struct slot_payload_header *)SLOT_PAYLOAD_ADDR;
  if (h->magic != SLOT_PAYLOAD_MAGIC || h->abi_version != SLOT_ABI_VERSION ||
      h->link_base != SLOT_PAYLOAD_ADDR || h->iwram_base != SLOT_IWRAM_BASE ||
      h->iwram_size > SLOT_IWRAM_SIZE || h->image_size > SLOT_PAYLOAD_MAX ||
      !(h->entry & 1) || h->entry < SLOT_PAYLOAD_ADDR || h->entry >= SLOT_PAYLOAD_ADDR + h->image_size)
    return SLOT_STOCK_UI;

  struct slot_mailbox *mb = slot_api.mailbox;
  mb->launch_size = 0;
  *(volatile uint16_t *)&mb->launch_path[0] = 0;   // slot: SDRAM takes no byte stores (ADR-0010)
  mb->entry_reason = slot_booted ? SLOT_ENTER_RESUME : SLOT_ENTER_BOOT;
  slot_booted = true;
  slot_from_carousel = false;

  int (*entry)(const struct slot_host_api *) = (int (*)(const struct slot_host_api *))h->entry;
  int act = entry(&slot_api);

  // The payload owns video while it runs; put the stock state back (D4).
  REG_WAITCNT = SLOT_STOCK_WAITCNT;
  menu_slot_restore_video();

  if (act == SLOT_LAUNCH && mb->launch_path[0]) {
    *(volatile uint16_t *)&mb->launch_path[SLOT_MAX_PATH - 2] = 0;   // never trust the payload's terminator (halfword, ADR-0010)
    return SLOT_LAUNCH;
  }
  return SLOT_STOCK_UI;
}

void slot_enter(void) {
  slot_wanted = false;
  slot_resume = false;
  while (slot_run_payload() == SLOT_LAUNCH) {
    // Same code path as picking the file in the stock browser: shows the load popup
    // (or starts the GB emulator). If it did not open anything, go back to the carousel.
    slot_from_carousel = true;
    menu_open_file(slot_api.mailbox->launch_path, slot_api.mailbox->launch_size);
    if (menu_popup_active()) {
      slot_resume = true;     // slot_poll() re-enters once the popup is dismissed
      break;
    }
  }
}

// True while a file picked in the carousel is being opened: SuperFW must not add it to its
// shared recent list (.superfw/recent.txt), so this fork's own .gba stays on top of stock
// SuperFW's Recent tab and returning to the fork is power-on -> A (task 4.9).
bool slot_recent_suppressed(void) {
  return slot_from_carousel;
}

void slot_request_carousel(void) {
  slot_wanted = true;
}

bool slot_poll(void) {
  if (slot_wanted || (slot_resume && !menu_popup_active())) {
    slot_enter();
    return true;
  }
  return false;
}

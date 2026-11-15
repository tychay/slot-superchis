/*
 * slot. superchis: throwaway host for smoke-testing slot.payload in an emulator (task 3.4/3.6).
 * Plays the firmware's role: validates the payload header at its SDRAM address, calls entry()
 * with a host API table (no SD, so the FatFs entries are NULL), then shows the result as a
 * solid colour (blue = SLOT_STOCK_UI as expected, red = anything else).
 * `make slot-smoke.gba` links this at 0x08000000 and appends slot.payload at SLOT_OFF_PAYLOAD.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "slot/slot_host_api.h"

#define REG_DISPCNT  (*(volatile uint16_t *)0x04000000)
#define REG_DISPSTAT (*(volatile uint16_t *)0x04000004)
#define REG_KEYINPUT (*(volatile uint16_t *)0x04000130)
#define VRAM         ((volatile uint16_t *)0x06000000)

static void wait_vblank(void) {
  while (!(REG_DISPSTAT & 1));      // wait for VBlank flag...
  while (REG_DISPSTAT & 1);         // ...and then for it to end so each call is one frame
}

static uint16_t prev_keys;
static uint16_t keypress(void) {
  uint16_t cur = ~REG_KEYINPUT & 0x3FF, n = cur & ~prev_keys;
  prev_keys = cur;
  return n;
}

static volatile unsigned fake_frames;
static struct slot_host_api api = {
  .abi_version = SLOT_ABI_VERSION, .struct_size = sizeof(struct slot_host_api),
  .fil_size = sizeof(FIL), .dir_size = sizeof(DIR), .filinfo_size = sizeof(FILINFO),
  .get_keypress = keypress, .wait_for_vblank = wait_vblank, .frame_count = &fake_frames,
  .mailbox = (struct slot_mailbox *)SLOT_MAILBOX_ADDR,
  .scratch = (void *)SLOT_SCRATCH_ADDR, .scratch_size = SLOT_SCRATCH_SIZE,
};

static void show(uint16_t color) {
  REG_DISPCNT = 0x0403;
  for (unsigned i = 0; i < 240 * 160; i++) VRAM[i] = color;
}

void smoke_main(void) {
  const struct slot_payload_header *h = (const struct slot_payload_header *)SLOT_PAYLOAD_ADDR;
  int r = -100;
  if (h->magic == SLOT_PAYLOAD_MAGIC && h->abi_version == SLOT_ABI_VERSION &&
      h->link_base == SLOT_PAYLOAD_ADDR && h->iwram_base == SLOT_IWRAM_BASE &&
      h->iwram_size <= SLOT_IWRAM_SIZE && (h->entry & 1))
    r = ((int (*)(const struct slot_host_api *))h->entry)(&api);
  show(r == SLOT_STOCK_UI ? 0x7C00 /* blue */ : 0x001F /* red */);
  for (;;) ;
}

__attribute__((naked, section(".boot"))) void boot(void) {
  __asm__(".arm\n b 1f\n .space 0xBC\n 1: ldr r0, =smoke_main\n bx r0\n .pool\n");
}

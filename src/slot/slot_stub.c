/*
 * slot. superchis: stub slot_main() for task 3.4 (replaced by the carousel in section 4).
 * Fills the screen with one colour in Mode 3, waits for any key, returns SLOT_STOCK_UI.
 * The fill loop lives in the .iwram_code section on purpose: it proves the payload's
 * IWRAM copy-in works on hardware (the colour only appears if it ran from IWRAM).
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "slot/slot_host_api.h"

#define REG_DISPCNT   (*(volatile uint16_t *)0x04000000)
#define VRAM_MODE3    ((volatile uint32_t *)0x06000000)
#define MODE3_BG2     0x0403
#define STUB_COLOR    0x02FF   /* orange, BGR555 */

__attribute__((section(".iwram_code"), target("arm"), long_call, noinline))
static void fill_mode3(uint16_t color) {
  uint32_t c = color | ((uint32_t)color << 16);
  for (unsigned i = 0; i < 240 * 160 / 2; i++)
    VRAM_MODE3[i] = c;
}

int slot_main(const struct slot_host_api *api) {
  REG_DISPCNT = MODE3_BG2;
  fill_mode3(STUB_COLOR);
  for (;;) {
    api->wait_for_vblank();
    if (api->get_keypress())
      break;
  }
  return SLOT_STOCK_UI;
}

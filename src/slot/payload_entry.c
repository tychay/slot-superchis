/*
 * slot. superchis: slot.payload entry (header + crt).  Built freestanding: no libc, no firmware
 * symbols (ADR-0004).  See ldscripts/slot_payload.ld for the image layout.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "slot/slot_host_api.h"

// Linker-provided. The *_size / __image_size symbols are absolute values, not addresses.
extern uint32_t __iwram_start[], __iwram_lma[], __iwram_size[];
extern uint32_t __data_start[], __data_lma[], __data_size[];
extern uint32_t __bss_start[], __bss_size[];
extern uint32_t __image_size[];

int slot_entry(const struct slot_host_api *api);

__attribute__((section(".slot_header"), used))
const struct slot_payload_header slot_header = {
  .magic       = SLOT_PAYLOAD_MAGIC,
  .abi_version = SLOT_ABI_VERSION,
  .image_size  = (uint32_t)(uintptr_t)__image_size,
  .entry       = (uint32_t)(uintptr_t)slot_entry,       // thumb function: bit 0 is set by the linker
  .link_base   = SLOT_PAYLOAD_ADDR,
  .iwram_base  = SLOT_IWRAM_BASE,
  .iwram_size  = (uint32_t)(uintptr_t)__iwram_size,
};

// Word copy/zero loops; volatile-free but kept out of memcpy/memset idiom recognition by
// -fno-tree-loop-distribute-patterns (Makefile) so no libc symbol is needed.
static void copy_words(uint32_t *dst, const uint32_t *src, uint32_t bytes) {
  for (uint32_t i = 0; i < bytes / 4; i++)
    dst[i] = src[i];
}

static void zero_words(uint32_t *dst, uint32_t bytes) {
  for (uint32_t i = 0; i < bytes / 4; i++)
    dst[i] = 0;
}

int slot_entry(const struct slot_host_api *api) {
  if (api->abi_version != SLOT_ABI_VERSION ||
      api->struct_size != sizeof(struct slot_host_api) ||
      api->fil_size != sizeof(FIL) || api->dir_size != sizeof(DIR) ||
      api->filinfo_size != sizeof(FILINFO))
    return SLOT_ERR_ABI;

  copy_words(__iwram_start, __iwram_lma, (uint32_t)(uintptr_t)__iwram_size);
  copy_words(__data_start,  __data_lma,  (uint32_t)(uintptr_t)__data_size);
  zero_words(__bss_start, (uint32_t)(uintptr_t)__bss_size);

  return slot_main(api);
}

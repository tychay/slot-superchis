/*
 * slot. superchis: firmware side of the carousel payload (ADR-0004).
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef SLOT_HOST_H
#define SLOT_HOST_H

#include <stdbool.h>
#include <stdint.h>

// Run the carousel (blocks until it hands control to the stock UI or to a launch popup).
// Safe to call with no/invalid payload: it then returns immediately (stock UI stays).
void slot_enter(void);
// Called once per stock main-loop iteration: re-enters the carousel when requested.
bool slot_poll(void);    // true if the carousel ran (caller should resync its frame timer)
// Ask the main loop to re-enter the carousel (Select on a top-level stock tab).
void slot_request_carousel(void);

// Provided by main.c / menu.c (upstream files; thin hooks marked `// slot:`).
void setup_video(void);
void menu_open_file(const char *path, uint32_t size);
bool menu_popup_active(void);
void menu_slot_restore_video(void);

#endif

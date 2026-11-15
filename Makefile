
VERSION_WORD := 0x00000016
VERSION_SLUG_WORD := $(shell git rev-parse --short=8 HEAD || echo FFFFFFFF)
# slot: fork milestone, shown on the Info screen as "slot.<milestone>". Bump here only.
SLOT_MILESTONE := M2
SLOT_MAX_IMAGE_KB ?= 2048

PREFIX		:= arm-none-eabi-
CC		:= $(PREFIX)gcc
CXX		:= $(PREFIX)g++
OBJDUMP		:= $(PREFIX)objdump
OBJCOPY		:= $(PREFIX)objcopy

COMPRESSION_RATIO ?= 4

GLOBAL_DEFINES = -D__GBA__

# BOARD can be "sd", "lite", "chis"
BOARD ?= sd

ifeq ($(BOARD),lite)
  GLOBAL_DEFINES += -DSUPERCARD_LITE_IO
  COMPRESS_FIRMWARE = 1
  MAXFSIZE = 496
  FWFLAVOUR = "Lite"
else ifeq ($(BOARD),sd)
  GLOBAL_DEFINES += -DSUPERCARD_FLASH_ADDRPERM
  BUNDLE_GBC_EMULATOR = 1
  COMPRESS_FIRMWARE = 1
  MAXFSIZE = 512
  FWFLAVOUR = "SD"
else ifeq ($(BOARD),chis)
  GLOBAL_DEFINES += -DSUPPORT_NORGAMES -DSUPERCHIS_IO -DFONTS_EXT
  BUNDLE_GBC_EMULATOR = 1
  BUNDLE_OTHER_EMULATORS = 1
  # Can't be over 2MiB (hardlimit)
  MAXFSIZE = 2048
  FWFLAVOUR = "Chis"
else
  $(error No valid board specified in BOARD)
endif

FWBINFILES=firmware.ewram.gba res/patches.db res/fonts.pack

ifeq ($(ENABLE_DISK_LOGGING),1)
  PAYLOADFLAGS += -DENABLE_DISK_LOGGING
endif
ifeq ($(ENABLE_EMU_LOGGING),1)
  PAYLOADFLAGS += -DENABLE_EMU_LOGGING
endif
ifeq ($(ENABLE_UART_LOGGING),1)
  PAYLOADFLAGS += -DENABLE_UART_LOGGING
endif

ifeq ($(COMPRESS_FIRMWARE),1)
  GLOBAL_DEFINES += -DCOMPRESS_FONTS -DCOMPRESS_PATCHES -DCOMPRESS_FIRMWARE
  FWBINFILES := $(addsuffix .comp,$(FWBINFILES))
endif

ifeq ($(BUNDLE_GBC_EMULATOR),1)
  GLOBAL_DEFINES += -DBUNDLE_GBC_EMULATOR
  BIEMUFILES += emu/jagoombacolor_v0.5.gba.comp
endif

ifeq ($(BUNDLE_OTHER_EMULATORS),1)
  GLOBAL_DEFINES += -DBUNDLE_OTHER_EMULATOR
  BIEMUFILES += emu/pocketnes_20130701.gba.comp \
                emu/wasabigba_v0.2.4.gba.comp \
                emu/NGPGBA_v0.5.7.gba.comp \
                emu/pceadvance-v7.5-scptch.gba.comp \
                emu/smsadvance-v2.5-scptch.gba.comp
endif

BASEFLAGS=$(GLOBAL_DEFINES) -mcpu=arm7tdmi -mtune=arm7tdmi

CFLAGS=-O2 -ggdb \
       $(BASEFLAGS) $(PAYLOADFLAGS) \
       -DFW_MAX_SIZE_KB=$(MAXFSIZE) -DFW_FLAVOUR="\"$(FWFLAVOUR)\"" \
       -DSC_FAST_ROM_MIRROR="use_fast_mirror()" \
       -DSD_PREERASE_BLOCKS_WRITE \
       -DVERSION_WORD="$(VERSION_WORD)" \
       -DVERSION_SLUG_WORD="0x$(VERSION_SLUG_WORD)" \
       -DSLOT_VERSION_SUFFIX="\"slot.$(SLOT_MILESTONE)\"" \
       -Wall -Isrc -I. -mthumb -flto -flto-partition=none


INGAME_CFLAGS=-Os -ggdb \
              $(BASEFLAGS) \
              -DNO_SUPERCARD_INIT \
              -DSD_PREERASE_BLOCKS_WRITE \
              -Wall -Isrc -I. \
              -mthumb -flto

DLDI_CFLAGS=-Os -ggdb \
            $(BASEFLAGS) -DSDDRV_TIMEOUT_MULT=2 \
            -DSD_PREERASE_BLOCKS_WRITE \
            -Wall -Isrc -I. \
            -mthumb -flto -fPIC

DIRECTSAVE_CFLAGS=-Os -ggdb \
                 $(BASEFLAGS) -DSDDRV_TIMEOUT_MULT=16 \
                 -DNO_SUPERCARD_INIT \
                 -Wall -Isrc -I. \
                 -marm -flto -fPIC

# slot: the carousel payload (ADR-0004). Freestanding thumb code at a fixed SDRAM address; links no
# firmware symbols and no libc (-nostdlib turns any stray libc/firmware reference into a link error);
# only libgcc (integer division, 64-bit multiply) is linked, see src/slot/slot_rt.c.
SLOT_CFLAGS=-Os -ggdb \
            $(BASEFLAGS) \
            -Wall -Wextra -Isrc -I. \
            -mthumb -ffreestanding -fno-builtin -fno-tree-loop-distribute-patterns

# slot: hardware diagnostic payload (src/slot/slot_diag.h): make SLOT_DIAG=1
ifeq ($(SLOT_DIAG),1)
SLOT_CFLAGS += -DSLOT_DIAG
endif

SLOT_PAYLOAD_OFFSET=12058624   # == SLOT_OFF_PAYLOAD (0x00B80000, src/slot/slot_layout.h)

SLOTFILES=src/slot/payload_entry.c \
          src/slot/slot_rt.c \
          src/slot/slot_scan.c \
          src/slot/slot_settings.c \
          src/slot/slot_frontend.c \
          src/slot/slot_blit.s

SLOTHEADERS=$(wildcard src/slot/*.h)

FATFSFILES=fatfs/diskio.c \
           fatfs/ff.c \
           fatfs/ffsystem.c \
           fatfs/ffunicode.c

DLDIFILES=src/dldi.S \
          src/dldi_driver.c \
          src/crc.c \
          src/supercard_io.S \
          src/supercard_driver.c

DIRECTSAVEFILES=src/directsaver.S \
                src/directsave_emu.c \
                src/crc.c \
                src/supercard_driver.c

MENUFILES=src/ingame.S \
          src/ingame_menu.c \
          src/cimpl.c \
          src/fonts/font_render.c \
          src/save.c \
          src/util.c \
          src/utf_util.c \
          src/fileutil.c \
          src/crc.c \
          src/nanoprintf.c \
          src/supercard_io.S \
          src/supercard_driver.c \
          ${FATFSFILES}

INFILES=src/gba_ewram_crt0.S \
        src/main.c \
        src/log.c \
        src/cimpl.c \
        src/settings.c \
        src/loader.c \
        src/save.c \
        src/patchengine.c \
        src/patcher.c \
        src/patches.S \
        src/menu.c \
        src/recent.c \
        src/cheats.c \
        src/flash.c \
        src/sha256.c \
        src/misc.c \
        src/util.c \
        src/utf_util.c \
        src/emu.c \
        src/fileutil.c \
        src/asmutil.S \
        src/gbahw.c \
        src/virtfs.c \
        src/flash_mgr.c \
        src/binassets.S \
        src/crc.c \
        src/nds_loader.c \
        src/dldi_patcher.c \
        src/supercard_driver.c \
        src/supercard_io.S \
        src/heapsort.c \
        src/nanoprintf.c \
        src/fonts/font_render.c \
        src/slot/slot_host.c \
        ${FATFSFILES}

all:	$(FWBINFILES) res/slot_assets.pack.comp $(BIEMUFILES) directsave.payload ingame_trampoline.payload slot.payload
	# Wrap the firmware around a ROM->EWRAM loader
	$(CC) $(CFLAGS) -o firmware.elf rom_boot.S -T ldscripts/gba_romboot.ld -nostartfiles -nostdlib -Wl,--defsym,MAX_FLASH_SIZE=$(MAXFSIZE)K
	$(OBJCOPY) --output-target=binary firmware.elf superfw.gba
	# Fix the header/checksum.
	./tools/fw-fixer.py superfw.gba
	# slot: size guard (ADR-0005), fail the build if the image exceeds SLOT_MAX_IMAGE_KB.
	@sz=$$(wc -c < superfw.gba); max=$$(( $(SLOT_MAX_IMAGE_KB) * 1024 )); \
	  if [ $$sz -gt $$max ]; then echo "slot: superfw.gba is $$sz bytes, exceeds $$max ($(SLOT_MAX_IMAGE_KB) KB)"; exit 1; fi

firmware.ewram.gba: $(INFILES) ingamemenu.payload superfw.dldi.payload directsave.payload ingame_trampoline.payload src/messages_data.h ldscripts/gba_ewram.ld.i
	# Build the actual firmware image
	$(CC) $(CFLAGS) -o firmware.ewram.elf $(INFILES) -T ldscripts/gba_ewram.ld.i -nostartfiles -Wl,-Map=firmware.ewram.map -Wl,--print-memory-usage -fno-builtin
	$(OBJCOPY) --output-target=binary firmware.ewram.elf firmware.ewram.gba

superfw.dldi.payload:	$(DLDIFILES)
	# Build in-game menu
	$(CC) $(DLDI_CFLAGS) -o superfw.dldi.elf $(DLDIFILES) -T ldscripts/gba_dldi.ld \
			-nostartfiles -fno-builtin -Wl,-Map=superfw.dldi.map -Wl,--print-memory-usage
	$(OBJCOPY) --output-target=binary superfw.dldi.elf superfw.dldi.payload

directsave.payload:	$(DIRECTSAVEFILES)
	$(CC) $(DIRECTSAVE_CFLAGS) -o directsave.elf $(DIRECTSAVEFILES) -T ldscripts/gba_directsave.ld \
			-nostartfiles -fno-builtin -Wl,-Map=directsave.map -Wl,--print-memory-usage
	$(OBJCOPY) --output-target=binary directsave.elf directsave.payload

ingamemenu.payload:	$(MENUFILES) src/menu_messages.h
	# Build in-game menu
	$(CC) $(INGAME_CFLAGS) -o ingamemenu.elf $(MENUFILES) -T ldscripts/gba_ingame.ld \
			-nostartfiles -fno-builtin -Wl,-Map=firmware.ingame.map -Wl,--print-memory-usage
	$(OBJCOPY) --output-target=binary ingamemenu.elf ingamemenu.payload

# slot: the art/sound asset pack (task 6.1), generated from the pinned submodule (read-only) at build
# time, apultra-compressed by the %.pack.comp rule and unpacked into the SDRAM window by rom_boot.S.
res/slot_assets.pack:	tools/slot_pack.py vendor/slot-advance/common/slot_assets.h vendor/slot-advance/tools/convert_sounds.py
	python tools/slot_pack.py vendor/slot-advance $@

# slot: carousel payload
slot.payload:	$(SLOTFILES) $(SLOTHEADERS) ldscripts/slot_payload.ld.i
	$(CC) $(SLOT_CFLAGS) -o slot.elf $(SLOTFILES) -T ldscripts/slot_payload.ld.i \
			-nostartfiles -nostdlib -Wl,-Map=slot.map -Wl,--print-memory-usage -lgcc
	$(OBJCOPY) --output-target=binary slot.elf slot.payload

ldscripts/slot_payload.ld.i ldscripts/gba_ewram.ld.i ldscripts/slot_preview.ld.i:	src/slot/slot_layout.h

# slot: emulator preview ROM (developer tool, not firmware; tasks 5.1/D6). The same carousel code
# linked against a stub host (src/slot/preview_host.c) into an ordinary ROM: the payload's writable
# window is not available in an emulator (ROM space is read-only), so the scratch
# buffers live in EWRAM (statics link into the same IWRAM window as on hardware, ADR-0010). Use with tools/preview_shots.sh.
preview:	slot-preview.gba

slot-preview.gba:	$(SLOTFILES) $(SLOTHEADERS) src/slot/preview_host.c src/slot/preview_art.S res/slot_assets.pack $(shell find tests/fixtures/art -name "*.bmp") ldscripts/slot_preview.ld.i
	$(CC) $(SLOT_CFLAGS) -DSLOT_PREVIEW -o slot-preview.elf $(SLOTFILES) src/slot/preview_host.c src/slot/preview_art.S -T ldscripts/slot_preview.ld.i \
			-nostartfiles -nostdlib -Wl,-Map=slot-preview.map -Wl,--print-memory-usage -lgcc
	$(OBJCOPY) --output-target=binary slot-preview.elf $@

# slot: host unit tests for the pure carousel modules (scan/sort/filter, settings). Run via
# ./build.sh slot-tests (host gcc + AddressSanitizer inside the build image).
SLOT_TEST_CFLAGS=-O0 -ggdb -Wall -Wextra -Isrc -I. -fsanitize=address,undefined -fno-omit-frame-pointer
slot-tests:
	mkdir -p build/tests
	gcc $(SLOT_TEST_CFLAGS) -o build/tests/slot_scan_test tests/slot_scan_test.c src/slot/slot_scan.c
	./build/tests/slot_scan_test
	gcc $(SLOT_TEST_CFLAGS) -o build/tests/slot_settings_test tests/slot_settings_test.c src/slot/slot_settings.c src/slot/slot_scan.c
	./build/tests/slot_settings_test
	# slot: Cart Studio art format (task 6.2): real converter output generated here + converted fictional labels (tests/make_fixture_labels.py).
	python3 tests/make_art_fixtures.py build/tests/art
	gcc $(SLOT_TEST_CFLAGS) -o build/tests/slot_bmp_test tests/slot_bmp_test.c
	./build/tests/slot_bmp_test build/tests/art tests/fixtures/art

ingame_trampoline.payload:	src/ingame_trampoline.S
	$(CC) $(BASEFLAGS) -nostartfiles -T ldscripts/gba_ingametramp.ld -o ingame_trampoline.elf src/ingame_trampoline.S
	$(OBJCOPY) --output-target=binary ingame_trampoline.elf ingame_trampoline.payload

src/messages_data.h:	res/messages.py
	./res/messages.py h main > src/messages_data.h

src/menu_messages.h:	res/messages.py
	./res/messages.py h menu > src/menu_messages.h

firmware.ewram.gba.comp:	firmware.ewram.gba ./upkr.elf
	./upkr.elf -l $(COMPRESSION_RATIO) $< $@

%.gba.comp:	%.gba.bin ./upkr.elf
	./upkr.elf -l $(COMPRESSION_RATIO) $< $@

%.db.comp:	%.db ./upkr.elf
	./upkr.elf -l $(COMPRESSION_RATIO) $< $@

%.pack.comp:	%.pack apultra.elf
	./apultra.elf $< $@

%.ld.i:	%.ld
	cpp $< -o $@

apultra.elf:	tools/apultra.cc
	g++ -std=c++20 -O3 $< -o $@

upkr.elf:	tools/upkr.cc
	g++ -o $@ $< -O3 -ffast-math

clean:
	rm -f ldscripts/*.i *.gba *.elf *.payload *.map res/*.comp emu/*.comp *.comp src/menu_messages.h src/messages_data.h res/slot_assets.pack
	rm -rf build/tests


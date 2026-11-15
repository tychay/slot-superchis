#!/usr/bin/env python3
# slot. superchis: build res/slot_assets.pack from the pinned slot. advance submodule (task 6.1).
#
# Reads the read-only vendor/slot-advance (a2a2fe9, v1.2.4, GPL-3.0-or-later; artwork by Markus,
# sounds from Slot) and writes ONE uncompressed blob: a header of offsets followed by every
# built-in asset the carousel draws (wood background, dark/light skeuomorphic and flat bands,
# plain cartridge moulds, 6x12 font, splash RLE, insert PCM).  The Makefile compresses it with
# tools/apultra.cc (the same aPLib-derived packer as the font pack) and rom_boot.S unpacks it into
# the pack region of the SDRAM window at boot (src/slot/slot_layout.h, ADR-0011).
# Nothing from the submodule is copied into the repo; the pack is a build output.
#
# Layout (all little-endian, every item 4-byte aligned):
#   u32 magic 'SLPK' (0x4B504C53), u32 version (1), u32 count, u32 total_size,
#   u32 insert_samples (valid PCM bytes), u32 samples_per_frame, u32 timer_period, u32 reserved,
#   count x { u32 offset, u32 size }   (offset from the start of the blob)
#   item data
# Item ids must match src/slot/slot_art.h (SLOT_PK_*).
#
# usage: slot_pack.py <vendor/slot-advance> <out.pack>
import importlib.util, re, struct, sys
from pathlib import Path

# The DirectSound stream is frame-synchronous (src/slot/slot_audio.h): the timer period T and the
# samples per frame N satisfy N*T == 280896 (cycles per GBA frame) exactly, so a restart at every
# VBlank never drifts.  N=352, T=798 -> 21,024.7 Hz (the clip is resampled to it, same duration).
FRAME_CYCLES = 280896
SND_N, SND_T = 352, 798
assert SND_N * SND_T == FRAME_CYCLES
SND_HZ = 16777216 / SND_T

root = Path(sys.argv[1])
src = (root / 'common/slot_assets.h').read_text()
arrays = {}
for m in re.finditer(r'static const unsigned (short|char) (\w+)\[[^\]]+\](?:\[\d+\])? = \{(.*?)\};', src, re.S):
    arrays[m.group(2)] = [int(x, 0) for x in re.findall(r'0x[0-9a-fA-F]+|\d+', m.group(3))]

def u16(name, size):
    v = arrays[name]
    assert len(v) == size, (name, len(v), size)
    return struct.pack('<%dH' % size, *v)

# the PCM: resample the original 48 kHz clip exactly like tools/convert_sounds.py, to SND_HZ
spec = importlib.util.spec_from_file_location('convert_sounds', root / 'tools/convert_sounds.py')
cs = importlib.util.module_from_spec(spec); spec.loader.exec_module(cs)
pcm = cs.resample((root / 'assets/slot/audio/insert-48000-s16.pcm').read_bytes(), 48000, round(SND_HZ))
valid = len(pcm)
pcm += b'\0' * ((-len(pcm)) % SND_N)          # whole frames

splash = arrays['slot_splash']; assert len(splash) == 38400
pairs, i = [], 0
while i < len(splash):
    j = i
    while j < len(splash) and splash[j] == splash[i] and j - i < 65535:
        j += 1
    pairs += [j - i, splash[i]]; i = j

font = bytes(arrays['slot_font']); assert len(font) == 128 * 12

# order == SLOT_PK_* in src/slot/slot_art.h
items = [
    u16('slot_background', 38400),
    u16('slot_back_dark', 4560), u16('slot_front_dark', 4560),
    u16('slot_back_light', 4560), u16('slot_front_light', 4560),
    u16('slot_back_flat_dark', 4560), u16('slot_front_flat_dark', 4560),
    u16('slot_back_flat_light', 4560), u16('slot_front_flat_light', 4560),
    u16('slot_fallback_gba', 8520), u16('slot_fallback_gb', 11648), u16('slot_fallback_gbc', 11648),
    u16('slot_flat_gba', 8520), u16('slot_flat_gb', 11648), u16('slot_flat_gbc', 11648),
    font,
    struct.pack('<%dH' % len(pairs), *pairs),
    pcm,
]
head = 32 + 8 * len(items)
blob, table, off = bytearray(), [], head
for it in items:
    it = it + b'\0' * ((-len(it)) % 4)
    table.append((off, len(it))); blob += it; off += len(it)
out = struct.pack('<8I', 0x4B504C53, 1, len(items), off, valid, SND_N, SND_T, 0)
for o, s in table:
    out += struct.pack('<2I', o, s)
out += blob
Path(sys.argv[2]).write_bytes(out)
print('slot_assets.pack: %d items, %d bytes, insert PCM %d samples at %.1f Hz' % (len(items), len(out), valid, SND_HZ))

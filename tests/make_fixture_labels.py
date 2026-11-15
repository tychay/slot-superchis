#!/usr/bin/env python3
"""Draw the fictional label PNGs behind tests/fixtures/art (original shapes and invented titles only).

usage: tests/make_fixture_labels.py <out dir>   -> <out>/Labels/{GBA,GB,GBC}/<stem>.png + Labels/cart_shell.txt
Then convert with the vendored converter exactly as tools/convert_labels.sh does (zip the Labels folder).
SPDX-License-Identifier: GPL-3.0-or-later
"""
import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

# stem, platform, title, background, accent, shell (outline colour finish)
ITEMS = [
    ("gba_aurora_quest",  "GBA", "AURORA QUEST",  "#1b2a5c", "#f0b24a", "auto 8855cc solid"),
    ("gba_harbor_lights", "GBA", "HARBOR LIGHTS", "#0f4a4f", "#f2e7b8", "auto 2f8f4e clear"),
    ("gb_pocket_pinball", "GB",  "POCKET PINBALL", "#3a3f2a", "#d8e4a0", "notched 9a978f solid"),
    ("gbc_neon_racer",    "GBC", "NEON RACER",    "#2b1245", "#ff5fa8", "rounded 7c7a8a clear"),
    ("gbc_pixel_farm",    "GBC", "PIXEL FARM",    "#5a3a1c", "#9be06a", "notched 333031 solid"),
]

def draw(platform, title, bg, accent):
    size = (1280, 661) if platform == "GBA" else (1024, 901)
    im = Image.new("RGB", size, bg); d = ImageDraw.Draw(im); w, h = size
    for k in range(9):  # concentric rings
        r = 40 + k * 38
        d.ellipse((w * .5 - r, h * .42 - r, w * .5 + r, h * .42 + r), outline=accent, width=6)
    d.polygon([(w * .1, h * .8), (w * .3, h * .35), (w * .5, h * .8)], fill=accent)
    d.rectangle((0, h - 150, w, h), fill="#efe9d6")
    f = ImageFont.load_default(size=96)
    while d.textlength(title, font=f) > w - 60:
        f = ImageFont.load_default(size=f.size - 4)
    d.text((w / 2, h - 75), title, font=f, anchor="mm", fill="#25223a")
    return im

out = Path(sys.argv[1])
for stem, plat, title, bg, acc, shell in ITEMS:
    (out / "Labels" / plat).mkdir(parents=True, exist_ok=True)
    draw(plat, title, bg, acc).save(out / "Labels" / plat / (stem + ".png"))
(out / "Labels" / "cart_shell.txt").write_text(
    "".join(f"{s} = {sh}\n" for s, _, _, _, _, sh in ITEMS))

#!/usr/bin/env python3
# slot. superchis: generate sample Cart Studio output for tests/slot_bmp_test.c (task 6.2).
# Makes three label PNGs, runs the pinned vendor/slot-advance/tools/convert_labels.py on them (the real
# converter, needs Pillow: present in the build image) and leaves SYSTEM/SLOT/{CARTS,CARTS_FLAT,LABELS}/...
# under <outdir>.  usage: make_art_fixtures.py <outdir>
import shutil, subprocess, sys
from pathlib import Path
from PIL import Image, ImageDraw

out = Path(sys.argv[1]).resolve()
work = out.parent / (out.name + '-input')
for d in (out, work):
    shutil.rmtree(d, ignore_errors=True)
for plat, size in (('GBA', (240, 160)), ('GB', (160, 144)), ('GBC', (160, 144))):
    d = work / 'Labels' / plat
    d.mkdir(parents=True)
    for stem in ('Sample Game (USA)', 'Odd & Name, The (Rev 1)'):
        im = Image.new('RGB', size, (200, 60, 40))
        ImageDraw.Draw(im).rectangle((10, 10, size[0] - 10, size[1] // 2), fill=(30, 90, 200))
        ImageDraw.Draw(im).text((20, size[1] - 30), stem, fill=(255, 255, 255))
        im.save(d / (stem + '.png'))
tool = Path(__file__).resolve().parents[1] / 'vendor/slot-advance/tools/convert_labels.py'
subprocess.run([sys.executable, str(tool), '--input', str(work), '--output', str(out)], check=True)

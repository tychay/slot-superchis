#!/bin/sh
# slot. superchis: convert a Cart Studio "labels.zip" into the /SYSTEM/SLOT art the carousel reads.
#
# usage: tools/convert_labels.sh <labels.zip> <card root>
#
# Runs the vendored slot. advance converter (vendor/slot-advance/tools/convert_labels.py) inside the
# build container (Pillow is in docker/Dockerfile), so a Mac or Linux box needs only Docker. The card
# root is the folder that holds Games/ (the mounted SD card, or its mirror): the ROM headers in
# Games/{GBA,GB,GBC} choose each cart's shell colour and outline, and the result is written to
# <card root>/SYSTEM/SLOT/{CARTS,CARTS_FLAT,LABELS}/<GBA|GB|GBC>/<ROM name>.bmp.
# Existing files there are replaced; SETTINGS.TXT and everything else on the card is left alone.
# On Windows use slot. advance's "Convert Labels.bat" / Label Converter.exe instead.
set -e
[ $# -eq 2 ] || { sed -n '2,12p' "$0" | sed 's/^# \{0,1\}//'; exit 2; }
[ -f "$1" ] || { echo "no such file: $1" >&2; exit 2; }
[ -d "$2" ] || { echo "no such folder: $2" >&2; exit 2; }
ZIP=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
CARD=$(cd "$2" && pwd)
cd "$(dirname "$0")/.."
IMG=slot-superchis-build
docker build -q -t "$IMG" docker >/dev/null
docker run --rm -u "$(id -u):$(id -g)" \
  -v "$PWD/vendor/slot-advance:/sa:ro" -v "$ZIP:/in/labels.zip:ro" -v "$CARD:/card" "$IMG" \
  python3 -W ignore /sa/tools/convert_labels.py --input /in/labels.zip --output /card --roms /card

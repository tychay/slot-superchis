#!/bin/sh
# slot: headless screenshot harness (tasks 2.1/2.2). Runs a GBA ROM in libmgba inside the build
# container (no window, no GUI/screen-capture permissions) with a scripted key sequence and
# writes PNGs to build/shots/.
#
# usage: tools/preview_shots.sh <rom.gba> <sequence> [outdir]     (default outdir: build/shots)
#
# sequence = comma-separated steps, executed in order, no spaces:
#   wait:N          emulate N frames (60 frames = 1 s)
#   KEY             tap KEY: held 4 frames, then 6 frames released
#   hold:KEY[:N]    hold KEY for N frames (default 10)
#   shot            write step<NN>_f<frame>.png (NN = shot counter)
#   shot:NAME       write NAME.png
# KEY (case-insensitive) = A B L R Start Select Up Down Left Right
#
# example: wait:120,shot:boot,Right,Right,wait:20,shot,R,wait:20,shot
#
# The runner is tools/mgba_shots.c, compiled on the fly against libmgba-dev (docker/Dockerfile).
set -e
[ $# -ge 2 ] || { sed -n '2,19p' "$0" | sed 's/^# \{0,1\}//'; exit 2; }
cd "$(dirname "$0")/.."
ROM=$1; SEQ=$2; OUT=${3:-build/shots}
[ -f "$ROM" ] || { echo "no such ROM: $ROM" >&2; exit 2; }
mkdir -p "$OUT" build/tools
IMG=slot-superchis-build
docker build -q -t "$IMG" docker >/dev/null
docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/src" "$IMG" sh -c \
  'gcc -O2 -Wall -o build/tools/mgba_shots tools/mgba_shots.c -lmgba && build/tools/mgba_shots "$@"' sh "$ROM" "$OUT" "$SEQ"

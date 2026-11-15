#!/bin/sh
# Canonical build: runs make inside the docker/Dockerfile image.
# Usage: ./build.sh [make args...]   (default: BOARD=chis COMPRESSION_RATIO=10)
set -e
cd "$(dirname "$0")"
IMG=slot-superchis-build
docker build -q -t "$IMG" docker >/dev/null
[ $# -eq 0 ] && set -- BOARD=chis COMPRESSION_RATIO=10
exec docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/src" "$IMG" make -j"$(sysctl -n hw.ncpu 2>/dev/null || nproc)" "$@"

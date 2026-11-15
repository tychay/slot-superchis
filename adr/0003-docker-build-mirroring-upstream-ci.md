# 0003. Canonical build runs in Docker, mirroring upstream CI

- Status: accepted
- Date: 2026-11-14

## Context

The upstream Makefile assumes Linux host tools. On macOS it fails at
`%.ld.i: cpp $< -o $@`, because Apple's clang `cpp` rejects `.ld` inputs. Host-built helpers
(`apultra.cc`, `upkr.cc`) and `#!/usr/bin/env python` scripts are also sensitive to the host.

Options considered:
- patch the Makefile;
- put a `cpp` shim on PATH;
- a native Homebrew `gcc-arm-embedded` toolchain;
- a Docker image matching upstream CI.

## Decision

- `docker/Dockerfile` reproduces upstream's `build-release.yml` environment: Ubuntu, apt
  `gcc-arm-none-eabi` with newlib, gcc/g++, make, python3, `python-is-python3`.
- `./build.sh [make args]` runs `make` inside it (default `BOARD=chis COMPRESSION_RATIO=10`).
- This is the canonical build. A native toolchain is an unsupported convenience.

## Consequences

- The upstream Makefile builds unmodified, which keeps merges clean (ADR-0001).
- Builds behave the same on any OS with Docker. Docker Desktop must be running.
- The host still needs mGBA for preview screenshots; that is outside the container.

# Build environment mirroring upstream SuperFW CI (.github/workflows/build-release.yml):
# Ubuntu + apt gcc-arm-none-eabi + host gcc/g++ + python3.
FROM ubuntu:26.04
RUN apt-get update \
 && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
      gcc-arm-none-eabi libnewlib-arm-none-eabi gcc g++ make python3 python-is-python3 git ca-certificates \
 && rm -rf /var/lib/apt/lists/*
WORKDIR /src

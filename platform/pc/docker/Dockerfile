# The Linux build environment (build-linux.bat): 32- and 64-bit GCC, binutils, Python and SDL2.
# Builds made here link against bookworm's glibc (2.36).
FROM debian:bookworm
RUN dpkg --add-architecture i386 && apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    gcc-multilib g++-multilib binutils python3 pkg-config libsdl2-dev:i386 libsdl2-dev \
    && rm -rf /var/lib/apt/lists/*

# Build & test environment for rcc700.
# Uses Espressif's riscv32-esp-elf toolchain (same one ESP-IDF installs), so
# the crt0/newlib that test binaries link against match the final target.
# Works the same on Apple Silicon (arm64 base image, qemu-user emulates only
# the RV32 binaries) and on x86-64 (e.g. GitHub Actions runners).
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        make \
        qemu-user \
        xz-utils \
    && rm -rf /var/lib/apt/lists/*

# riscv32-esp-elf toolchain, prebuilt by Espressif (crosstool-NG releases).
ARG ESP_TC_VER=esp-15.2.0_20251204
RUN arch=$(uname -m) && \
    curl -fsSL "https://github.com/espressif/crosstool-NG/releases/download/${ESP_TC_VER}/riscv32-esp-elf-${ESP_TC_VER#esp-}-${arch}-linux-gnu.tar.xz" \
    | tar -xJ -C /opt && \
    ln -s /opt/riscv32-esp-elf/bin/* /usr/local/bin/

WORKDIR /work

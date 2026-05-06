#!/usr/bin/env bash
set -euo pipefail

run_as_root() {
    if [ "$(id -u)" -eq 0 ]; then
        "$@"
    elif command -v sudo >/dev/null 2>&1; then
        sudo "$@"
    else
        echo "This installer needs root privileges. Install sudo or run as root." >&2
        exit 1
    fi
}

run_as_root pacman -Sy --needed --noconfirm \
    base-devel \
    cmake \
    firejail \
    gdb \
    git \
    glew \
    iproute2 \
    libglvnd \
    make \
    net-tools \
    nodejs \
    npm \
    pkgconf \
    rsync \
    sdl2 \
    vulkan-headers \
    vulkan-icd-loader \
    wget \
    xorg-server-xvfb \
    xorg-xauth \
    xpra

if pacman -Si execstack >/dev/null 2>&1; then
    run_as_root pacman -S --needed --noconfirm execstack
else
    echo "execstack is not in the configured pacman repositories; build.sh will skip it."
fi

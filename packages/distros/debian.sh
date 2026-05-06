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

export DEBIAN_FRONTEND="${DEBIAN_FRONTEND:-noninteractive}"

run_as_root apt-get update
run_as_root apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    firejail \
    gdb \
    git \
    iproute2 \
    libglew-dev \
    libgl-dev \
    libsdl2-dev \
    libvulkan-dev \
    make \
    net-tools \
    nodejs \
    npm \
    pkg-config \
    rsync \
    wget \
    xauth \
    xpra \
    xserver-xorg-core \
    xserver-xorg-video-dummy \
    xvfb

if apt-cache show execstack >/dev/null 2>&1; then
    run_as_root apt-get install -y --no-install-recommends execstack
else
    echo "execstack package is not available on this apt repository; build.sh will skip it."
fi

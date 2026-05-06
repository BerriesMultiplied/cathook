#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "$script_dir/.." && pwd)"

source_os_release() {
    if [ -r /etc/os-release ]; then
        # shellcheck disable=SC1091
        . /etc/os-release
    else
        ID=""
        ID_LIKE=""
        NAME="unknown Linux"
    fi
}

run_distro_script() {
    local distro_name="$1"
    local script_name="$2"

    echo "Detected $distro_name"
    exec bash "$repo_root/packages/distros/$script_name"
}

source_os_release

case "${ID:-}" in
    debian | ubuntu | linuxmint | pop | elementary | zorin)
        run_distro_script "Debian/Ubuntu family" "debian.sh"
        ;;
    arch | manjaro | endeavouros | garuda)
        run_distro_script "Arch Linux family" "arch.sh"
        ;;
    fedora | rhel | centos | rocky | almalinux | nobara)
        run_distro_script "Fedora/RHEL family" "fedora.sh"
        ;;
    gentoo)
        run_distro_script "Gentoo" "gentoo.sh"
        ;;
    void)
        run_distro_script "Void Linux" "void.sh"
        ;;
    alpine)
        run_distro_script "Alpine Linux" "alpine.sh"
        ;;
    opensuse* | sles)
        run_distro_script "openSUSE/SLES family" "opensuse.sh"
        ;;
esac

case " ${ID_LIKE:-} " in
    *" debian "*)
        run_distro_script "Debian-like distribution" "debian.sh"
        ;;
    *" arch "*)
        run_distro_script "Arch-like distribution" "arch.sh"
        ;;
    *" fedora "* | *" rhel "*)
        run_distro_script "Fedora/RHEL-like distribution" "fedora.sh"
        ;;
    *" gentoo "*)
        run_distro_script "Gentoo-like distribution" "gentoo.sh"
        ;;
    *" suse "*)
        run_distro_script "SUSE-like distribution" "opensuse.sh"
        ;;
esac

if command -v apt-get >/dev/null 2>&1; then
    run_distro_script "apt-based distribution" "debian.sh"
elif command -v pacman >/dev/null 2>&1; then
    run_distro_script "pacman-based distribution" "arch.sh"
elif command -v dnf >/dev/null 2>&1 || command -v yum >/dev/null 2>&1; then
    run_distro_script "dnf/yum-based distribution" "fedora.sh"
elif command -v emerge >/dev/null 2>&1; then
    run_distro_script "emerge-based distribution" "gentoo.sh"
elif command -v xbps-install >/dev/null 2>&1; then
    run_distro_script "xbps-based distribution" "void.sh"
elif command -v apk >/dev/null 2>&1; then
    run_distro_script "apk-based distribution" "alpine.sh"
elif command -v zypper >/dev/null 2>&1; then
    run_distro_script "zypper-based distribution" "opensuse.sh"
fi

echo "Unsupported Linux distribution: ${NAME:-unknown}"
exit 1

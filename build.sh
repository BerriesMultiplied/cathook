#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
asset_source_dir="$project_root/assets"
install_root="${CATHOOK_ROOT:-/opt/cathook}"
build_mode="${CAT_BUILD_MODE:-}"

usage() {
    cat <<'EOF'
Usage: ./build.sh [default|textmode|both|--default|--textmode|--both|--no-install]

Without a mode argument, builds the default NORMAL/NON-TEXTMODE
libcathook.so with SDL hooking enabled.

Environment:
  CAT_BUILD_MODE=default|textmode|both
  CATHOOK_ROOT=/opt/cathook
EOF
}

run_as_root() {
    if [ "$(id -u)" -eq 0 ]; then
        "$@"
    elif command -v sudo >/dev/null 2>&1; then
        sudo "$@"
    else
        echo "sudo is required to install to $install_root." >&2
        return 1
    fi
}

copy_assets() {
    local install_assets_dir="$1"

    if [ ! -d "$asset_source_dir" ]; then
        echo "No assets directory found at $asset_source_dir"
        return
    fi

    run_as_root install -d "$install_assets_dir"
    run_as_root cp -a "$asset_source_dir"/. "$install_assets_dir"/
    echo "Installed assets to $install_assets_dir"
}

normalize_mode() {
    case "$1" in
        default | non-textmode | non_textmode | normal | gui | 1)
            printf '%s\n' "default"
            ;;
        textmode | text | 2)
            printf '%s\n' "textmode"
            ;;
        both | all | 3)
            printf '%s\n' "both"
            ;;
        *)
            return 1
            ;;
    esac
}

choose_build_mode() {
    if [ -n "$build_mode" ]; then
        normalize_mode "$build_mode"
        return
    fi

    printf '%s\n' "default"
}

ensure_funchook() {
    mkdir -p "$project_root/libs"

    if [ -d "$project_root/libs/funchook" ] &&
       [ ! -f "$project_root/libs/funchook/libfunchook.a" ] &&
       [ ! -f "$project_root/libs/funchook/libdistorm.a" ]; then
        rm -rf "$project_root/libs/funchook"
    fi

    if [ ! -d "$project_root/libs/funchook" ]; then
        git clone https://github.com/Doctor-Coomer/funchook.git "$project_root/libs/funchook"
    fi

    if [ ! -f "$project_root/libs/funchook/libfunchook.a" ] ||
       [ ! -f "$project_root/libs/funchook/libdistorm.a" ]; then
        cmake -S "$project_root/libs/funchook" -B "$project_root/libs/funchook/build" -DCMAKE_BUILD_TYPE=Release
        cmake --build "$project_root/libs/funchook/build" --parallel "$(nproc 2>/dev/null || echo 1)"
    fi
}

clear_execstack_if_needed() {
    local output_binary="$1"

    if ! command -v execstack >/dev/null 2>&1 || [ ! -f "$output_binary" ]; then
        return
    fi

    if [ "$(execstack -q "$output_binary")" = "X $output_binary" ]; then
        execstack -c "$output_binary"
    fi
}

build_cat() {
    local mode="$1"

    case "$mode" in
        default)
            make -C "$project_root"
            ;;
        textmode)
            make -C "$project_root" TEXTMODE=1
            ;;
        both)
            make -C "$project_root" both
            ;;
    esac

    make -C "$project_root" catbot_ipc
}

install_outputs() {
    local mode="$1"
    local install_bin_dir="$install_root/bin"
    local install_assets_dir="$install_root/assets"
    local install_ipc_dir="$install_root/ipc"

    run_as_root install -d "$install_bin_dir" "$install_ipc_dir/bin"

    if [ "$mode" = "default" ] || [ "$mode" = "both" ]; then
        run_as_root install -m 0755 "$project_root/bin/libcathook.so" "$install_bin_dir/libcathook.so"
    fi

    if [ "$mode" = "textmode" ] || [ "$mode" = "both" ]; then
        run_as_root install -m 0755 "$project_root/bin/libcathooktextmode.so" "$install_bin_dir/libcathooktextmode.so"
        run_as_root install -m 0755 "$project_root/bin/libcathooktextmode.so" "$install_bin_dir/libcathook-textmode.so"
    fi

    make -C "$project_root/botpanel/catbot-ipc-server-main" REPO_ROOT="$project_root" INSTALL_DIR="$install_ipc_dir" install
    copy_assets "$install_assets_dir"
    echo "Installed Cat runtime to $install_root"
}

install_enabled=1

while [ "$#" -gt 0 ]; do
    case "$1" in
        --default)
            build_mode="default"
            ;;
        --textmode)
            build_mode="textmode"
            ;;
        --both)
            build_mode="both"
            ;;
        --no-install)
            install_enabled=0
            ;;
        -h | --help)
            usage
            exit 0
            ;;
        default | non-textmode | non_textmode | normal | gui | textmode | text | both | all)
            build_mode="$(normalize_mode "$1")"
            ;;
        *)
            echo "Unknown build option: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
    shift
done

selected_mode="$(choose_build_mode)" || {
    echo "Invalid build mode: ${build_mode:-}" >&2
    exit 1
}

ensure_funchook
build_cat "$selected_mode"

clear_execstack_if_needed "$project_root/bin/libcathook.so"
clear_execstack_if_needed "$project_root/bin/libcathooktextmode.so"

if [ "$install_enabled" = "1" ]; then
    install_outputs "$selected_mode"
fi

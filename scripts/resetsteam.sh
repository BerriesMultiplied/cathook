#!/usr/bin/env bash
set -euo pipefail

force=0

usage() {
  printf 'usage: %s [--force]\n' "$0"
  printf 'default mode is dry-run. pass --force to delete files.\n'
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --force)
      force=1
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      usage >&2
      exit 2
      ;;
  esac
  shift
done

home_dir="${HOME:?}"

run_rm() {
  path="$1"
  if [ ! -e "$path" ] && [ ! -L "$path" ]; then
    return 0
  fi
  if [ "$force" -eq 1 ]; then
    printf 'remove %s\n' "$path"
    rm -rf -- "$path"
  else
    printf 'would remove %s\n' "$path"
  fi
}

clean_steam_root() {
  steam_root="$1"
  if [ ! -d "$steam_root" ]; then
    return 0
  fi

  printf '\nsteam root: %s\n' "$steam_root"

  run_rm "$steam_root/userdata"
  run_rm "$steam_root/config"
  run_rm "$steam_root/appcache"
  run_rm "$steam_root/logs"
  run_rm "$steam_root/depotcache"
  run_rm "$steam_root/steam/cached"
  run_rm "$steam_root/.crash"
  run_rm "$steam_root/local.vdf"
  run_rm "$steam_root/update_hosts_cached.vdf"

  for path in "$steam_root"/ssfn*; do
    [ -e "$path" ] || [ -L "$path" ] || continue
    run_rm "$path"
  done

  find "$steam_root" -maxdepth 4 \
    \( -type d \( -name htmlcache -o -name avatarcache -o -name httpcache -o -name librarycache -o -name stats -o -name dumps \) \
    -o -type f \( -name 'loginusers.vdf' -o -name 'registry.vdf' -o -name 'DialogConfig.vdf' -o -name 'remoteclients.vdf' \) \) \
    -print 2>/dev/null | while IFS= read -r path; do
      run_rm "$path"
    done
}

clean_steamapps_state() {
  steamapps="$1"
  if [ ! -d "$steamapps" ]; then
    return 0
  fi

  printf '\nsteamapps state: %s\n' "$steamapps"

  run_rm "$steamapps/downloading"
  run_rm "$steamapps/temp"
  run_rm "$steamapps/shadercache"
  run_rm "$steamapps/workshop"
  run_rm "$steamapps/compatdata"
  run_rm "$steamapps/sourcemods"
  run_rm "$steamapps/music"
  run_rm "$steamapps/incomplete"
}

clean_user_paths() {
  run_rm "$home_dir/.steampid"
  run_rm "$home_dir/.steampath"
  run_rm "$home_dir/.steam/registry.vdf"
  run_rm "$home_dir/.steam/exportedsettings.json"
  run_rm "$home_dir/.steam/steam.pid"
  run_rm "$home_dir/.steam/steam.token"
  run_rm "$home_dir/.steam/steam.pipe"
  run_rm "$home_dir/.cache/Steam"
  run_rm "$home_dir/.config/steam"
  run_rm "$home_dir/.config/Valve Corporation"
}

stop_steam() {
  if command -v pgrep >/dev/null 2>&1; then
    if pgrep -u "$(id -u)" -x steam >/dev/null 2>&1; then
      if [ "$force" -eq 1 ]; then
        printf 'stopping steam\n'
        pkill -u "$(id -u)" -x steam || true
        sleep 2
      else
        printf 'would stop steam\n'
      fi
    fi
  fi
}

add_root() {
  root="$1"
  [ -n "$root" ] || return 0
  [ -d "$root" ] || return 0
  if command -v realpath >/dev/null 2>&1; then
    realpath "$root"
  elif command -v readlink >/dev/null 2>&1; then
    readlink -f "$root" 2>/dev/null || printf '%s\n' "$root"
  else
    printf '%s\n' "$root"
  fi
}

collect_roots() {
  {
    add_root "$home_dir/.steam/debian-installation"
    add_root "$home_dir/.steam/steam"
    add_root "$home_dir/.steam/root"
    add_root "$home_dir/.local/share/Steam"
    add_root "$home_dir/.var/app/com.valvesoftware.Steam/.local/share/Steam"
    add_root "$home_dir/snap/steam/common/.local/share/Steam"
  } | awk '!seen[$0]++'
}

if [ "$force" -eq 0 ]; then
  printf 'dry-run mode. re-run with --force to delete.\n'
fi

stop_steam
clean_user_paths

roots="$(collect_roots)"

if [ -z "$roots" ]; then
  printf 'no steam roots found\n'
  exit 0
fi

printf '%s\n' "$roots" | while IFS= read -r steam_root; do
  clean_steam_root "$steam_root"
  if [ -d "$steam_root/steamapps" ]; then
    clean_steamapps_state "$steam_root/steamapps"
  fi
done

printf '\ndone\n'

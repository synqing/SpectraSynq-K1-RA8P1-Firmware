#!/bin/sh
# E2 Lite -> Titan RA8P1 CPU0. Preserve flash. Probe power OFF.
# Must run with cwd = E2_Debug_Runtime (MCU files + ra8p1.6.6.0.xml).
set -eu
RT="${E2_DEBUG_RUNTIME:-/Users/spectrasynq/Applications/Renesas/E2_Debug_Runtime_10.6.0}"
cd "$RT"
PORT="${1:-61234}"
SPEED="${2:-10}"
HOTPLUG="${3:-0}"
RESET_BEGIN="${4:-1}"
exec ./e2-server-gdb \
  -g E2LITE \
  -t R7KA8P1KF \
  -p "$PORT" \
  -w 0 \
  -uInteface= "SWD" \
  -uIfSpeed= "$SPEED" \
  -uSyncMode= "async" \
  -uCore= "CPU0|enabled|1|${PORT}" \
  -uCore= "CPU1|disabled|0|$((PORT+1))" \
  -uHotPlug= "$HOTPLUG" \
  -uAuthLevel= "0" \
  -uEnableSciBoot= "0" \
  -uResetCon= "0" \
  -uResetBeginConnection= "$RESET_BEGIN" \
  -uNoReset= "1" \
  -uDisconnectionMode= "2" \
  -uConnectionTimeout= "20" \
  -ueraseRomOnDownload= "0" \
  -n 0

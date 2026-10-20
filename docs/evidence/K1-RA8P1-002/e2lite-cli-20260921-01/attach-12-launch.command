#!/bin/sh
# Aqua-session launch so ShowGUI can present the hot-plug dialog.
set -eu
RT="/Users/spectrasynq/Applications/Renesas/E2_Debug_Runtime_10.6.0"
EV="/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/docs/evidence/K1-RA8P1-002/e2lite-cli-20260921-01"
LOG="$EV/attach-12-hotplug-delayed.log"
cd "$RT"
# shell pid recorded; gdb pid written after spawn
echo "$$" > "$EV/attach-12-shell.pid"
./e2-server-gdb \
  -g E2LITE \
  -t R7KA8P1KF \
  -p 61234 \
  -w 0 \
  -uInteface= "SWD" \
  -uIfSpeed= "10" \
  -uSyncMode= "async" \
  -uCore= "CPU0|enabled|1|61234" \
  -uCore= "CPU1|disabled|0|61235" \
  -uHotPlug= "1" \
  -uShowGUI= "1" \
  -uAuthLevel= "0" \
  -uEnableSciBoot= "0" \
  -uResetCon= "0" \
  -uResetBeginConnection= "0" \
  -uNoReset= "1" \
  -uDisconnectionMode= "0" \
  -uConnectionTimeout= "120" \
  -ueraseRomOnDownload= "0" \
  -n 0 2>&1 | tee "$LOG"

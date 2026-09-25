#!/bin/zsh
# First-ever attach over the soldered six-wire harness (pins 1,2,3,4,9,10; pin5 GND removed by user)
# Standard connection, no hot-plug prompt (harness is permanently fitted), 10 kHz, probe power OFF, no download, no erase.
EV="$(cd "$(dirname "$0")" && pwd)"
RT=/Users/spectrasynq/Applications/Renesas/E2_Debug_Runtime_10.6.0
cd "$RT"
date -u +"%Y-%m-%dT%H:%M:%SZ" > "$EV/attach-utc.txt"
timeout 75 ./e2-server-gdb -g E2LITE -t R7KA8P1KF -p 61234 -w 0 \
  -uInteface= SWD -uIfSpeed= 10 -uSyncMode= async \
  -uCore= "CPU0|enabled|1|61234" -uCore= "CPU1|disabled|0|61235" \
  -uHotPlug= 0 -uShowGUI= 0 -uAuthLevel= 0 -uEnableSciBoot= 0 \
  -uResetCon= 1 -uResetBeginConnection= 1 -uNoReset= 1 \
  -uDisconnectionMode= 2 -uConnectionTimeout= 30 \
  -ueraseRomOnDownload= 0 -n 0 > "$EV/attach-harness-01.log" 2>&1
echo "exit=$?" >> "$EV/attach-harness-01.log"

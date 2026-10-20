#!/bin/sh
# Preserve-flash hot-plug attach: E2 Lite SWD -> R7KA8P1KF CPU0.
# Probe power OFF. No download/erase. Does not flash.
set -eu
cd /tmp/renesas-host-check-e4rag280
exec ./e2-server-gdb \
  -g E2LITE \
  -t R7KA8P1KF \
  -uConnectionTimeout= 30 \
  -uInteface= "SWD" \
  -uIfSpeed= "auto" \
  -w 0 \
  -uHotPlug= 1 \
  -uConnectMode= HOTPLUGIN \
  -uNoReset= 1 \
  -uResetBeginConnection= 0 \
  -uResetCon= 0 \
  -uresetOnReload= 0 \
  -ueraseRomOnDownload= 0 \
  -ueraseDataRomOnDownload= 0 \
  -ueraseSipFlashOnDownload= 0 \
  -n 0 \
  -uAuthLevel= 0 \
  -uneedAuthentication= 0 \
  -uLowPower= 1

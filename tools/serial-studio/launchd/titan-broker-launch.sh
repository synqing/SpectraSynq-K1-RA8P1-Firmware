#!/bin/bash
# launchd entry for the sole Titan CDC owner (com.spectrasynq.titan-broker).
# Restores the retained image-bound admission into /tmp (emptied at reboot)
# byte-for-byte, refuses on any hash mismatch, then execs the broker.
# It never generates or edits admission content. Titan's autonomous show
# does not depend on this process.
#
# No Titan on USB is not a crash. This process stays up and polls until
# exactly one 045b:5310 is present, so launchd does not exit-and-retry
# every 10 seconds while the board is unplugged. Two or more is a fault
# and still exits. A vanished board still ends the broker; the next start
# waits here until it returns.
set -euo pipefail

: "${TITAN_ADMISSION_SOURCE:?retained admission path required}"
: "${TITAN_ADMISSION_SHA256:?expected admission SHA-256 required}"
EVIDENCE="${TITAN_BROKER_EVIDENCE:-/tmp/titan-emotiscope-control}"
PYTHON="${TITAN_BROKER_PYTHON:-/opt/homebrew/opt/python@3.14/bin/python3.14}"
HERE="$(cd "$(dirname "$0")/.." && pwd)"

sha() { /usr/bin/shasum -a 256 "$1" | /usr/bin/awk '{print $1}'; }

# Same witness as the mixer health check: count USB product 21264 (0x5310).
# This reads the registry only. It does not open the serial port.
titan_cdc_count() {
  /usr/sbin/ioreg -p IOUSB -w0 -l | /usr/bin/awk '
    BEGIN { n = 0 }
    $0 ~ /"idProduct" = 21264([^0-9]|$)/ { n++ }
    END { print n + 0 }
  '
}

wait_for_one_titan() {
  local count now last_log
  count="$(titan_cdc_count)"
  if [ "$count" = "1" ]; then
    return 0
  fi
  if [ "$count" != "0" ]; then
    echo "REFUSED: expected one Titan 045b:5310, found $count" >&2
    exit 1
  fi
  echo "$(date -u +%FT%TZ) Titan 045b:5310 absent; waiting in this process"
  last_log="$(date +%s)"
  while [ "$count" = "0" ]; do
    sleep 5
    now="$(date +%s)"
    if [ $((now - last_log)) -ge 60 ]; then
      echo "$(date -u +%FT%TZ) Titan 045b:5310 still absent; still waiting"
      last_log="$now"
    fi
    count="$(titan_cdc_count)"
  done
  if [ "$count" != "1" ]; then
    echo "REFUSED: expected one Titan 045b:5310, found $count" >&2
    exit 1
  fi
  echo "$(date -u +%FT%TZ) Titan 045b:5310 present; starting broker"
}

echo "$(date -u +%FT%TZ) titan-broker-launch pid=$$ source=$TITAN_ADMISSION_SOURCE"
[ "$(sha "$TITAN_ADMISSION_SOURCE")" = "$TITAN_ADMISSION_SHA256" ] || {
  echo "REFUSED: retained admission hash differs from $TITAN_ADMISSION_SHA256" >&2; exit 78; }

mkdir -p "$EVIDENCE"
target="$EVIDENCE/accepted-candidate.json"
if [ ! -f "$target" ] || [ "$(sha "$target")" != "$TITAN_ADMISSION_SHA256" ]; then
  tmp="$(mktemp "$EVIDENCE/.accepted-candidate.XXXXXX")"
  /bin/cp "$TITAN_ADMISSION_SOURCE" "$tmp"
  [ "$(sha "$tmp")" = "$TITAN_ADMISSION_SHA256" ] || { rm -f "$tmp"; echo "REFUSED: copy hash mismatch" >&2; exit 78; }
  /bin/mv -f "$tmp" "$target"
fi
# A stale checkpoint file would take precedence over the candidate; never carry one.
[ ! -e "$EVIDENCE/accepted-checkpoint.json" ] || { echo "REFUSED: unexpected accepted-checkpoint.json present" >&2; exit 78; }
echo "admission $(sha "$target") -> $target"

wait_for_one_titan
# The wait can outlast the file that was checked above. Refuse again before exec.
[ "$(sha "$TITAN_ADMISSION_SOURCE")" = "$TITAN_ADMISSION_SHA256" ] || {
  echo "REFUSED: retained admission hash differs from $TITAN_ADMISSION_SHA256" >&2; exit 78; }
[ "$(sha "$target")" = "$TITAN_ADMISSION_SHA256" ] || {
  echo "REFUSED: restored admission hash differs from $TITAN_ADMISSION_SHA256" >&2; exit 78; }
[ ! -e "$EVIDENCE/accepted-checkpoint.json" ] || { echo "REFUSED: unexpected accepted-checkpoint.json present" >&2; exit 78; }

export TITAN_LIVE_CONTROL_ADMITTED=1
exec "$PYTHON" "$HERE/titan_broker.py" run --evidence "$EVIDENCE" --live-control --continuous-output

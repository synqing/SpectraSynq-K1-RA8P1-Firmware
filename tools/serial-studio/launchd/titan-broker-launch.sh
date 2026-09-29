#!/bin/bash
# launchd entry for the sole Titan CDC owner (com.spectrasynq.titan-broker).
# Restores the retained image-bound admission into /tmp (emptied at reboot)
# byte-for-byte, refuses on any hash mismatch, then execs the broker.
# It never generates or edits admission content. Titan's autonomous show
# does not depend on this process.
set -euo pipefail

: "${TITAN_ADMISSION_SOURCE:?retained admission path required}"
: "${TITAN_ADMISSION_SHA256:?expected admission SHA-256 required}"
EVIDENCE="${TITAN_BROKER_EVIDENCE:-/tmp/titan-emotiscope-control}"
PYTHON="${TITAN_BROKER_PYTHON:-/opt/homebrew/opt/python@3.14/bin/python3.14}"
HERE="$(cd "$(dirname "$0")/.." && pwd)"

sha() { /usr/bin/shasum -a 256 "$1" | /usr/bin/awk '{print $1}'; }

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

export TITAN_LIVE_CONTROL_ADMITTED=1
exec "$PYTHON" "$HERE/titan_broker.py" run --evidence "$EVIDENCE" --live-control --continuous-output

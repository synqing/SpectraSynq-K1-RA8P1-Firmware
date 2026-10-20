#!/usr/bin/env bash
# One honest check for the Titan Emotiscope instrument (firmware lane + SMC host).
# Exit status is the real result: the first failing suite stops the run non-zero.
# Never touches Titan, the broker socket or MIDI.
set -euo pipefail

F="$(cd "$(dirname "$0")/.." && pwd)"
S="${SMC_WORKTREE:-/Users/spectrasynq/Workspace_Management/Software/SMC-Mixer/.worktrees/titan-control-001}"
HOST_PY="$S/host/titan_control/.venv/bin/python"
FW_PY="${FW_PYTHON:-/opt/homebrew/opt/python@3.14/bin/python3.14}"
OUT="$(mktemp -d /tmp/check-instrument.XXXXXX)"   # short path: AF_UNIX sockets in tests
export TMPDIR="$OUT"

state() { # repo label
  local head dirty
  head="$(git -C "$1" rev-parse --short HEAD)"
  dirty="$(git -C "$1" status --porcelain --untracked-files=no | wc -l | tr -d ' ')"
  echo "$2 $1 HEAD=$head tracked-dirty=$dirty ($(git -C "$1" branch --show-current))"
}

echo "== interpreters"
echo "host:     $HOST_PY ($("$HOST_PY" -V 2>&1))"
echo "firmware: $FW_PY ($("$FW_PY" -V 2>&1))"
echo "== worktrees"
state "$F" F
state "$S" S
echo "== output $OUT"

step() { echo; echo "== $1"; }

step "S host suite (run_tests.py, pytest)"
"$HOST_PY" "$S/host/titan_control/run_tests.py"

step "F serial-studio broker suite"
"$FW_PY" -m pytest -q -p no:cacheprovider --basetemp="$OUT/pytest" "$F/tools/serial-studio/tests"

step "F live protocol"
"$FW_PY" "$F/scripts/test_live_protocol.py"

step "F Emotiscope controls codec"
"$FW_PY" "$F/scripts/test_emotiscope_controls.py" --output "$OUT/controls"

step "F Emotiscope output path"
"$FW_PY" "$F/scripts/test_emotiscope_output.py" --output "$OUT/output"

echo
step "F composed scenes and negotiated dual controls"
"$FW_PY" "$F/scripts/test_emotiscope_controls.py" --top-output --output "$OUT/dual-controls"
for name in motion timing scene donor; do
  "$FW_PY" "$F/harness/tests/test_composed_${name}.py"
done

echo "CHECK_INSTRUMENT PASS"

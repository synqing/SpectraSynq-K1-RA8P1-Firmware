#!/usr/bin/env python3
"""Host-only contract checks for the Titan LED interactive console."""
from titan_led_console import ConsoleState, advance, apply_hotkey, apply_typed, frame_for_state

state = ConsoleState()
frame = frame_for_state(state)
assert len(frame) == 384 and frame[63 * 3 + 1] == 96 and frame[64 * 3 + 1] == 96
assert apply_hotkey("g", state) == "solid=green"
frame = frame_for_state(state)
assert frame[:3] == bytes((96, 0, 0)) and frame[-3:] == bytes((96, 0, 0))
assert apply_hotkey("]", state) == "brightness=104"
assert apply_hotkey(" ", state) == "off" and not any(frame_for_state(state))
assert apply_typed("centre", state) == "centre"
for _ in range(63):
    advance(state)
assert state.radius == 63 and state.direction == -1
for _ in range(63):
    advance(state)
assert state.radius == 0 and state.direction == 1
assert apply_typed("brightness 128", state) == "brightness=128"
assert apply_typed("fps 60", state) == "fps=60"
for command in ("brightness 0", "brightness 129", "fps 0", "fps 61", "erase", "reset"):
    try:
        apply_typed(command, state)
    except ValueError:
        pass
    else:
        raise AssertionError(f"unsafe or invalid command accepted: {command}")
assert apply_hotkey("x", state) == "ignored"
print("TITAN_LED_CONSOLE=PASS hotkeys=PASS typed_commands=PASS unsafe_rejected=PASS")

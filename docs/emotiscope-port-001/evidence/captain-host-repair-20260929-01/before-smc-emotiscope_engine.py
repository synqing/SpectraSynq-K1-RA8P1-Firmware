"""Emotiscope SMC strategy. Wire values are native. UI transforms stay here.

Saturation: fader u becomes sqrt(u) once. Pickup compares u with raw squared.
Color pickup uses the sampled effective hue and does not follow auto-color.
Fader1 (ch0 CC40) and Fader4 (ch0 CC43) do not write until each has been seen
on channel 0. Channel 1 CC40 is R5 and channel 1 CC43 is R8. They never alias.
"""
from __future__ import annotations

from collections import deque
import copy
import json
import math
from pathlib import Path

from .emotiscope_store import (
    ASSIGNABLE_ABSOLUTE,
    ASSIGNABLE_TOGGLE,
    NATIVE_FIELDS,
    SCENE_SLOTS,
    EmotiscopeStore,
)

PROFILE_PATH = Path(__file__).resolve().parents[1] / "profiles" / "emotiscope-v1.json"
PICKUP = 1.0 / 127.0
CONTINUOUS_ORDER = (
    "brightness", "softness", "speed", "background", "color", "color_range",
    "saturation", "warmth", "hardware_output_brightness",
)
FLOATS = (
    "brightness", "softness", "color", "warmth", "color_range", "speed",
    "saturation", "background",
)
TOGGLES = (
    "mirror_mode", "screensaver", "temporal_dithering", "auto_color_cycle",
    "reverse_color_range",
)
SCENE_FIELDS = FLOATS + ("current_mode",) + TOGGLES + ("hardware_output_brightness",)
DEFAULTS = {
    "brightness": 1.0,
    "softness": 0.25,
    "color": 0.33,
    "warmth": 0.0,
    "color_range": 0.0,
    "speed": 0.50,
    "saturation": 0.75,
    "background": 0.25,
    "current_mode": 1,
    "mirror_mode": True,
    "screensaver": True,
    "temporal_dithering": True,
    "auto_color_cycle": False,
    "reverse_color_range": False,
    "hardware_output_brightness": 100,
    "emission_enabled": True,
}
MODES = ("Analog", "Spectrum", "Octave", "Metronome", "Spectronome", "Hype", "Bloom")


def load_profile(path: Path | None = None) -> dict:
    profile = json.loads((path or PROFILE_PATH).read_text())
    validate_profile(profile)
    return profile


def validate_profile(profile: dict) -> None:
    if profile.get("name") != "emotiscope-v1" or profile.get("version") != 1:
        raise ValueError("not the emotiscope profile")
    by = {}
    for name, rule in profile["bindings"].items():
        key = (int(rule["channel"]), int(rule["cc"]))
        if key in by:
            raise ValueError(f"ambiguous MIDI identity {key}")
        by[key] = name
        if rule.get("field") in {"motion", "visual_a", "mode_a"}:
            raise ValueError("legacy field in emotiscope profile")
    witnesses = {}
    for name, rule in profile.get("witnesses", {}).items():
        key = (int(rule["channel"]), int(rule["cc"]))
        if key in by or key in witnesses:
            raise ValueError(f"witness collides at {key}")
        witnesses[key] = name
    if by.get((0, 40)) != "Fader1" or by.get((0, 43)) != "Fader4":
        raise ValueError("Fader1/Fader4 identity changed")
    if witnesses.get((1, 40)) != "R5" or witnesses.get((1, 43)) != "R8":
        raise ValueError("R5/R8 must stay channel-1 witnesses")
    if (1, 40) in by or (1, 43) in by:
        raise ValueError("channel 1 CC40/CC43 aliased onto a fader")
    for index, name in enumerate(("M1", "M2", "M3", "M4", "M5", "M6", "M7")):
        rule = profile["bindings"][name]
        if rule.get("action") != "mode" or int(rule.get("mode")) != index:
            raise ValueError("musical mode map changed")


def _hue(value: float) -> float:
    number = float(value) % 1.0
    if number < 0.0:
        number += 1.0
    return number


def _same(field: str, left, right) -> bool:
    if field in TOGGLES or field == "emission_enabled":
        return bool(left) == bool(right)
    if field in {"current_mode", "hardware_output_brightness"}:
        return int(left) == int(right)
    return abs(float(left) - float(right)) <= 1e-6


class EmotiscopeEngine:
    def __init__(self, client, profile: dict | None = None, store: EmotiscopeStore | None = None):
        self.client = client
        self.profile = copy.deepcopy(profile or load_profile())
        validate_profile(self.profile)
        self.store = store
        self.bindings = {name: dict(rule) for name, rule in self.profile["bindings"].items()}
        self.by_cc = {(int(rule["channel"]), int(rule["cc"])): name for name, rule in self.bindings.items()}
        self.witnesses = {
            (int(rule["channel"]), int(rule["cc"])): name
            for name, rule in self.profile.get("witnesses", {}).items()
        }
        self.button_names = {name for name, rule in self.bindings.items() if rule["kind"] == "button"}
        self.state = dict(DEFAULTS)
        self.effective_hue = 0.33
        self.color_reference = 0.33
        self.boot_id = 0
        self.revision = 0
        self.schema = None
        self.bound = {}
        self.ready = False
        self.last_error = None
        self.fault = None
        self.request_seq = 0
        self.discrete = deque()
        self.local = deque()
        self.dirty = {}
        self.dirty_at = {}
        self.cursor = 0
        self.picked = {}
        self.last_ui = {}
        self.unknown = set()
        self.release_required = set(self.button_names)
        self.need_reconcile = False
        self.next_presence = 0.0
        self.seen_f1 = False
        self.seen_f4 = False
        self.faders_armed = False
        self.recent = deque(maxlen=32)
        self.qualification_log = []
        self.empty_recalls = 0
        self.speed_applicable = False
        self.reverse_applicable = False
        self.applicability = None
        self.accepted_request_id = None
        self.rendered_request_id = None
        self.emitted_request_id = None
        self.sent_batches = []
        if store is not None and store.error is None:
            self._overlay_assignments(store.assignments)

    def attach(self, now: float = 0.0) -> dict:
        hello = self.client.hello()
        self.bound = hello.get("bound") or {}
        self._reset_gestures()
        hinted = self.bound.get("boot_id")
        reply = self._get(0 if hinted is None else int(hinted))
        if reply.get("error") == "wrong_boot":
            boot = (reply.get("state") or {}).get("boot_id")
            if boot is None:
                self.ready = False
                self.last_error = "wrong_boot"
                return reply
            reply = self._get(int(boot))
        return self._accept_snapshot(reply, sample_hue=True, now=now)

    def reconnect(self, now: float = 0.0) -> dict:
        self._reset_gestures()
        reply = self._get(self.boot_id or 0)
        if reply.get("error") == "wrong_boot":
            boot = (reply.get("state") or {}).get("boot_id")
            if boot is None:
                self.ready = False
                self.last_error = "wrong_boot"
                return reply
            reply = self._get(int(boot))
        return self._accept_snapshot(reply, sample_hue=True, now=now)

    def abandon(self) -> None:
        self.discrete.clear()
        self.local.clear()
        self.dirty.clear()
        self.dirty_at.clear()
        self.need_reconcile = False

    def note_overflow(self) -> None:
        self.abandon()
        self.fault = "MIDI queue overflow"
        self.ready = False

    def observe(self, channel: int, cc: int, value: int) -> None:
        key = (int(channel), int(cc))
        name = self.by_cc.get(key)
        witness = self.witnesses.get(key)
        self.recent.append({"channel": key[0], "cc": key[1], "value": int(value), "name": name or witness})
        if name is None:
            return
        rule = self.bindings[name]
        if rule["kind"] == "absolute":
            if not 0 <= int(value) <= 127:
                return
            self._absolute(name, int(value))
            return
        if int(value) == 0:
            self.release_required.discard(name)
            return
        if int(value) != 127 or name in self.release_required:
            return
        self._press(name)

    def tick(self, now: float = 0.0) -> dict:
        self._service_local()
        if not self.ready or self.fault:
            return {"ipc": None, "fault": self.fault}
        kind, fields, own_color = self._next_discrete()
        if kind == "reconcile":
            return {"ipc": "get", "reply": self._get(self.boot_id)}
        if kind == "patch":
            return {"ipc": "patch", "reply": self._patch(fields, own_color=own_color, now=now)}
        if now >= self.next_presence:
            self.next_presence = now + 1.0
            return {"ipc": "get", "reply": self._get(self.boot_id)}
        batch = self._take(8)
        if batch:
            return {"ipc": "patch", "reply": self._patch(batch, own_color=batch.get("color"), now=now)}
        return {"ipc": None}

    def request_save(self, slot: str) -> None:
        if slot not in SCENE_SLOTS:
            raise ValueError("unknown scene slot")
        self.local.append(("save", slot))

    def request_recall(self, slot: str) -> None:
        if slot not in SCENE_SLOTS:
            raise ValueError("unknown scene slot")
        self.local.append(("recall", slot))

    def request_defaults(self) -> None:
        self.local.append(("defaults", None))

    def request_remap(self, control: str, field: str | None) -> None:
        self.local.append(("remap", control, field))

    def status(self) -> dict:
        rows = []
        for name, rule in self.bindings.items():
            field = rule.get("field")
            rows.append({
                "control": name,
                "label": rule.get("label") or name,
                "channel": rule["channel"],
                "cc": rule["cc"],
                "field": field,
                "picked_up": bool(self.picked.get(name)),
                "release_needed": name in self.release_required,
                "writes_armed": self._armed(name),
                "evidence": rule.get("evidence"),
                "qualified": False,
            })
        return {
            "ready": self.ready,
            "fault": self.fault,
            "last_error": self.last_error,
            "boot_id": self.boot_id,
            "revision": self.revision,
            "schema_sha256": self.schema,
            "mode": self.state["current_mode"],
            "mode_name": MODES[self.state["current_mode"]] if 0 <= self.state["current_mode"] <= 6 else "outside",
            "donor_brightness": self.state["brightness"],
            "output_brightness": self.state["hardware_output_brightness"],
            "emission_enabled": self.state["emission_enabled"],
            "effective_hue": self.effective_hue,
            "color_reference": self.color_reference,
            "auto_color_cycle": self.state["auto_color_cycle"],
            "speed_applicable": self.speed_applicable,
            "reverse_applicable": self.reverse_applicable,
            "applicability_notes": self._notes(),
            "accepted_request_id": self.accepted_request_id,
            "rendered_request_id": self.rendered_request_id,
            "emitted_request_id": self.emitted_request_id,
            "unknown": sorted(self.unknown),
            "faders_armed": self.faders_armed,
            "pending_fields": sorted(self.dirty),
            "pending_actions": len(self.discrete),
            "recent": list(self.recent),
            "controls": rows,
            "scenes": sorted(self.store.scenes) if self.store else [],
            "store_error": self.store.error if self.store else None,
            "optical_proof": False,
        }

    def _notes(self) -> list[str]:
        notes = []
        if self.state["current_mode"] == 6 or not self.speed_applicable:
            notes.append("Speed has no effect in this donor Bloom variant" if self.state["current_mode"] == 6
                         else "Speed changes Analog only")
        if not self.reverse_applicable:
            notes.append("Reverse color range has no effect while color range is zero")
        notes.append("Turning the screensaver off does not clear retained mix history")
        return notes

    def _armed(self, name: str) -> bool:
        if self.bindings[name].get("write_gate") == "fader-pair":
            return self.faders_armed
        return True

    def _overlay_assignments(self, saved: dict) -> None:
        for control, field in saved.items():
            if control not in self.bindings:
                raise ValueError("saved control is not in the emotiscope profile")
            rule = self.bindings[control]
            if field is None:
                rule["field"] = None
                continue
            if rule["kind"] == "absolute" and field not in ASSIGNABLE_ABSOLUTE:
                raise ValueError("absolute control cannot take " + field)
            if rule["kind"] == "button" and rule.get("action") == "toggle" and field not in ASSIGNABLE_TOGGLE:
                raise ValueError("toggle cannot take " + field)
            if rule["kind"] == "button" and rule.get("action") != "toggle":
                raise ValueError("this button is not remapped by an assignment field")
            rule["field"] = field

    def _reset_gestures(self) -> None:
        self.discrete.clear()
        self.dirty.clear()
        self.dirty_at.clear()
        self.picked.clear()
        self.last_ui.clear()
        self.unknown.clear()
        self.release_required = set(self.button_names)
        self.need_reconcile = False
        self.fault = None

    def _accept_snapshot(self, reply: dict, sample_hue: bool, now: float) -> dict:
        if not reply.get("ok") or not reply.get("state"):
            self.ready = False
            self.last_error = reply.get("error") or "no_donor_state"
            return reply
        bound_schema = self.bound.get("schema_sha256") or self.bound.get("schema")
        got = reply["state"].get("schema_sha256")
        if bound_schema and got and bound_schema != got:
            self.ready = False
            self.last_error = "schema_mismatch"
            return reply
        self._apply(reply["state"], sample_hue=sample_hue)
        self.ready = True
        self.last_error = None
        self.next_presence = now + 1.0
        return reply

    def _apply(self, state: dict, own_color=None, sample_hue: bool = False) -> None:
        for key in FLOATS + TOGGLES + ("hardware_output_brightness", "emission_enabled"):
            if key in state:
                self.state[key] = state[key]
        if "mode" in state:
            self.state["current_mode"] = int(state["mode"])
        if "effective_hue" in state:
            self.effective_hue = _hue(state["effective_hue"])
        elif "color" in state:
            self.effective_hue = _hue(state["color"])
        self.boot_id = int(state.get("boot_id", self.boot_id))
        self.revision = int(state.get("revision", self.revision))
        self.schema = state.get("schema_sha256", self.schema)
        self.speed_applicable = bool(state.get("speed_applicable", self.state["current_mode"] == 0))
        self.reverse_applicable = bool(state.get("reverse_applicable", self.state["color_range"] > 0))
        self.applicability = state.get("applicability")
        self.accepted_request_id = state.get("accepted_request_id")
        self.rendered_request_id = state.get("rendered_request_id")
        self.emitted_request_id = state.get("emitted_request_id")
        if own_color is not None:
            self.color_reference = _hue(own_color)
        elif sample_hue or not self.state["auto_color_cycle"]:
            self.color_reference = self.effective_hue
        self.unknown.clear()
        self._drop_matching_dirty()

    def _drop_matching_dirty(self) -> None:
        for field, value in list(self.dirty.items()):
            current = self.color_reference if field == "color" and self.state["auto_color_cycle"] else self.state.get(field)
            if field == "color" and self.state["auto_color_cycle"]:
                continue
            if _same(field, value, self.state.get(field)):
                self.dirty.pop(field, None)
                self.dirty_at.pop(field, None)

    def _get(self, boot_id: int) -> dict:
        self.request_seq += 1
        reply = self.client.emotiscope_get(int(boot_id), self.request_seq)
        if reply.get("state") and reply.get("ok"):
            self._apply(reply["state"])
        elif reply.get("error") == "wrong_boot" and reply.get("state"):
            self.boot_id = int(reply["state"].get("boot_id", self.boot_id))
        return reply

    def _patch(self, fields: dict, own_color=None, now: float = 0.0) -> dict:
        if any(key not in NATIVE_FIELDS for key in fields) or "current_mode" in fields and not 0 <= int(fields["current_mode"]) <= 6:
            self.last_error = "refusing non-donor patch"
            return {"ok": False, "error": "refusing non-donor patch"}
        self.request_seq += 1
        reply = self.client.emotiscope_patch(self.boot_id, self.revision, dict(fields), self.request_seq)
        self.sent_batches.append(dict(fields))
        if reply.get("ok") and reply.get("state"):
            self._apply(reply["state"], own_color=own_color if "color" in fields else None)
            return reply
        if reply.get("state"):
            self._apply(reply["state"])
        for key in fields:
            self.unknown.add(key)
            if key in CONTINUOUS_ORDER and key not in self.dirty:
                self.dirty[key] = fields[key]
                self.dirty_at.setdefault(key, now)
        self.need_reconcile = True
        self.last_error = reply.get("error") or "patch_failed"
        return reply

    def _service_local(self) -> None:
        if not self.local:
            return
        command = self.local.popleft()
        if command[0] == "save":
            self._save_scene(command[1])
        elif command[0] == "recall":
            self._queue_recall(command[1])
        elif command[0] == "defaults":
            self.discrete.append(("defaults", None))
        elif command[0] == "remap":
            self._remap(command[1], command[2])

    def _save_scene(self, slot: str) -> None:
        if self.store is None:
            self.last_error = "no scene store"
            return
        if self.unknown:
            self.last_error = "reconcile before save"
            return
        fields = {key: self.state[key] for key in SCENE_FIELDS}
        fields["color"] = _hue(self.effective_hue)
        self.store.save_scene(slot, {"schema_sha256": self.schema or "", "fields": fields})

    def _remap(self, control: str, field) -> None:
        if control not in self.bindings:
            self.last_error = "unknown control"
            return
        rule = self.bindings[control]
        if field is not None:
            if rule["kind"] == "absolute" and field not in ASSIGNABLE_ABSOLUTE:
                self.last_error = "field is not a donor fader control"
                return
            if rule["kind"] == "button" and field not in ASSIGNABLE_TOGGLE:
                self.last_error = "field is not a donor toggle"
                return
        if "motion" == field:
            self.last_error = "legacy field"
            return
        rule["field"] = field
        self.picked.pop(control, None)
        self.last_ui.pop(control, None)
        self.release_required.add(control)
        self.dirty.clear()
        self.dirty_at.clear()
        if self.store is not None:
            snapshot = {name: item.get("field") for name, item in self.bindings.items() if "field" in item}
            self.store.save_assignments(snapshot)

    def _absolute(self, name: str, raw: int) -> None:
        rule = self.bindings[name]
        field = rule.get("field")
        if not field:
            return
        gated = not self._fader_write_allowed(name)
        ui = raw / 127.0
        target = self._device_ui(field)
        previous = self.last_ui.get(name)
        self.last_ui[name] = ui
        if not self.picked.get(name):
            crossed = previous is not None and previous != ui and (previous - target) * (ui - target) <= 0
            near = abs(ui - target) <= PICKUP + 1e-12
            if crossed or near:
                self.picked[name] = True
        if gated or not self.picked.get(name):
            return
        native = self._native(field, ui)
        if _same(field, native, self.state.get(field)) and field not in self.unknown:
            return
        self.dirty[field] = native
        self.dirty_at.setdefault(field, 0.0)

    def _fader_write_allowed(self, name: str) -> bool:
        if self.bindings[name].get("write_gate") != "fader-pair":
            return True
        if name == "Fader1" and not self.seen_f1:
            self.seen_f1 = True
            self.qualification_log.append(("Fader1", 0, 40))
        elif name == "Fader4" and not self.seen_f4:
            self.seen_f4 = True
            self.qualification_log.append(("Fader4", 0, 43))
        was = self.faders_armed
        self.faders_armed = self.seen_f1 and self.seen_f4
        return was

    def _device_ui(self, field: str) -> float:
        if field == "saturation":
            raw = float(self.state["saturation"])
            return raw * raw
        if field == "hardware_output_brightness":
            return int(self.state["hardware_output_brightness"]) / 255.0
        if field == "color":
            return self.color_reference
        return float(self.state[field])

    def _native(self, field: str, ui: float):
        if field == "saturation":
            return math.sqrt(min(1.0, max(0.0, ui)))
        if field == "hardware_output_brightness":
            return int(math.floor((255.0 * ui) + 0.5))
        return ui

    def _press(self, name: str) -> None:
        rule = self.bindings[name]
        action = rule.get("action")
        if action == "mode":
            self.discrete.append(("mode", int(rule["mode"])))
        elif action == "toggle":
            self.discrete.append(("toggle", rule["field"]))
        elif action == "step":
            self._queue_step(int(rule["delta"]))
        elif action == "emission":
            self.discrete.append(("emission", bool(rule["value"])))
        elif action == "scene":
            self._queue_recall(rule["slot"])

    def _queue_step(self, delta: int) -> None:
        if "current_mode" in self.unknown:
            self.discrete.append(("step", delta))
            return
        base = self.state["current_mode"]
        for kind, arg in self.discrete:
            if kind == "mode":
                base = arg
        self.discrete.append(("mode", (int(base) + delta) % 7))

    def _queue_recall(self, slot: str) -> None:
        if self.store is None:
            self.empty_recalls += 1
            return
        scene = self.store.scene(slot)
        if not scene:
            self.empty_recalls += 1
            return
        if scene.get("schema_sha256") != self.schema:
            self.last_error = "scene_schema_mismatch"
            return
        fields = {}
        for key, value in scene["fields"].items():
            if key == "emission_enabled" or key not in SCENE_FIELDS:
                continue
            fields[key] = value
        if not fields or len(fields) > 16:
            return
        self.discrete.append(("patch", fields))

    def _next_discrete(self):
        if self.need_reconcile:
            self.need_reconcile = False
            return "reconcile", None, None
        while self.discrete:
            kind, arg = self.discrete[0]
            if kind == "toggle" and arg in self.unknown:
                return "reconcile", None, None
            if kind == "step" and "current_mode" in self.unknown:
                return "reconcile", None, None
            self.discrete.popleft()
            if kind == "step":
                self.discrete.appendleft(("mode", (int(self.state["current_mode"]) + int(arg)) % 7))
                continue
            fields = self._fields_for(kind, arg)
            if not fields:
                continue
            own = fields.get("color") if "color" in fields else None
            return "patch", fields, own
        return None, None, None

    def _fields_for(self, kind: str, arg):
        if kind == "mode":
            mode = int(arg)
            if not 0 <= mode <= 6:
                return None
            if mode == self.state["current_mode"] and "current_mode" not in self.unknown:
                return None
            return {"current_mode": mode}
        if kind == "toggle":
            return {arg: (not bool(self.state[arg]))}
        if kind == "emission":
            if self.state["emission_enabled"] is bool(arg) and "emission_enabled" not in self.unknown:
                return None
            return {"emission_enabled": bool(arg)}
        if kind == "defaults":
            return dict(DEFAULTS)
        if kind == "patch":
            return {key: value for key, value in arg.items() if key != "emission_enabled" and key in NATIVE_FIELDS}
        return None

    def _take(self, limit: int) -> dict:
        chosen = {}
        scanned = 0
        count = len(CONTINUOUS_ORDER)
        index = self.cursor % count
        while scanned < count and len(chosen) < limit:
            field = CONTINUOUS_ORDER[index]
            if field in self.dirty:
                chosen[field] = self.dirty.pop(field)
                self.dirty_at.pop(field, None)
            index = (index + 1) % count
            scanned += 1
        self.cursor = index
        return chosen

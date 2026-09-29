#!/usr/bin/env python3
"""Emotiscope mixer strategy. A leaked write, a retried patch, or a legacy op fails."""
from __future__ import annotations

import json
import math
import socket
import stat
import tempfile
import threading
import time
import unittest
import urllib.request
from pathlib import Path

from smc_titan_control.emotiscope_store import digest

from smc_titan_control.broker_client import (
    SINGLE_ATTEMPT_OPS,
    BrokerClient,
    UnixBroker,
)
from smc_titan_control.cli import build_parser
from smc_titan_control.emotiscope_engine import (
    CONTINUOUS_ORDER,
    DEFAULTS,
    SCENE_FIELDS,
    EmotiscopeEngine,
    load_profile,
    validate_profile,
)
from smc_titan_control.emotiscope_live import PortError, admit_endpoint
from smc_titan_control.emotiscope_store import EmotiscopeStore
from smc_titan_control.emotiscope_dashboard import make_emotiscope_dashboard


SCHEMA = "a" * 64
OTHER = "b" * 64


def public_state(**over):
    state = {
        "boot_id": 4242,
        "revision": 3,
        "schema_sha256": SCHEMA,
        "brightness": 1.0,
        "softness": 0.25,
        "color": 0.33,
        "effective_hue": 0.33,
        "warmth": 0.0,
        "color_range": 0.0,
        "speed": 0.50,
        "saturation": 0.75,
        "background": 0.25,
        "mode": 1,
        "mirror_mode": True,
        "screensaver": True,
        "temporal_dithering": True,
        "auto_color_cycle": False,
        "reverse_color_range": False,
        "emission_enabled": True,
        "hardware_output_brightness": 100,
        "applicability": 0xDFDF,
        "speed_applicable": False,
        "reverse_applicable": False,
        "accepted_request_id": 1,
        "rendered_request_id": 1,
        "emitted_request_id": 1,
    }
    state.update(over)
    return state


class DonorDouble:
    """Hello has no boot id. The first get discovers it. Legacy patch is a failure."""

    def __init__(self, state=None, hello_schema=SCHEMA):
        self.state = public_state() if state is None else state
        self.hello_schema = hello_schema
        self.ops = []
        self.fail_next = None

    def hello(self):
        self.ops.append(("hello",))
        return {"ok": True, "session": "s", "bound": {
            "schema_sha256": self.hello_schema,
            "runtime_kind": "live",
        }}

    def emotiscope_get(self, boot_id, request_id=None):
        self.ops.append(("emotiscope_get", int(boot_id), request_id))
        if int(boot_id) != int(self.state["boot_id"]):
            return {"ok": False, "error": "wrong_boot", "state": dict(self.state)}
        return {"ok": True, "state": dict(self.state)}

    def emotiscope_patch(self, boot_id, revision, fields, request_id=None):
        self.ops.append(("emotiscope_patch", int(boot_id), int(revision), dict(fields), request_id))
        if self.fail_next:
            kind = self.fail_next
            self.fail_next = None
            if kind == "timeout":
                return {"ok": False, "error": "timeout"}
            if kind == "conflict":
                return {"ok": False, "error": "request_conflict", "state": dict(self.state)}
            if kind == "stale":
                return {"ok": False, "error": "stale", "state": dict(self.state)}
        if int(boot_id) != int(self.state["boot_id"]):
            return {"ok": False, "error": "wrong_boot", "state": dict(self.state)}
        for key, value in fields.items():
            if key == "current_mode":
                self.state["mode"] = int(value)
                self.state["speed_applicable"] = int(value) == 0
            elif key == "color":
                self.state["color"] = float(value)
                self.state["effective_hue"] = float(value) % 1.0
            elif key == "color_range":
                self.state["color_range"] = float(value)
                self.state["reverse_applicable"] = float(value) > 0
            else:
                self.state[key] = value
        self.state["revision"] = int(self.state["revision"]) + 1
        self.state["accepted_request_id"] = request_id
        return {"ok": True, "state": dict(self.state)}

    def patch(self, *args, **kwargs):
        raise AssertionError("legacy patch must not be called")


def engine(donor=None, store=None):
    donor = donor or DonorDouble()
    found = EmotiscopeEngine(donor, load_profile(), store)
    found.attach(0.0)
    return found


def release(found, channel, cc):
    found.observe(channel, cc, 0)


def press(found, channel, cc):
    release(found, channel, cc)
    found.observe(channel, cc, 127)


def patches(donor):
    return [op for op in donor.ops if op[0] == "emotiscope_patch"]


class AttachAndIdentityTests(unittest.TestCase):
    def test_profile_loads_and_keeps_both_collision_hashes(self):
        profile = load_profile()
        provenance = profile["provenance"]
        self.assertNotEqual(
            provenance["collision_capture_current_sha256"],
            provenance["collision_capture_profile_cited_sha256"],
        )
        self.assertFalse(any(rule["qualified"] for rule in profile["bindings"].values()))
        self.assertFalse(any(rule["qualified"] for rule in profile["witnesses"].values()))

    def test_attach_learns_boot_from_wrong_boot_and_sends_no_patch(self):
        donor = DonorDouble()
        found = engine(donor)
        gets = [op for op in donor.ops if op[0] == "emotiscope_get"]
        self.assertEqual(gets[0][1], 0)
        self.assertEqual(gets[1][1], 4242)
        self.assertEqual(patches(donor), [])
        self.assertTrue(found.ready)
        self.assertEqual(found.boot_id, 4242)
        self.assertEqual(found.color_reference, 0.33)
        self.assertTrue(found.release_required)

    def test_schema_mismatch_refuses_to_patch(self):
        donor = DonorDouble(hello_schema=OTHER)
        found = EmotiscopeEngine(donor, load_profile(), None)
        found.attach(0.0)
        self.assertFalse(found.ready)
        self.assertEqual(found.last_error, "schema_mismatch")
        found.observe(0, 41, 0)
        found.tick(0.0)
        self.assertEqual(patches(donor), [])

    def test_channel_1_cc40_is_r5_and_never_brightness(self):
        donor = DonorDouble()
        found = engine(donor)
        found.observe(1, 40, 0)
        found.observe(1, 40, 127)
        found.observe(1, 43, 64)
        found.tick(0.0)
        self.assertEqual(patches(donor), [])
        self.assertEqual(found.recent[-1]["name"], "R8")
        self.assertEqual(found.recent[-3]["name"], "R5")
        self.assertNotIn("brightness", found.dirty)

    def test_fader1_and_fader4_arm_without_writing_including_the_completing_event(self):
        donor = DonorDouble()
        found = engine(donor)
        found.observe(0, 40, 127)
        found.observe(0, 43, 32)
        self.assertTrue(found.faders_armed)
        self.assertEqual([item[0] for item in found.qualification_log], ["Fader1", "Fader4"])
        found.tick(0.0)
        self.assertEqual(patches(donor), [])
        found.observe(0, 40, 64)
        result = found.tick(0.0)
        sent = patches(donor)
        self.assertEqual(len(sent), 1)
        self.assertEqual(set(sent[0][3]), {"brightness"})
        self.assertAlmostEqual(sent[0][3]["brightness"], 64 / 127)
        self.assertEqual(result["ipc"], "patch")

    def test_duplicate_identity_and_aliased_witness_are_rejected(self):
        profile = load_profile()
        profile["bindings"]["Alias"] = dict(profile["bindings"]["Fader2"])
        with self.assertRaises(ValueError):
            validate_profile(profile)
        profile = load_profile()
        profile["bindings"]["Stolen"] = {
            "channel": 1, "cc": 40, "kind": "absolute", "field": "brightness",
        }
        with self.assertRaises(ValueError):
            validate_profile(profile)


class TransformAndPickupTests(unittest.TestCase):
    def test_saturation_square_root_is_applied_once_and_a_held_fader_does_not_resend(self):
        donor = DonorDouble()
        found = engine(donor)
        found.observe(0, 46, 0)
        found.observe(0, 46, 100)
        found.tick(0.0)
        sent = patches(donor)
        self.assertEqual(len(sent), 1)
        raw = math.sqrt(100 / 127)
        self.assertAlmostEqual(sent[0][3]["saturation"], raw)
        self.assertNotAlmostEqual(sent[0][3]["saturation"], 100 / 127)
        self.assertAlmostEqual(found._device_ui("saturation"), raw * raw)
        found.observe(0, 46, 100)
        found.tick(0.1)
        self.assertEqual(len(patches(donor)), 1)

    def test_auto_color_readback_does_not_move_the_sampled_hue_or_chase(self):
        parked = 64 / 127
        donor = DonorDouble(public_state(
            auto_color_cycle=True, effective_hue=parked, color=parked,
        ))
        found = engine(donor)
        self.assertAlmostEqual(found.color_reference, parked)
        donor.state["effective_hue"] = 0.80
        found.tick(1.0)
        self.assertAlmostEqual(found.color_reference, parked)
        self.assertEqual(found.state["color"], parked)
        self.assertEqual(patches(donor), [])
        found.observe(0, 44, 64)
        found.tick(1.1)
        self.assertEqual(patches(donor), [])
        found.observe(0, 44, 80)
        found.tick(1.2)
        sent = patches(donor)
        self.assertEqual(len(sent), 1)
        self.assertAlmostEqual(sent[0][3]["color"], 80 / 127)
        self.assertAlmostEqual(found.color_reference, 80 / 127)

    def test_pickup_accepts_one_step_or_a_crossing_and_ignores_a_distant_fader(self):
        donor = DonorDouble()
        found = engine(donor)
        found.observe(0, 41, 0)
        found.tick(0.0)
        self.assertEqual(patches(donor), [])
        self.assertFalse(found.picked.get("Fader2"))
        near = round(0.25 * 127)
        found.state["softness"] = near / 127
        found.observe(0, 41, near)
        found.tick(0.1)
        self.assertTrue(found.picked.get("Fader2"))
        self.assertEqual(patches(donor), [])
        found.picked["Fader8"] = False
        found.last_ui["Fader8"] = 0.0
        found.observe(0, 47, 127)
        self.assertTrue(found.picked.get("Fader8"))

    def test_output_brightness_rounds_half_up_and_is_not_donor_brightness(self):
        donor = DonorDouble()
        found = engine(donor)
        cc = 51
        found.observe(0, 30, round(100 / 255 * 127))
        found.observe(0, 30, cc)
        found.tick(0.0)
        sent = patches(donor)
        self.assertEqual(len(sent), 1)
        expected = int(math.floor((255.0 * (cc / 127.0)) + 0.5))
        self.assertEqual(sent[0][3], {"hardware_output_brightness": expected})
        self.assertNotIn("brightness", sent[0][3])
        self.assertNotEqual(expected, cc)


class ButtonAndSceneTests(unittest.TestCase):
    def test_button_is_ignored_until_release_and_two_presses_toggle_opposite_ways(self):
        donor = DonorDouble()
        found = engine(donor)
        found.observe(0, 28, 127)
        found.tick(0.0)
        self.assertEqual(patches(donor), [])
        press(found, 0, 28)
        press(found, 0, 28)
        first = found.tick(0.0)
        second = found.tick(0.1)
        sent = patches(donor)
        self.assertEqual(sent[0][3], {"mirror_mode": False})
        self.assertEqual(sent[1][3], {"mirror_mode": True})
        self.assertEqual(first["ipc"], "patch")
        self.assertEqual(second["ipc"], "patch")

    def test_timeout_then_the_next_press_reads_before_toggling_confirmed_state(self):
        donor = DonorDouble()
        found = engine(donor)
        donor.fail_next = "timeout"
        press(found, 0, 28)
        failed = found.tick(0.0)
        self.assertFalse(failed["reply"]["ok"])
        self.assertIn("mirror_mode", found.unknown)
        press(found, 0, 28)
        reconcile = found.tick(0.1)
        self.assertEqual(reconcile["ipc"], "get")
        self.assertEqual(len(patches(donor)), 1)
        follow = found.tick(0.2)
        self.assertEqual(follow["ipc"], "patch")
        self.assertEqual(patches(donor)[-1][3], {"mirror_mode": False})
        self.assertTrue(donor.state["mirror_mode"] is False)

    def test_previous_and_next_use_queued_mode_and_wrap_inside_0_to_6(self):
        donor = DonorDouble(public_state(mode=6))
        found = engine(donor)
        press(found, 0, 58)
        press(found, 0, 58)
        found.tick(0.0)
        found.tick(0.1)
        sent = [op[3]["current_mode"] for op in patches(donor)]
        self.assertEqual(sent, [0, 1])
        donor.state["mode"] = 0
        found.state["current_mode"] = 0
        press(found, 0, 57)
        press(found, 0, 57)
        found.tick(0.2)
        found.tick(0.3)
        self.assertEqual([op[3]["current_mode"] for op in patches(donor)][-2:], [6, 5])

    def test_neutral_and_self_test_are_never_sent(self):
        donor = DonorDouble()
        found = engine(donor)
        found.observe(0, 27, 0)
        found.observe(0, 27, 127)
        found.discrete.append(("mode", 7))
        found.discrete.append(("mode", 8))
        found.tick(0.0)
        found.tick(0.1)
        self.assertEqual(patches(donor), [])

    def test_play_is_idempotent_and_pause_is_explicit(self):
        donor = DonorDouble()
        found = engine(donor)
        press(found, 0, 52)
        found.tick(0.0)
        self.assertEqual(patches(donor), [])
        press(found, 0, 53)
        found.tick(0.1)
        self.assertEqual(patches(donor)[-1][3], {"emission_enabled": False})

    def test_abandon_overflow_remap_and_save_do_not_change_emission_or_send_defaults(self):
        donor = DonorDouble()
        with tempfile.TemporaryDirectory() as temp:
            store = EmotiscopeStore(
                Path(temp) / "assignments.json",
                Path(temp) / "scenes.json",
            )
            found = engine(donor, store)
            press(found, 0, 53)
            found.abandon()
            found.tick(0.0)
            found.note_overflow()
            found.tick(0.1)
            found.ready = True
            found.fault = None
            found.request_remap("Fader8", "softness")
            found.request_save("SQ1")
            found.tick(0.2)
            found.tick(0.3)
            self.assertEqual(patches(donor), [])
            self.assertNotIn("emission_enabled", store.scenes["SQ1"]["fields"])
            self.assertEqual(store.assignments["Fader8"], "softness")

    def test_empty_and_wrong_schema_scenes_send_nothing_and_emission_is_stripped(self):
        donor = DonorDouble()
        with tempfile.TemporaryDirectory() as temp:
            store = EmotiscopeStore(Path(temp) / "a.json", Path(temp) / "s.json")
            found = engine(donor, store)
            press(found, 1, 45)
            found.tick(0.0)
            self.assertEqual(found.empty_recalls, 1)
            store.scenes["SQ1"] = {
                "schema_sha256": OTHER,
                "fields": {"brightness": 0.2, "emission_enabled": False},
            }
            press(found, 1, 44)
            found.tick(0.1)
            store.scenes["SQ1"] = {
                "schema_sha256": SCHEMA,
                "fields": {"brightness": 0.2, "emission_enabled": False, "motion": 1},
            }
            # Direct recall skips the validator. The engine still drops emission
            # and refuses a field the donor patch does not own.
            found.request_recall("SQ1")
            found.tick(0.2)
            sent = patches(donor)
            self.assertEqual(len(sent), 1)
            self.assertEqual(sent[0][3], {"brightness": 0.2})
            self.assertNotIn("emission_enabled", sent[0][3])
            self.assertNotIn("motion", sent[0][3])

    def test_scene_recall_is_one_fifteen_field_patch_and_defaults_are_one_sixteen_field_patch(self):
        donor = DonorDouble()
        with tempfile.TemporaryDirectory() as temp:
            store = EmotiscopeStore(Path(temp) / "a.json", Path(temp) / "s.json")
            found = engine(donor, store)
            fields = {key: DEFAULTS[key] for key in SCENE_FIELDS}
            fields["brightness"] = 0.4
            fields["color"] = 0.2
            store.save_scene("SQ3", {"schema_sha256": SCHEMA, "fields": fields})
            press(found, 1, 46)
            found.tick(0.0)
            sent = patches(donor)
            self.assertEqual(len(sent), 1)
            self.assertEqual(set(sent[0][3]), set(SCENE_FIELDS))
            self.assertNotIn("emission_enabled", sent[0][3])
            found.request_defaults()
            found.tick(0.1)
            defaults = patches(donor)[-1][3]
            self.assertEqual(set(defaults), set(DEFAULTS))
            self.assertEqual(defaults["current_mode"], 1)
            self.assertEqual(defaults["hardware_output_brightness"], 100)
            self.assertIs(defaults["emission_enabled"], True)
            self.assertEqual(defaults["saturation"], 0.75)

    def test_nine_dirty_fields_share_two_ticks_and_a_button_is_not_starved(self):
        donor = DonorDouble()
        found = engine(donor)
        for index, field in enumerate(CONTINUOUS_ORDER):
            found.dirty[field] = 0.1 if field != "hardware_output_brightness" else 40 + index
        first = found.tick(0.0)
        second = found.tick(0.1)
        self.assertEqual(len(patches(donor)[0][3]), 8)
        self.assertEqual(len(patches(donor)[1][3]), 1)
        covered = set(patches(donor)[0][3]) | set(patches(donor)[1][3])
        self.assertEqual(covered, set(CONTINUOUS_ORDER))
        found.dirty["brightness"] = 0.2
        press(found, 0, 28)
        found.tick(0.2)
        self.assertEqual(patches(donor)[-1][3], {"mirror_mode": False})
        self.assertIn("brightness", found.dirty)


class PersistenceDashboardAndLaunchTests(unittest.TestCase):
    def test_store_rejects_bad_checksum_version_and_legacy_names_and_a_missing_file_does_not_write(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            assignments = root / "assignments.json"
            scenes = root / "scenes.json"
            missing = EmotiscopeStore(assignments, scenes)
            self.assertIsNone(missing.error)
            self.assertEqual(missing.assignments, {})
            donor = DonorDouble()
            found = engine(donor, missing)
            found.tick(0.0)
            self.assertEqual(patches(donor), [])
            missing.save_assignments({"Fader2": "softness"})
            mode = stat.S_IMODE(assignments.stat().st_mode)
            self.assertEqual(mode, 0o600)
            body = json.loads(assignments.read_text())
            self.assertEqual(set(body), {"snapshot", "sha256"})
            body["snapshot"]["version"] = 2
            body["sha256"] = digest(body["snapshot"])
            assignments.write_text(json.dumps(body))
            rejected = EmotiscopeStore(assignments, scenes)
            self.assertIn("version", rejected.error)
            body["snapshot"]["version"] = 1
            body["snapshot"]["assignments"] = {"motion": "brightness"}
            body["sha256"] = digest(body["snapshot"])
            assignments.write_text(json.dumps(body))
            rejected = EmotiscopeStore(assignments, scenes)
            self.assertIn("legacy", rejected.error)
            body["sha256"] = "0" * 64
            assignments.write_text(json.dumps(body))
            rejected = EmotiscopeStore(assignments, scenes)
            self.assertIn("checksum", rejected.error)
            with self.assertRaises(ValueError):
                missing.save_scene("SQ1", {"schema_sha256": SCHEMA, "fields": {"motion": 1}})

    def test_dashboard_get_does_not_write_and_post_only_enqueues(self):
        donor = DonorDouble()
        found = engine(donor)
        http = make_emotiscope_dashboard(found.status, found, 0)
        thread = threading.Thread(target=http.serve_forever, daemon=True)
        thread.start()
        port = http.server_address[1]
        try:
            base = f"http://127.0.0.1:{port}"
            page = urllib.request.urlopen(base + "/", timeout=2)
            self.assertIn(b"not a picture of the lights", page.read())
            status = json.loads(urllib.request.urlopen(base + "/api/status", timeout=2).read())
            self.assertFalse(status["optical_proof"])
            self.assertEqual(patches(donor), [])
            token = json.loads(urllib.request.urlopen(base + "/api/session", timeout=2).read())["token"]
            request = urllib.request.Request(
                base + "/api/defaults",
                data=b"{}",
                headers={"Content-Type": "application/json", "X-SMC-Token": token},
                method="POST",
            )
            reply = urllib.request.urlopen(request, timeout=2)
            self.assertEqual(reply.status, 202)
            self.assertEqual(patches(donor), [])
            self.assertEqual(found.local[0][0], "defaults")
        finally:
            http.shutdown()
            http.server_close()

    def test_endpoint_admission_refuses_private_wrong_id_and_ambiguous_rows(self):
        usb = {
            "name": "SINCO SMC-Mixer-Master",
            "driver": "com.apple.AppleMIDIUSBDriver",
            "unique_id": 1873467316,
            "offline": 0,
        }
        self.assertEqual(admit_endpoint([usb], usb["name"])["transport"], "usb")
        with self.assertRaises(PortError):
            admit_endpoint([usb], "SMC-Mixer Private")
        wrong = dict(usb, unique_id=1)
        with self.assertRaises(PortError):
            admit_endpoint([wrong], usb["name"])
        with self.assertRaises(PortError):
            admit_endpoint([usb, usb], usb["name"])

    def test_cli_keeps_performance_and_parses_emotiscope_paths(self):
        parser = build_parser()
        emotiscope = parser.parse_args(["live", "--socket", "/tmp/x.sock", "--strategy", "emotiscope"])
        performance = parser.parse_args(["live", "--socket", "/tmp/x.sock", "--strategy", "performance"])
        self.assertEqual(emotiscope.strategy, "emotiscope")
        self.assertEqual(performance.strategy, "performance")
        self.assertTrue(str(emotiscope.emotiscope_scenes).endswith("smc-emotiscope-scenes.json"))
        self.assertTrue(str(performance.scenes).endswith("smc-performance-scenes.json"))

    def test_patch_is_single_attempt_and_get_is_not(self):
        self.assertIn("emotiscope_patch", SINGLE_ATTEMPT_OPS)
        self.assertNotIn("emotiscope_get", SINGLE_ATTEMPT_OPS)
        seen = []

        class Transport:
            def request(self, payload):
                seen.append(payload["op"])
                return {"ok": True, "session": "s", "bound": {}}

        client = BrokerClient(Transport())
        client.hello()
        client.emotiscope_get(1, 2)
        client.emotiscope_patch(1, 3, {})
        self.assertEqual(seen, ["hello", "emotiscope_get", "emotiscope_patch"])

    def test_unix_broker_does_not_retry_a_donor_patch(self):
        path = Path("/tmp/smc-emotiscope-attempt-test.sock")
        if path.exists():
            path.unlink()
        server_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        server_sock.bind(str(path))
        server_sock.listen(4)
        server_sock.settimeout(1)
        attempts = {"n": 0}

        def serve():
            while attempts["n"] < 3:
                try:
                    conn, _ = server_sock.accept()
                except OSError:
                    return
                attempts["n"] += 1
                try:
                    conn.recv(256)
                except OSError:
                    pass
                conn.close()

        thread = threading.Thread(target=serve, daemon=True)
        thread.start()
        time.sleep(0.05)
        broker = UnixBroker(str(path), timeout=0.5)
        try:
            patch = broker.request({"op": "emotiscope_patch", "fields": {}})
            get = broker.request({"op": "emotiscope_get"})
        finally:
            broker.close()
            server_sock.close()
            if path.exists():
                path.unlink()
        self.assertEqual(patch.get("error"), "broker_unavailable")
        self.assertEqual(get.get("error"), "broker_unavailable")
        self.assertEqual(attempts["n"], 3)


if __name__ == "__main__":
    unittest.main()

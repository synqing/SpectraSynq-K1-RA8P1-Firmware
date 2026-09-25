#!/usr/bin/env python3
"""Host negatives for the A8 experiment entry. Never opens a CDC device."""
from __future__ import annotations

import hashlib
import json
import struct
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools" / "serial-studio"))
sys.path.insert(0, str(ROOT / "scripts"))

import a8_bench_experiment as exp  # noqa: E402
import live_lease  # noqa: E402
import live_protocol as proto  # noqa: E402

IMAGE = {
    "uid": "545433931bd25436593630352d068363",
    "build": "db5f1ca7",
    "schema_sha256": "schema",
    "protocol": 1,
    "source": "6b1e7bc5",
    "contract": "sr24000.hop180.bins80.xover40",
}


def packet(**overrides) -> dict:
    base = {
        "image": dict(IMAGE),
        "stages": {
            "emit_off_observe": {
                "host_runnable": True,
                "max_duration_s": 30,
                "samples": 2,
                "interval_s": 0,
            }
        },
        "admitted_changes": [],
        "expected_revision": None,
        "expected_profile": {"wire_profile": 3, "configured_backend": "ws2816_gpt_pair"},
        "fixture_injection": {"supported": False, "absence_reason": "no pair injection opcode"},
        "wiring": {"din_b_on_p603": False, "break_80_81_verified": False},
        "output_permission": None,
        "current_limit": {},
        "music": {"named_music": False},
        "max_record_bytes": 2_000_000,
    }
    base.update(overrides)
    return base


def write_packet(directory: Path, body: dict) -> Path:
    path = directory / "packet.json"
    path.write_text(json.dumps(body), encoding="utf-8")
    return path


def metrics(hops: int, **extra) -> bytes:
    pdm = {
        "stream_epoch": 1,
        "pair_epoch_drops": 0,
        "asrc_consumed": 100 + hops,
        "asrc_discarded": 0,
        "push_rejected": 0,
        "stale_discards": 0,
        "ap_hops": hops,
        "paired_slots": extra.pop("paired_slots", 3),
        "sample_rate_match": False,
    }
    pdm.update(extra)
    return json.dumps({"pdm_target": pdm}).encode()


def status(**extra) -> dict:
    body = {
        "wire_profile": 3,
        "configured_backend": "ws2816_gpt_pair",
        "emit_enabled": False,
        "mode_a": 32,
        "mode_b": 32,
        "brightness": 24,
        "submitted_generation": 4,
        "completed_generation": 4,
        "pair_completions": 1,
        "pending_replacements": 0,
        "lane_a0_fault": 0,
        "lane_a1_fault": 0,
        "pair_fault": 0,
        "stream_epoch": 1,
    }
    body.update(extra)
    return body


class FakeSession:
    def __init__(self, info: dict, metric_bodies: list[bytes], palette: dict, config: bytes | None = None, fail_metrics: dict | None = None) -> None:
        self._info = info
        self.metric_bodies = list(metric_bodies)
        self.palette = palette
        self.config = config if config is not None else proto.pack_config({"revision": 1, "emit_on": 0})
        self.fail_metrics = fail_metrics
        self.calls: list[tuple] = []
        self.closed = False
        self.lease_token = None

    def info(self) -> dict:
        self.calls.append(("info", 1, b""))
        return {"ok": True, "info": self._info}

    def transact(self, op: int, payload: bytes = b"") -> dict:
        self.calls.append(("transact", op, bytes(payload)))
        if self.fail_metrics and op == exp.OP_METRICS:
            return self.fail_metrics
        if op == exp.OP_METRICS:
            body = self.metric_bodies.pop(0)
            return {"ok": True, "status": 0, "body": body}
        if op == exp.OP_PALETTE_STATUS:
            return {"ok": True, "status": 0, "body": json.dumps(self.palette).encode()}
        if op == exp.OP_SNAPSHOT:
            return {"ok": False, "status": 1, "body": b""}
        if op == exp.OP_CONFIG:
            return {"ok": True, "status": 0, "body": self.config}
        return {"ok": False, "status": 9, "body": b""}

    def close(self) -> None:
        self.closed = True

    def control_writes(self) -> list:
        return [call for call in self.calls if call[0] == "transact" and exp.is_control_write(call[1], call[2])]


def assert_no_writes(session: FakeSession) -> None:
    assert session.control_writes() == []


def test_wrong_build_denies_before_write() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        path = write_packet(root, packet())
        evidence = root / "out"
        session = FakeSession({**IMAGE, "build": "ffffffffffffffff"}, [metrics(1), metrics(2)], status())
        opened = {"n": 0}

        def open_session():
            opened["n"] += 1
            return session

        result = exp.dispatch("emit-off", path, evidence, open_session)
        assert result["verdict"] == "DENIED"
        assert result["reason"] == "build mismatch"
        assert result["before_write"] is True
        assert opened["n"] == 1
        assert session.calls == [("info", 1, b"")]
        assert_no_writes(session)
        assert session.closed is True


def test_missing_permission_and_wiring_do_not_open() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        path = write_packet(root, packet())
        opened = {"n": 0}

        def open_session():
            opened["n"] += 1
            raise AssertionError("device opened")

        result = exp.dispatch("physical", path, root / "phys", open_session)
        assert result["verdict"] == "BLOCKED"
        assert result["device_opened"] is False
        assert opened["n"] == 0
        text = " ".join(result["blockers"])
        assert "P603/U18 pin 33" in text
        assert "AUTHORISED_THIS_PAIR_PACKET" in text


def test_permission_without_injection_still_does_not_open() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = packet(
            output_permission="AUTHORISED_THIS_PAIR_PACKET",
            wiring={"din_b_on_p603": True, "break_80_81_verified": True},
            current_limit={"milliamps": 100},
            music={"named_music": True},
        )
        path = write_packet(root, body)
        opened = {"n": 0}

        def open_session():
            opened["n"] += 1
            raise AssertionError("device opened")

        result = exp.dispatch("physical", path, root / "phys", open_session)
        assert opened["n"] == 0
        assert result["device_opened"] is False
        assert any("injection" in item for item in result["blockers"])


def test_cleared_external_gates_run_mapping_and_black_stop_without_enabling_emit() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        lane0, lane1 = _lanes(root)
        body = packet(
            fixture_injection={"supported": True},
            output_permission="AUTHORISED_THIS_PAIR_PACKET",
            wiring={"din_b_on_p603": True, "break_80_81_verified": True},
            current_limit={"milliamps": 80, "meter": True, "class": "bench-measured"},
            mapping={"fixtures": [{"lane0": str(lane0), "lane1": str(lane1), "generation": 1}]},
            stages={
                "emit_off_observe": {"host_runnable": True, "max_duration_s": 30, "samples": 2, "interval_s": 0},
                "physical_mapping": {"max_duration_s": 30, "interval_s": 0},
            },
        )
        path = write_packet(root, body)
        session = _MapSession()
        result = exp.dispatch("physical", path, root / "phys", lambda: session)
        assert result["verdict"] == "SEQUENCED"
        assert result["physical_observation"] is False
        assert result["darkness_verified"] is True
        assert result["emit_enabled_by_this_run"] is False
        assert result["host_can_switch_led_supply"] is False
        fixture_ops = [call for call in session.calls if call[0] == "transact" and call[1] == exp.OP_PAIR_FIXTURE]
        assert len(fixture_ops) == 2
        assert len(fixture_ops[0][2]) == exp.PAIR_FIXTURE_BYTES
        assert fixture_ops[1][2][24:] == bytes(exp.PAIR_LANE_BYTES * 2)
        assert_no_writes(session)
        assert session.closed is True
        assert json.loads((root / "phys" / "result.json").read_text(encoding="utf-8")) == result


def test_stale_revision_denies_before_commit() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = packet(admitted_changes=[{"field": "palette_a", "value": 34}], expected_revision=9)
        path = write_packet(root, body)
        session = FakeSession(dict(IMAGE), [metrics(1), metrics(2)], status(), config=proto.pack_config({"revision": 8, "emit_on": 0}))
        result = exp.dispatch("emit-off", path, root / "out", lambda: session)
        assert result["verdict"] == "DENIED"
        assert result["reason"] == "stale config revision"
        assert result["before_write"] is True
        assert_no_writes(session)
        assert any(call[1] == exp.OP_CONFIG and exp.config_sub(call[2]) == proto.CFG_GET_CONFIG for call in session.calls if call[0] == "transact")


def test_missing_metric_stays_unknown_and_zero_stays_zero() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        path = write_packet(root, packet())
        first = metrics(10, paired_slots=0)
        second = metrics(25, paired_slots=0)
        session = FakeSession(dict(IMAGE), [first, second], status())
        result = exp.dispatch("emit-off", path, root / "out", lambda: session)
        assert result["verdict"] == "PASS"
        assert result["emit_enabled_by_this_run"] is False
        sample = json.loads((root / "out" / "sample-0000.json").read_text(encoding="utf-8"))
        assert sample["counters"]["gain_clip_pos"] == "UNKNOWN"
        assert sample["counters"]["gain_clip_neg"] == "UNKNOWN"
        assert sample["counters"]["paired_slots"] == 0
        assert sample["counters"]["sample_rate_match"] is False
        assert result["account"]["sample_rate_match"] is False
        assert result["account"]["deltas"]["ap_hops"] == 15
        assert result["account"]["deltas"]["gain_clip_pos"] == "UNKNOWN"
        assert result["runtime_acceptance"] == "PASS"
        assert result["counter_progression"] == "PASS"
        assert json.loads((root / "out" / "result.json").read_text(encoding="utf-8")) == result
        assert (root / "out" / "sample-0000-metrics.bin").read_bytes() == first
        assert_no_writes(session)


def test_truncated_and_failed_replies_record_raw_and_do_not_write() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        path = write_packet(root, packet())
        truncated = FakeSession(dict(IMAGE), [], status(), fail_metrics={"ok": True, "status": 0, "body": b"{"})
        result = exp.dispatch("emit-off", path, root / "trunc", lambda: truncated)
        assert result["verdict"] == "DENIED"
        assert result["before_write"] is True
        assert result["detail"]["kind"] == "truncated_or_invalid"
        assert_no_writes(truncated)
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        path = write_packet(root, packet())
        failed = FakeSession(dict(IMAGE), [], status(), fail_metrics={"ok": False, "status": 3, "body": b""})
        result = exp.dispatch("emit-off", path, root / "fail", lambda: failed)
        assert result["verdict"] == "DENIED"
        assert result["detail"]["kind"] == "rejected"
        assert result["detail"]["status"] == 3
        assert_no_writes(failed)


def test_recorder_failure_and_interrupt_release_the_lease() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        evidence = root / "disk"
        path = write_packet(root, packet())
        session = FakeSession(dict(IMAGE), [metrics(1), metrics(2)], status())

        def open_session():
            lease = live_lease.acquire(evidence, "recorder-test")
            session.lease_token = lease["token"]
            return session

        real = exp.EvidenceWriter.write_json

        def boom(self, name, payload):
            if name.startswith("sample-"):
                raise OSError("disk full")
            return real(self, name, payload)

        exp.EvidenceWriter.write_json = boom  # type: ignore[method-assign]
        try:
            result = exp.dispatch("emit-off", path, evidence, open_session)
        finally:
            exp.EvidenceWriter.write_json = real  # type: ignore[method-assign]
        assert result["verdict"] == "FAULT"
        assert result["emit_enabled_by_this_run"] is False
        assert session.closed is True
        assert not (evidence / "campaign.lease.json").exists()
        assert_no_writes(session)
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        evidence = root / "stop"
        path = write_packet(root, packet())

        class Interrupt(FakeSession):
            def __init__(self) -> None:
                super().__init__(dict(IMAGE), [metrics(1), metrics(2)], status())
                self.metrics_seen = 0

            def transact(self, op: int, payload: bytes = b"") -> dict:
                if op == exp.OP_METRICS:
                    self.metrics_seen += 1
                    if self.metrics_seen == 2:
                        self.calls.append(("transact", op, bytes(payload)))
                        raise KeyboardInterrupt()
                return super().transact(op, payload)

        session = Interrupt()

        def open_session():
            lease = live_lease.acquire(evidence, "interrupt-test")
            session.lease_token = lease["token"]
            return session

        result = exp.dispatch("emit-off", path, evidence, open_session)
        assert result["verdict"] == "FAULT"
        assert result["reason"] == "KeyboardInterrupt"
        assert session.closed is True
        assert not (evidence / "campaign.lease.json").exists()
        assert_no_writes(session)


def test_dry_run_does_not_open_and_names_the_forbidden_script() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        path = write_packet(root, packet())

        def open_session():
            raise AssertionError("device opened")

        result = exp.dispatch("dry-run", path, root / "dry", open_session)
        assert result["device_opened"] is False
        assert result["verdict"] == "DRY_RUN"
        assert result["forbidden_command"] == "scripts/run_mode32_real_audio.py"
        assert result["physical_blockers"]


def test_partial_pair_is_not_complete_and_units_differ() -> None:
    rows = [
        {"counters": {"ap_hops": 1, "asrc_consumed": 180, "paired_slots": 1, "gain_clip_pos": exp.UNKNOWN}, "snapshot": {"hop_sequence": 1}, "palette": {}},
        {"counters": {"ap_hops": 3, "asrc_consumed": 540, "paired_slots": 2, "gain_clip_pos": exp.UNKNOWN}, "snapshot": {"hop_sequence": 3}, "palette": {"submitted_generation": 9, "completed_generation": 8, "pending_replacements": 1}},
    ]
    account = exp.account_window(rows)
    assert account["deltas"]["ap_hops"] == 2
    assert account["deltas"]["asrc_consumed"] == 360
    assert account["deltas"]["ap_hops"] != account["deltas"]["asrc_consumed"]
    assert account["pair_join"]["pair_complete"] is False
    assert account["pair_join"]["partial_pair_counted_complete"] is False
    assert account["deltas"]["gain_clip_pos"] == "UNKNOWN"
    assert exp.progression_verdict(rows, account) == "PASS"
    stalled = [
        {"counters": {"ap_hops": 4}, "snapshot": {"hop_sequence": 4}, "palette": {}},
        {"counters": {"ap_hops": 4}, "snapshot": {"hop_sequence": 4}, "palette": {}},
    ]
    stalled_account = exp.account_window(stalled)
    assert stalled_account["deltas"]["ap_hops"] == 0
    assert exp.progression_verdict(stalled, stalled_account) == "FAIL"


def test_silence_is_not_named_music() -> None:
    assert exp.pcm_s16le_report(b"\x00\x00" * 8)["named_music"] is False
    assert exp.pcm_s16le_report(b"\x00\x00" * 8)["non_silent"] is False
    assert exp.pcm_s16le_report(b"\x00\x00" * 8)["peak_abs"] == 0
    tone = exp.pcm_s16le_report(b"\x00\x00\x01\x00")
    assert tone["named_music"] is False
    assert tone["non_silent"] is True
    assert tone["nonzero_samples"] == 1
    assert tone["peak_abs"] == 1


def _lanes(directory: Path) -> tuple[Path, Path]:
    lane0 = directory / "lane0.bin"
    lane1 = directory / "lane1.bin"
    lane0.write_bytes(bytes([0x11]) + bytes(exp.PAIR_LANE_BYTES - 1))
    lane1.write_bytes(bytes([0x22]) + bytes(exp.PAIR_LANE_BYTES - 1))
    return lane0, lane1


def _mapping_packet(directory: Path, **overrides) -> dict:
    lane0, lane1 = _lanes(directory)
    body = packet(
        fixture_injection={"supported": True},
        output_permission="AUTHORISED_THIS_PAIR_PACKET",
        wiring={"din_b_on_p603": True, "break_80_81_verified": True},
        current_limit={"milliamps": 80, "meter": True, "class": "bench-measured"},
        mapping={"fixtures": [{"lane0": str(lane0), "lane1": str(lane1), "generation": 1}]},
        stages={
            "emit_off_observe": {"host_runnable": True, "max_duration_s": 30, "samples": 2, "interval_s": 0},
            "physical_mapping": {"max_duration_s": 30, "interval_s": 0},
        },
    )
    body.update(overrides)
    return body


class _MapSession(FakeSession):
    def __init__(self, palette: dict | None = None) -> None:
        initial = palette or status(
            submitted_generation=0,
            completed_generation=0,
            pair_completions=0,
            pending_replacements=0,
            lane_a0_fault=0,
            lane_a1_fault=0,
            pair_fault=0,
            stream_epoch=1,
            lane_a0_dma=0,
            lane_a1_dma=0,
            lane_a0_stop=0,
            lane_a1_stop=0,
        )
        super().__init__(dict(IMAGE), [metrics(1), metrics(2), metrics(3), metrics(4)], initial)

    def transact(self, op: int, payload: bytes = b"") -> dict:
        if op == exp.OP_PAIR_FIXTURE:
            self.calls.append(("transact", op, bytes(payload)))
            import struct
            kind = struct.unpack_from("<I", payload, 16)[0]
            generation = struct.unpack_from("<I", payload, 8)[0]
            self.palette = status(
                submitted_generation=generation,
                completed_generation=generation,
                pair_completions=1 if kind == exp.KIND_MAP else 2,
                pending_replacements=0,
                lane_a0_fault=0,
                lane_a1_fault=0,
                lane_a0_dma=1,
                lane_a1_dma=1,
                lane_a0_stop=1,
                lane_a1_stop=1,
                pair_fault=0,
                stream_epoch=1,
            )
            body = json.dumps({"accepted": True, "generation": generation, "kind": kind}).encode()
            return {"ok": True, "status": 0, "body": body}
        return super().transact(op, payload)


class _ShowMapSession(_MapSession):
    def __init__(self, *, emit_on: int = 1, delay: int = 0, restore_mismatch: bool = False, black_fault: bool = False, partial: bool = False, wrong_epoch: bool = False, omit_fault: bool = False, freeze_led: bool = False, advance_submit: bool = False) -> None:
        palette = status(
            emit_enabled=bool(emit_on),
            submitted_generation=4,
            completed_generation=4,
            pair_completions=2,
            pending_replacements=0,
            lane_a0_fault=0,
            lane_a1_fault=0,
            pair_fault=0,
            stream_epoch=1,
            emitted=50,
            emit_errors=0,
            din_a="P601",
            din_b="P604",
            lane_a0_dma=1,
            lane_a1_dma=1,
            lane_a0_stop=1,
            lane_a1_stop=1,
        )
        super().__init__(palette)
        self.hops = 4
        self.delay = delay
        self.restore_mismatch = restore_mismatch
        self.black_fault = black_fault
        self.partial = partial
        self.wrong_epoch = wrong_epoch
        self.omit_fault = omit_fault
        self.freeze_led = freeze_led
        self.advance_submit = advance_submit
        self._polls = 0
        self._pending = None
        self.cfg = {
            "revision": 4,
            "emit_on": emit_on,
            "brightness": 24,
            "flags": 5,
            "palette_a": 33,
            "palette_b": 43,
            "mode_a": 32,
            "mode_b": 32,
        }
        self.staged = b""
        self._parts: dict[int, bytes] = {}

    def transact(self, op: int, payload: bytes = b"") -> dict:
        self.calls.append(("transact", op, bytes(payload)))
        if self.advance_submit and self.palette.get("emit_enabled"):
            self.palette["submitted_generation"] = int(self.palette.get("submitted_generation") or 0) + 1
        if op == exp.OP_METRICS:
            self.hops += 1
            return {"ok": True, "status": 0, "body": metrics(self.hops, asrc_consumed=100 + self.hops * 180)}
        if op == exp.OP_CONFIG:
            sub = exp.config_sub(payload)
            if sub == proto.CFG_GET_CONFIG:
                cfg = dict(self.cfg)
                if self.restore_mismatch and self.cfg.get("emit_on") == 1 and any(
                    call[1] == exp.OP_CONFIG and exp.config_sub(call[2]) == proto.CFG_COMMIT_SET
                    for call in self.calls[:-1]
                ):
                    cfg["palette_a"] = 99
                return {"ok": True, "status": 0, "body": proto.pack_config(cfg)}
            if sub == proto.CFG_BEGIN_SET:
                self._parts = {}
                return {"ok": True, "status": 0, "body": b""}
            if sub == proto.CFG_APPEND_SET:
                rest = payload[4:]
                offset = struct.unpack_from("<I", rest)[0]
                self._parts[offset] = rest[4:]
                self.staged = b"".join(self._parts[key] for key in sorted(self._parts))
                return {"ok": True, "status": 0, "body": b""}
            if sub == proto.CFG_COMMIT_SET:
                parsed = proto.unpack_config(self.staged)
                self.cfg["revision"] = int(self.cfg["revision"]) + 1
                for key in ("palette_a", "palette_b", "mode_a", "mode_b", "emit_on", "brightness", "flags"):
                    self.cfg[key] = parsed[key]
                self.palette["emit_enabled"] = bool(parsed["emit_on"])
                return {"ok": True, "status": 0, "body": b""}
            return {"ok": False, "status": 9, "body": b""}
        if op == exp.OP_PAIR_FIXTURE:
            kind = struct.unpack_from("<I", payload, 16)[0]
            generation = struct.unpack_from("<I", payload, 8)[0]
            self._pending = (generation, kind)
            self._polls = 0
            self.palette = status(
                emit_enabled=False,
                submitted_generation=generation,
                completed_generation=generation if self.delay == 0 else generation - 1,
                pair_completions=1,
                lane_a0_fault=0,
                lane_a1_fault=0,
                pair_fault=1 if (self.black_fault and kind == exp.KIND_BLACK) else 0,
                stream_epoch=9 if self.wrong_epoch else 1,
                emitted=60,
                emit_errors=0,
                din_a="P601",
                din_b="P604",
                lane_a0_dma=1,
                lane_a1_dma=0 if self.partial else 1,
                lane_a0_stop=1,
                lane_a1_stop=0 if self.partial else 1,
            )
            if self.omit_fault:
                del self.palette["pair_fault"]
            return {"ok": True, "status": 0, "body": json.dumps({"accepted": True, "generation": generation, "kind": kind}).encode()}
        if op == exp.OP_PALETTE_STATUS:
            if not self.palette.get("emit_enabled"):
                submitted = self.palette.get("submitted_generation")
                completed = self.palette.get("completed_generation")
                if exp._whole(submitted) and exp._whole(completed) and int(submitted) > int(completed):
                    self.palette["completed_generation"] = int(completed) + 1
            if self.palette.get("emit_enabled") and not self.freeze_led:
                self.palette["emitted"] = int(self.palette.get("emitted") or 0) + 7
                self.palette["pair_completions"] = int(self.palette.get("pair_completions") or 0) + 1
            if self._pending and self.delay and self._polls < self.delay and not self.palette.get("emit_enabled"):
                self._polls += 1
                if self._polls >= self.delay:
                    generation, kind = self._pending
                    self.palette["completed_generation"] = generation
                    self.palette["lane_a1_dma"] = 1
                    self.palette["lane_a1_stop"] = 1
                    self.palette["pair_completions"] = 2 if kind == exp.KIND_BLACK else 1
            return {"ok": True, "status": 0, "body": json.dumps(self.palette).encode()}
        return {"ok": False, "status": 9, "body": b""}


def test_begin_set_is_a_control_write() -> None:
    payload = proto.pack_config_sub(proto.CFG_BEGIN_SET, b"\x00\x00")
    assert exp.is_control_write(exp.OP_CONFIG, payload) is True
    assert exp.is_control_write(exp.OP_CONFIG, proto.pack_config_sub(proto.CFG_GET_CONFIG)) is False
    assert exp.is_control_write(exp.OP_METRICS, b"") is False


def test_empty_palette_does_not_pass_on_counter_movement() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        path = write_packet(root, packet())
        session = FakeSession(dict(IMAGE), [metrics(1), metrics(4)], {})
        result = exp.dispatch("emit-off", path, root / "out", lambda: session)
        assert result["verdict"] != "PASS"
        assert result["runtime_acceptance"] == "FAIL"
        assert result["counter_progression"] == "PASS"
        assert result["reason"]
        assert_no_writes(session)


def test_framing_error_is_a_saved_fault() -> None:
    from titan_transport import FramingError

    class Broken(FakeSession):
        def transact(self, op: int, payload: bytes = b"") -> dict:
            self.calls.append(("transact", op, bytes(payload)))
            if op == exp.OP_METRICS:
                raise FramingError("injected body crc mismatch")
            return super().transact(op, payload)

    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        evidence = root / "out"
        session = Broken(dict(IMAGE), [metrics(1)], status())
        result = exp.dispatch("emit-off", write_packet(root, packet()), evidence, lambda: session)
        assert result["verdict"] == "FAULT"
        assert result["reason"] == "FramingError"
        assert "crc" in result["detail"]
        assert session.closed is True
        assert (evidence / "identity.json").is_file()
        assert json.loads((evidence / "result.json").read_text(encoding="utf-8")) == result
        assert_no_writes(session)


class _Clock:
    def __init__(self) -> None:
        self.now = 0.0

    def __call__(self) -> float:
        return self.now

    def advance(self, seconds: float) -> None:
        self.now += seconds


def test_deadline_includes_transaction_time() -> None:
    clock = _Clock()

    class Slow(FakeSession):
        def transact(self, op: int, payload: bytes = b"") -> dict:
            clock.advance(2.0)
            return super().transact(op, payload)

    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = packet(stages={
            "emit_off_observe": {
                "host_runnable": True,
                "max_duration_s": 30,
                "samples": 8,
                "interval_s": 1,
                "cleanup_allowance_s": 5,
            }
        })
        session = Slow(dict(IMAGE), [metrics(index) for index in range(1, 9)], status())

        def sleeper(seconds: float) -> None:
            clock.advance(seconds)

        result = exp.dispatch("emit-off", write_packet(root, body), root / "out", lambda: session, sleeper=sleeper, clock=clock)
        transactions = [call for call in session.calls if call[0] == "transact"]
        assert result["verdict"] == "FAULT"
        assert result["reason"] == "acquisition deadline exceeded"
        assert result["samples_retained"] == 4
        assert len(transactions) == 14
        assert result["acquisition_limit_s"] == 30
        assert result["cleanup_allowance_s"] == 5
        assert session.closed is True
        assert json.loads((root / "out" / "result.json").read_text(encoding="utf-8")) == result


def test_zero_generations_are_not_a_completed_pair() -> None:
    joined = exp.pair_join({
        "submitted_generation": 0,
        "completed_generation": 0,
        "pair_completions": 0,
    })
    assert joined["pair_complete"] is False
    done = exp.pair_join({
        "submitted_generation": 4,
        "completed_generation": 4,
        "pair_completions": 2,
        "stream_epoch": 1,
        "lane_a0_fault": 0,
        "lane_a1_fault": 0,
    })
    assert done["pair_complete"] is True


def test_malformed_and_partial_fixtures_do_not_open() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        short0 = root / "short0.bin"
        short1 = root / "short1.bin"
        short0.write_bytes(b"\x01\x02")
        short1.write_bytes(b"\x03\x04")
        body = _mapping_packet(root)
        body["mapping"] = {"fixtures": [{"lane0": str(short0), "lane1": str(short1), "generation": 1}]}
        opened = {"n": 0}

        def open_session():
            opened["n"] += 1
            raise AssertionError("device opened")

        result = exp.dispatch("physical", write_packet(root, body), root / "phys", open_session)
        assert opened["n"] == 0
        assert result["verdict"] == "BLOCKED"
        assert any("480" in item for item in result["blockers"])
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        lane0, lane1 = _lanes(root)
        body = _mapping_packet(root)
        body["mapping"]["fixtures"].append({"lane0": str(lane0), "lane1": str(lane1), "generation": 1})
        opened = {"n": 0}

        def open_session():
            opened["n"] += 1
            raise AssertionError("device opened")

        result = exp.dispatch("physical", write_packet(root, body), root / "replay", open_session)
        assert opened["n"] == 0
        assert any("repeats generation" in item for item in result["blockers"])


def test_stale_generation_is_rejected_before_the_fixture_opcode() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = _mapping_packet(root)
        session = _MapSession(status(
            submitted_generation=5,
            completed_generation=5,
            pair_completions=1,
            pending_replacements=0,
            lane_a0_fault=0,
            lane_a1_fault=0,
            stream_epoch=1,
        ))
        result = exp.dispatch("physical", write_packet(root, body), root / "phys", lambda: session)
        assert result["verdict"] == "DENIED"
        assert result["detail"]["admit"] == exp.FIXTURE_STALE
        assert not any(call[1] == exp.OP_PAIR_FIXTURE for call in session.calls if call[0] == "transact")
        assert session.closed is True


def test_lane_fault_stops_before_the_black_pair() -> None:
    class Faulty(_MapSession):
        def transact(self, op: int, payload: bytes = b"") -> dict:
            if op == exp.OP_PAIR_FIXTURE:
                self.calls.append(("transact", op, bytes(payload)))
                generation = int.from_bytes(payload[8:12], "little")
                self.palette = status(
                    submitted_generation=generation,
                    completed_generation=generation,
                    pair_completions=1,
                    lane_a0_fault=0,
                    lane_a1_fault=1,
                    pair_fault=1,
                    stream_epoch=1,
                )
                return {"ok": True, "status": 0, "body": json.dumps({"accepted": True, "generation": generation}).encode()}
            return super().transact(op, payload)

    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        session = Faulty()
        result = exp.dispatch("physical", write_packet(root, _mapping_packet(root)), root / "phys", lambda: session)
        assert result["verdict"] == "FAULT"
        assert result["darkness_verified"] is False
        assert result["darkness_unverified"] is True
        assert result["fixture_submits"] == 1
        assert sum(1 for call in session.calls if call[0] == "transact" and call[1] == exp.OP_PAIR_FIXTURE) == 1


def test_physical_interrupt_and_recorder_failure_stay_faults() -> None:
    class Stop(_MapSession):
        def transact(self, op: int, payload: bytes = b"") -> dict:
            if op == exp.OP_PAIR_FIXTURE:
                self.calls.append(("transact", op, bytes(payload)))
                raise KeyboardInterrupt()
            return super().transact(op, payload)

    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        evidence = root / "stop"
        session = Stop()

        def open_session():
            lease = live_lease.acquire(evidence, "map-interrupt")
            session.lease_token = lease["token"]
            return session

        result = exp.dispatch("physical", write_packet(root, _mapping_packet(root)), evidence, open_session)
        assert result["verdict"] == "FAULT"
        assert result["reason"] == "KeyboardInterrupt"
        assert session.closed is True
        assert not (evidence / "campaign.lease.json").exists()
        assert json.loads((evidence / "result.json").read_text(encoding="utf-8")) == result
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        evidence = root / "disk"
        session = _MapSession()

        def open_session():
            lease = live_lease.acquire(evidence, "map-recorder")
            session.lease_token = lease["token"]
            return session

        real = exp.EvidenceWriter.write_bytes

        def boom(self, name, payload):
            if name.startswith("fixture-"):
                raise OSError("disk full")
            return real(self, name, payload)

        exp.EvidenceWriter.write_bytes = boom  # type: ignore[method-assign]
        try:
            result = exp.dispatch("physical", write_packet(root, _mapping_packet(root)), evidence, open_session)
        finally:
            exp.EvidenceWriter.write_bytes = real  # type: ignore[method-assign]
        assert result["verdict"] == "FAULT"
        assert result["reason"] == "OSError"
        assert session.closed is True
        assert not (evidence / "campaign.lease.json").exists()


def test_serial_open_failure_releases_the_handle() -> None:
    released = []

    def acquire(device, owner):
        return {"device": device, "owner": owner}

    def open_port(device, handle):
        raise OSError("open failed")

    try:
        exp.open_locked_port("fake", "owner", acquire=acquire, open_port=open_port, release=released.append)
        raise AssertionError("open should fail")
    except OSError as exc:
        assert str(exc) == "open failed"
    assert released == [{"device": "fake", "owner": "owner"}]


def test_dead_pid_text_does_not_steal_a_held_flock() -> None:
    import cdc_lock

    with tempfile.TemporaryDirectory() as temp:
        previous = cdc_lock.LOCK_DIR
        cdc_lock.LOCK_DIR = Path(temp)
        try:
            device = str(Path(temp) / "not-a-cdc-node")
            held = exp.acquire_cdc_without_lsof(device, "holder")
            Path(held["path"]).write_text(f"owner=ghost\npid=1\ndevice={device}\n")
            try:
                exp.acquire_cdc_without_lsof(device, "thief")
                raise AssertionError("held flock was stolen")
            except exp.ExperimentError as exc:
                assert "not stolen" in str(exc)
            finally:
                cdc_lock.release(held)
        finally:
            cdc_lock.LOCK_DIR = previous


def test_fixture_opcode_stays_off_the_read_only_allowlist() -> None:
    from titan_transport import CAMPAIGN_OPS, FakeSerial, ObserveDenied, Transport

    assert exp.OP_PAIR_FIXTURE not in exp.EMIT_OFF_READ_OPS
    assert exp.OP_PAIR_FIXTURE not in CAMPAIGN_OPS
    assert exp.session_may_send(exp.OP_PAIR_FIXTURE, b"", pair_fixture=False)
    assert exp.session_may_send(exp.OP_PAIR_FIXTURE, b"\x00" * 8, pair_fixture=True) is None
    begin = proto.pack_config_sub(proto.CFG_BEGIN_SET, b"\x00\x00")
    assert exp.session_may_send(exp.OP_CONFIG, begin, pair_fixture=True)
    transport = Transport(FakeSerial(), allowed={1}, campaign=True)
    try:
        transport.transact(exp.OP_PAIR_FIXTURE, b"\x00", timeout=0.01)
        raise AssertionError("opcode 27 admitted without an explicit gate")
    except ObserveDenied as exc:
        assert "27" in str(exc)
    code = exp.host_admit({"last_generation": 0, "expected_epoch": 1, "in_flight": 0}, b"\x00" * (24 + exp.PAIR_LANE_BYTES))
    assert code == exp.FIXTURE_PARTIAL


class _RecPlayer:
    def __init__(self) -> None:
        self.events: list = []
        self._on = False

    def stop(self) -> None:
        self.events.append("stop")
        self._on = False

    def start(self, path: Path) -> None:
        self.events.append(("start", Path(path).name))
        self._on = True

    def playing(self) -> bool:
        return self._on

    def failed(self) -> bool:
        return False


class _SeqSession(FakeSession):
    def __init__(self, musical_on: set[int], *, freeze_hops: bool = False, freeze_after: int | None = None) -> None:
        super().__init__(dict(IMAGE), [], status())
        self.musical_on = musical_on
        self.palette_n = 0
        self.emitted = 100
        self.hops = 0
        self.freeze_hops = freeze_hops
        self.freeze_after = freeze_after

    def transact(self, op: int, payload: bytes = b"") -> dict:
        self.calls.append(("transact", op, bytes(payload)))
        if op == exp.OP_METRICS:
            if not self.freeze_hops and (self.freeze_after is None or self.hops < self.freeze_after):
                self.hops += 1
            body = metrics(max(self.hops, 1), asrc_consumed=100 + max(self.hops, 1) * 180)
            return {"ok": True, "status": 0, "body": body}
        if op == exp.OP_PALETTE_STATUS:
            self.palette_n += 1
            self.emitted += 7
            body = status(
                emit_enabled=True,
                din_a="P601",
                din_b="P604",
                brightness=24,
                emitted=self.emitted,
                emit_errors=0,
                musical=self.palette_n in self.musical_on,
                pair_fault=0,
                lane_a0_fault=0,
                lane_a1_fault=0,
                submitted_generation=4,
                completed_generation=4,
                pair_completions=2,
                stream_epoch=1,
            )
            return {"ok": True, "status": 0, "body": json.dumps(body).encode()}
        if op == exp.OP_SNAPSHOT:
            return {"ok": False, "status": 1, "body": b""}
        if op == exp.OP_CONFIG:
            return {"ok": True, "status": 0, "body": self.config}
        return {"ok": False, "status": 9, "body": b""}


def _clip(directory: Path, name: str, value: int) -> dict:
    data = struct.pack("<h", value) + bytes(exp.CLIP_SAMPLES * 2 - 2)
    path = directory / name
    path.write_bytes(data)
    return {
        "path": str(path),
        "sha256": hashlib.sha256(data).hexdigest(),
        "samples": exp.CLIP_SAMPLES,
        "permitted": True,
        "identity": name,
    }


def _sequence_packet(directory: Path, **overrides) -> dict:
    clips = [_clip(directory, "song-0.pcm", 11), _clip(directory, "song-1.pcm", 13)]
    resume = _clip(directory, "resume.pcm", 17)
    body = packet(
        expected_profile={"wire_profile": 3, "din_b": "P604", "emit_enabled": True, "brightness": 24},
        stages={
            "emit_off_observe": {"host_runnable": False, "max_duration_s": 30, "samples": 1, "interval_s": 0},
            "group_b": {"quiet_s": 15.0, "music_s": 60.0, "pause_s": 15.0, "resume_s": 30.0},
        },
        sequence={"music_clips": clips, "resume_clip": resume},
    )
    body.update(overrides)
    return body


def test_sequence_on_the_old_pin_does_not_open() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = packet(expected_profile={"din_b": "P603", "emit_enabled": True})
        path = write_packet(root, body)
        opened = {"n": 0}

        def open_session():
            opened["n"] += 1
            raise AssertionError("device opened")

        result = exp.dispatch("sequence", path, root / "seq", open_session, player=_RecPlayer(), sleeper=lambda _s: None)
        assert opened["n"] == 0
        assert result["device_opened"] is False
        assert any("P604" in item for item in result["blockers"])


def test_silence_clip_does_not_open_the_sequence() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        silent = bytes(exp.CLIP_SAMPLES * 2)
        path_pcm = root / "silent.pcm"
        path_pcm.write_bytes(silent)
        clip = {
            "path": str(path_pcm),
            "sha256": hashlib.sha256(silent).hexdigest(),
            "samples": exp.CLIP_SAMPLES,
            "permitted": True,
            "identity": "silence",
        }
        body = _sequence_packet(root)
        body["sequence"] = {"music_clips": [clip, clip], "resume_clip": clip}
        opened = {"n": 0}

        def open_session():
            opened["n"] += 1
            raise AssertionError("device opened")

        result = exp.dispatch("sequence", write_packet(root, body), root / "seq", open_session, player=_RecPlayer(), sleeper=lambda _s: None)
        assert opened["n"] == 0
        assert any("not named music" in item for item in result["blockers"])


def test_short_stability_playlist_does_not_open_or_loop() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = packet(stability_playlist={"seconds": 546.598, "loop": False})
        result = exp.dispatch("stability", write_packet(root, body), root / "stab")
        assert result["device_opened"] is False
        assert result["looped"] is False
        text = " ".join(result["blockers"])
        assert "53.402" in text
        assert "must not be looped" in text
        looped = packet(stability_playlist={"seconds": 600, "loop": True})
        again = exp.dispatch("stability", write_packet(root, looped), root / "stab2")
        assert again["device_opened"] is False
        assert any("must not loop" in item for item in again["blockers"])


def test_sequence_plays_three_clips_and_does_not_enable_emit() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = _sequence_packet(root)
        names = [Path(item["path"]).name for item in body["sequence"]["music_clips"]]
        names.append(Path(body["sequence"]["resume_clip"]["path"]).name)
        player = _RecPlayer()
        # Palette reads: 1 initial, 2-3 quiet, 4-5 music, then pause and resume.
        session = _SeqSession({4, 5})
        result = exp.dispatch(
            "sequence",
            write_packet(root, body),
            root / "seq",
            lambda: session,
            player=player,
            sleeper=lambda _s: None,
        )
        assert result["verdict"] == "PASS"
        assert result["functional_verdict"] == "FAIL"
        assert result["functional_reason"] == "resume did not wake"
        assert result["emit_enabled_by_this_run"] is False
        starts = [event[1] for event in player.events if isinstance(event, tuple)]
        assert starts == names
        assert exp.OP_PAIR_FIXTURE not in [call[1] for call in session.calls if call[0] == "transact"]
        assert_no_writes(session)


def test_sequence_does_not_pass_when_pause_never_goes_quiet() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = _sequence_packet(root)
        # Palette 7 is the end of the pause. It must not stay musical.
        session = _SeqSession({4, 5, 7, 8, 9})
        result = exp.dispatch(
            "sequence", write_packet(root, body), root / "seq",
            lambda: session, player=_RecPlayer(), sleeper=lambda _s: None,
        )
        assert result["verdict"] == "PASS"
        assert result["functional_verdict"] == "FAIL"
        assert result["functional_reason"] == "pause did not become quiet"


def test_sequence_does_not_pass_when_resume_never_wakes() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = _sequence_packet(root)
        session = _SeqSession({4, 5})
        result = exp.dispatch(
            "sequence", write_packet(root, body), root / "seq",
            lambda: session, player=_RecPlayer(), sleeper=lambda _s: None,
        )
        assert result["functional_verdict"] == "FAIL"
        assert result["functional_reason"] == "resume did not wake"


class _EarlyPlayer:
    def __init__(self) -> None:
        self.armed = False
        self.polls = 0

    def start(self, path: Path) -> None:
        self.armed = True
        self.polls = 0

    def playing(self) -> bool:
        if not self.armed:
            return False
        self.polls += 1
        return self.polls <= 2

    def failed(self) -> bool:
        return False

    def stop(self) -> None:
        self.armed = False
        self.polls = 0


def test_sequence_faults_when_playback_ends_early() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = _sequence_packet(root)
        result = exp.dispatch(
            "sequence", write_packet(root, body), root / "seq",
            lambda: _SeqSession({4, 5, 8, 9}), player=_EarlyPlayer(), sleeper=lambda _s: None,
        )
        assert result["verdict"] == "FAULT"
        assert "declared duration" in result["reason"]


class _ControlSession(_SeqSession):
    def __init__(self, musical_on: set[int]) -> None:
        super().__init__(musical_on)
        self.cfg = {
            "revision": 4,
            "emit_on": 1,
            "brightness": 24,
            "flags": 5,
            "palette_a": 33,
            "palette_b": 43,
            "mode_a": 32,
            "mode_b": 32,
        }
        self.staged = b""
        self._parts: dict[int, bytes] = {}

    def transact(self, op: int, payload: bytes = b"") -> dict:
        self.calls.append(("transact", op, bytes(payload)))
        if op == exp.OP_METRICS:
            if not self.freeze_hops and (self.freeze_after is None or self.hops < self.freeze_after):
                self.hops += 1
            body = metrics(max(self.hops, 1), asrc_consumed=100 + max(self.hops, 1) * 180)
            return {"ok": True, "status": 0, "body": body}
        if op == exp.OP_PALETTE_STATUS:
            self.palette_n += 1
            self.emitted += 7
            body = status(
                emit_enabled=True, din_a="P601", din_b="P604", brightness=24,
                emitted=self.emitted, emit_errors=0, musical=self.palette_n in self.musical_on,
                pair_fault=0, lane_a0_fault=0, lane_a1_fault=0,
                submitted_generation=4, completed_generation=4, pair_completions=2,
                stream_epoch=1,
            )
            return {"ok": True, "status": 0, "body": json.dumps(body).encode()}
        if op != exp.OP_CONFIG:
            return {"ok": False, "status": 9, "body": b""}
        sub = exp.config_sub(payload)
        if sub == proto.CFG_GET_CONFIG:
            return {"ok": True, "status": 0, "body": proto.pack_config(self.cfg)}
        if sub == proto.CFG_BEGIN_SET:
            self._parts = {}
            return {"ok": True, "status": 0, "body": b""}
        if sub == proto.CFG_APPEND_SET:
            rest = payload[4:]
            offset = struct.unpack_from("<I", rest)[0]
            self._parts[offset] = rest[4:]
            self.staged = b"".join(self._parts[key] for key in sorted(self._parts))
            return {"ok": True, "status": 0, "body": b""}
        if sub == proto.CFG_COMMIT_SET:
            parsed = proto.unpack_config(self.staged)
            revision = int(self.cfg["revision"]) + 1
            for key in ("palette_a", "palette_b", "mode_a", "mode_b", "emit_on", "brightness", "flags"):
                self.cfg[key] = parsed[key]
            self.cfg["revision"] = revision
            return {"ok": True, "status": 0, "body": b""}
        return {"ok": False, "status": 9, "body": b""}


def test_functional_applies_a_palette_change_without_clearing_emit() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = _sequence_packet(root)
        body["controls"] = [{"palette_a": 34}]
        body["expected_revision"] = 4
        session = _ControlSession({4, 5, 8, 9})
        clock = _Clock()
        result = exp.dispatch(
            "functional", write_packet(root, body), root / "seq",
            lambda: session, player=_RecPlayer(), sleeper=clock.advance, clock=clock,
        )
        assert result["verdict"] == "PASS"
        assert result["functional_verdict"] == "PASS"
        assert result["health"]["verdict"] == "PASS"
        assert result["controls_applied"]["applied"] == [{"palette_a": 34}]
        assert result["controls_applied"]["revision"] == 5
        assert 15.0 < result["controls_applied"]["elapsed_s"] < 75.0
        music = result["stage_bounds"]["music"]
        assert music["duration_s"] == 60.0
        assert music["start_s"] == 15.0
        assert music["end_s"] == 75.0
        assert result["stage_bounds"]["quiet"]["duration_s"] == 15.0
        assert music["start_s"] < result["controls_applied"]["elapsed_s"] < music["end_s"]
        commit_elapsed = result["controls_applied"]["commit_monotonic_s"]
        readback_elapsed = result["controls_applied"]["readback_monotonic_s"]
        assert music["start_s"] < commit_elapsed < music["end_s"]
        assert music["start_s"] < readback_elapsed < music["end_s"]
        staged = proto.unpack_config(session.staged)
        assert staged["emit_on"] == 1
        assert staged["brightness"] == 24
        assert exp.OP_PAIR_FIXTURE not in [call[1] for call in session.calls if call[0] == "transact"]


def test_control_that_clears_emit_is_refused_before_a_write() -> None:
    current = proto.unpack_config(proto.pack_config({"revision": 2, "emit_on": 1, "brightness": 24}))
    try:
        exp.show_control_blob(current, {"palette_a": 1, "emit_on": 0})
    except exp.ExperimentError as exc:
        assert "clear emit" in exc.reason
    else:
        raise AssertionError("emit-off control was accepted")


def test_p604_mapping_does_not_run_while_the_show_is_emitting() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        lane0, lane1 = _lanes(root)
        body = packet(
            fixture_injection={"supported": True},
            output_permission="AUTHORISED_THIS_PAIR_PACKET",
            wiring={"din_b_on_p604": True, "din_b_on_p603": False, "break_80_81_verified": True},
            current_limit={"milliamps": 80, "meter": True, "class": "bench-measured"},
            expected_profile={
                "wire_profile": 3,
                "configured_backend": "ws2816_gpt_pair",
                "din_b": "P604",
                "emit_enabled": True,
            },
            mapping={"fixtures": [{"lane0": str(lane0), "lane1": str(lane1), "generation": 1}]},
            stages={
                "emit_off_observe": {"host_runnable": True, "max_duration_s": 30, "samples": 2, "interval_s": 0},
                "physical_mapping": {"max_duration_s": 30, "interval_s": 0},
            },
        )
        session = FakeSession(
            dict(IMAGE),
            [metrics(1), metrics(2)],
            status(emit_enabled=True, din_b="P604", pending_replacements=3218),
        )
        result = exp.dispatch("physical", write_packet(root, body), root / "phys", lambda: session)
        assert result["verdict"] == "DENIED"
        assert "emit-off" in result["reason"]
        assert exp.OP_PAIR_FIXTURE not in [call[1] for call in session.calls if call[0] == "transact"]


def test_naming_both_din_pins_does_not_open() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = packet(
            fixture_injection={"supported": True},
            output_permission="AUTHORISED_THIS_PAIR_PACKET",
            wiring={"din_b_on_p603": True, "din_b_on_p604": True, "break_80_81_verified": True},
            current_limit={"milliamps": 80, "meter": True, "class": "bench-measured"},
        )
        opened = {"n": 0}

        def open_session():
            opened["n"] += 1
            raise AssertionError("device opened")

        result = exp.dispatch("physical", write_packet(root, body), root / "phys", open_session)
        assert opened["n"] == 0
        assert any("both P603 and P604" in item for item in result["blockers"])


def test_stability_executor_acquires_a_600s_playlist() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        samples = 600 * exp.RATE_HZ
        data = struct.pack("<h", 9) + bytes(samples * 2 - 2)
        path = root / "full.pcm"
        path.write_bytes(data)
        digest = hashlib.sha256(data).hexdigest()
        body = packet(
            expected_profile={
                "wire_profile": 3,
                "configured_backend": "ws2816_gpt_pair",
                "din_b": "P604",
                "emit_enabled": True,
                "brightness": 24,
            },
            stability_playlist={
                "seconds": 600.0,
                "loop": False,
                "tracks": [{
                    "path": str(path),
                    "sha256": digest,
                    "samples": samples,
                    "permitted": True,
                    "identity": "full-recording",
                }],
            },
        )
        bare = packet(stability_playlist={"seconds": 600, "loop": False})
        refused = exp.dispatch("stability", write_packet(root, bare), root / "preflight")
        assert refused["verdict"] == "BLOCKED"
        assert refused["device_opened"] is False
        assert refused["completed"] is False
        assert any("not declared" in item for item in refused["blockers"])
        player = _RecPlayer()
        session = _SeqSession({1})
        clock = _Clock()

        def sleeper(seconds: float) -> None:
            clock.advance(seconds)

        result = exp.dispatch(
            "stability", write_packet(root, body), root / "run",
            lambda: session, player=player, sleeper=sleeper, clock=clock,
        )
        assert result["verdict"] == "PASS"
        assert result["scope"] == "stability_run"
        assert result["completed"] is True
        assert result["device_opened"] is True
        assert result["samples"] >= 2
        assert result["duration_s"] == 600.0
        assert player.events[0] == ("start", "full.pcm")
        assert result["asrc_consumed_delta"] != exp.UNKNOWN
        assert result["pair_epoch_drops_delta"] == 0
        assert result["health"]["verdict"] == "PASS"


def test_stability_frozen_hops_fail() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        samples = 600 * exp.RATE_HZ
        data = struct.pack("<h", 9) + bytes(samples * 2 - 2)
        path = root / "full.pcm"
        path.write_bytes(data)
        body = packet(
            expected_profile={
                "wire_profile": 3,
                "configured_backend": "ws2816_gpt_pair",
                "din_b": "P604",
                "emit_enabled": True,
                "brightness": 24,
            },
            stability_playlist={
                "seconds": 600.0,
                "loop": False,
                "tracks": [{
                    "path": str(path),
                    "sha256": hashlib.sha256(data).hexdigest(),
                    "samples": samples,
                    "permitted": True,
                    "identity": "full-recording",
                }],
            },
        )
        clock = _Clock()
        result = exp.dispatch(
            "stability", write_packet(root, body), root / "run",
            lambda: _SeqSession({1}, freeze_hops=True), player=_RecPlayer(),
            sleeper=clock.advance, clock=clock,
        )
        assert result["verdict"] == "FAIL"
        assert result["completed"] is False
        assert "ap_hops" in result["reason"]


def test_stability_missing_pair_fault_fails() -> None:
    class MissingFault(_SeqSession):
        def transact(self, op: int, payload: bytes = b"") -> dict:
            got = super().transact(op, payload)
            if op == exp.OP_PALETTE_STATUS:
                body = json.loads(got["body"].decode())
                del body["pair_fault"]
                got = {"ok": True, "status": 0, "body": json.dumps(body).encode()}
            return got

    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        samples = 600 * exp.RATE_HZ
        data = struct.pack("<h", 9) + bytes(samples * 2 - 2)
        path = root / "full.pcm"
        path.write_bytes(data)
        body = packet(
            expected_profile={
                "wire_profile": 3, "configured_backend": "ws2816_gpt_pair",
                "din_b": "P604", "emit_enabled": True, "brightness": 24,
            },
            stability_playlist={
                "seconds": 600.0, "loop": False,
                "tracks": [{
                    "path": str(path), "sha256": hashlib.sha256(data).hexdigest(),
                    "samples": samples, "permitted": True, "identity": "full-recording",
                }],
            },
        )
        clock = _Clock()
        result = exp.dispatch(
            "stability", write_packet(root, body), root / "run",
            lambda: MissingFault({1}), player=_RecPlayer(),
            sleeper=clock.advance, clock=clock,
        )
        assert result["verdict"] == "FAIL"
        assert "pair_fault missing" in result["reason"]
        assert result["completed"] is False


def test_stability_does_not_pass_when_elapsed_time_does_not_move() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        samples = 600 * exp.RATE_HZ
        data = struct.pack("<h", 9) + bytes(samples * 2 - 2)
        path = root / "full.pcm"
        path.write_bytes(data)
        body = packet(
            expected_profile={
                "wire_profile": 3, "configured_backend": "ws2816_gpt_pair",
                "din_b": "P604", "emit_enabled": True, "brightness": 24,
            },
            stability_playlist={
                "seconds": 600.0, "loop": False,
                "tracks": [{
                    "path": str(path), "sha256": hashlib.sha256(data).hexdigest(),
                    "samples": samples, "permitted": True, "identity": "full-recording",
                }],
            },
        )
        clock = _Clock()
        result = exp.dispatch(
            "stability", write_packet(root, body), root / "run",
            lambda: _SeqSession({1}), player=_RecPlayer(),
            sleeper=lambda _s: None, clock=clock,
        )
        assert result["verdict"] == "FAIL"
        assert result["completed"] is False
        assert result["duration_s"] < 600
        assert "spaced" in result["reason"] or "elapsed" in result["reason"]


def test_functional_stale_revision_does_not_write() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = _sequence_packet(root)
        body["controls"] = [{"palette_a": 34}]
        body["expected_revision"] = 999
        session = _ControlSession({4, 5, 8, 9})
        clock = _Clock()
        result = exp.dispatch(
            "functional", write_packet(root, body), root / "seq",
            lambda: session, player=_RecPlayer(), sleeper=clock.advance, clock=clock,
        )
        assert result["verdict"] == "DENIED"
        assert result["reason"] == "stale config revision"
        assert not any(
            call[0] == "transact" and call[1] == exp.OP_CONFIG
            and exp.config_sub(call[2]) == proto.CFG_BEGIN_SET
            for call in session.calls
        )


def test_functional_missing_controls_does_not_open() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = _sequence_packet(root)
        opened = {"n": 0}

        def open_session():
            opened["n"] += 1
            raise AssertionError("device opened")

        result = exp.dispatch(
            "functional", write_packet(root, body), root / "seq",
            open_session, player=_RecPlayer(), sleeper=lambda _s: None,
        )
        assert opened["n"] == 0
        assert result["device_opened"] is False
        assert result["verdict"] == "BLOCKED"
        assert any("controls" in item for item in result["blockers"])


def test_functional_frozen_hops_fail() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = _sequence_packet(root)
        body["controls"] = [{"palette_a": 34}]
        body["expected_revision"] = 4
        session = _ControlSession({4, 5, 8, 9})
        session.freeze_hops = True
        clock = _Clock()
        result = exp.dispatch(
            "functional", write_packet(root, body), root / "seq",
            lambda: session, player=_RecPlayer(), sleeper=clock.advance, clock=clock,
        )
        assert result["verdict"] == "FAIL"
        assert result["functional_verdict"] == "FAIL"
        assert "ap_hops" in result["reason"]


def test_pending_replacements_are_not_in_flight() -> None:
    busy = status(submitted_generation=9, completed_generation=8, pending_replacements=3218)
    idle = status(submitted_generation=9, completed_generation=9, pending_replacements=3218)
    assert exp._pair_in_flight(busy) is True
    assert exp._pair_in_flight(idle) is False


def test_mapping_restore_on_success_and_not_on_fault() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = _mapping_packet(root, mapping_lifecycle={"restore": True, "fixture_dwell_s": 0, "completion_timeout_s": 2})
        session = _MapSession()
        result = exp.dispatch("physical", write_packet(root, body), root / "phys", lambda: session)
        assert result["verdict"] == "SEQUENCED"
        assert result["restore_applied"] is True
        assert any(
            call[0] == "transact" and call[1] == exp.OP_CONFIG
            and exp.config_sub(call[2]) == proto.CFG_COMMIT_SET
            for call in session.calls
        )
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)

        class Faulty(_MapSession):
            def transact(self, op: int, payload: bytes = b"") -> dict:
                if op == exp.OP_PAIR_FIXTURE:
                    self.calls.append(("transact", op, bytes(payload)))
                    generation = int.from_bytes(payload[8:12], "little")
                    self.palette = status(
                        submitted_generation=generation, completed_generation=generation,
                        pair_completions=1, lane_a0_fault=1, lane_a1_fault=0, pair_fault=1,
                        stream_epoch=1,
                    )
                    return {"ok": True, "status": 0, "body": json.dumps({"accepted": True, "generation": generation}).encode()}
                return super().transact(op, payload)

        body = _mapping_packet(root, mapping_lifecycle={"restore": True})
        session = Faulty()
        result = exp.dispatch("physical", write_packet(root, body), root / "fault", lambda: session)
        assert result["verdict"] == "FAULT"
        assert result.get("restore_applied") is not True
        assert not any(
            call[0] == "transact" and call[1] == exp.OP_CONFIG
            and exp.config_sub(call[2]) == proto.CFG_COMMIT_SET
            for call in session.calls
        )


def _stability_body(root: Path) -> dict:
    samples = 600 * exp.RATE_HZ
    data = struct.pack("<h", 9) + bytes(samples * 2 - 2)
    path = root / "full.pcm"
    path.write_bytes(data)
    return packet(
        expected_profile={
            "wire_profile": 3,
            "configured_backend": "ws2816_gpt_pair",
            "din_b": "P604",
            "emit_enabled": True,
            "brightness": 24,
        },
        stability_playlist={
            "seconds": 600.0,
            "loop": False,
            "tracks": [{
                "path": str(path),
                "sha256": hashlib.sha256(data).hexdigest(),
                "samples": samples,
                "permitted": True,
                "identity": "full-recording",
            }],
        },
    )


def test_stability_rising_losses_fail() -> None:
    class Rising(_SeqSession):
        def transact(self, op: int, payload: bytes = b"") -> dict:
            got = super().transact(op, payload)
            if op == exp.OP_METRICS:
                body = json.loads(got["body"].decode())
                body["pdm_target"]["asrc_discarded"] = self.hops
                got = {"ok": True, "status": 0, "body": json.dumps(body).encode()}
            return got

    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        clock = _Clock()
        result = exp.dispatch(
            "stability", write_packet(root, _stability_body(root)), root / "run",
            lambda: Rising({1}), player=_RecPlayer(), sleeper=clock.advance, clock=clock,
        )
        assert result["verdict"] == "FAIL"
        assert result["completed"] is False
        assert "asrc_discarded" in result["reason"]


def test_stability_progress_then_freeze_fails() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        clock = _Clock()
        result = exp.dispatch(
            "stability", write_packet(root, _stability_body(root)), root / "run",
            lambda: _SeqSession({1}, freeze_after=3), player=_RecPlayer(),
            sleeper=clock.advance, clock=clock,
        )
        assert result["verdict"] == "FAIL"
        assert result["completed"] is False
        assert result["reason"] == "progress then freeze"


def test_functional_stationary_clock_fails() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = _sequence_packet(root)
        body["controls"] = [{"palette_a": 34}]
        body["expected_revision"] = 4
        clock = _Clock()
        result = exp.dispatch(
            "functional", write_packet(root, body), root / "seq",
            lambda: _ControlSession({4, 5, 8, 9}), player=_RecPlayer(),
            sleeper=lambda _s: None, clock=clock,
        )
        assert result["verdict"] == "FAULT"
        assert "monotonic clock" in result["reason"]


def test_functional_noop_control_is_refused() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = _sequence_packet(root)
        body["controls"] = [{"palette_a": 33}]
        body["expected_revision"] = 4
        clock = _Clock()
        session = _ControlSession({4, 5, 8, 9})
        result = exp.dispatch(
            "functional", write_packet(root, body), root / "seq",
            lambda: session, player=_RecPlayer(), sleeper=clock.advance, clock=clock,
        )
        assert result["verdict"] == "DENIED"
        assert "does not change" in result["reason"]
        assert not any(
            call[0] == "transact" and call[1] == exp.OP_CONFIG
            and exp.config_sub(call[2]) == proto.CFG_BEGIN_SET
            for call in session.calls
        )


def test_emit_errors_must_be_zero() -> None:
    rows = [
        {"t": 0.0, "counters": {"stream_epoch": 1, "pair_epoch_drops": 0, "asrc_consumed": 100, "ap_hops": 4, "asrc_discarded": 0, "push_rejected": 0, "stale_discards": 0},
         "palette": {"emitted": 10, "emit_errors": 5, "pair_fault": 0, "lane_a0_fault": 0, "lane_a1_fault": 0, "din_b": "P604", "submitted_generation": 4, "completed_generation": 4, "pair_completions": 2}},
        {"t": 10.0, "counters": {"stream_epoch": 1, "pair_epoch_drops": 0, "asrc_consumed": 280, "ap_hops": 8, "asrc_discarded": 0, "push_rejected": 0, "stale_discards": 0},
         "palette": {"emitted": 20, "emit_errors": 5, "pair_fault": 0, "lane_a0_fault": 0, "lane_a1_fault": 0, "din_b": "P604", "submitted_generation": 4, "completed_generation": 4, "pair_completions": 2}},
    ]
    result = exp.evaluate_run_health(rows)
    assert result["verdict"] == "FAIL"
    assert "emit errors not zero" in result["reason"]


def test_sparse_fixtures_match_pack_order_and_centre_pixels() -> None:
    assert exp.pack_grb48(0x12AB, 0x34CD, 0x56EF) == bytes.fromhex("34cd12ab56ef")
    centre_a, centre_b = exp.sparse_pair_lanes(a={79: (0x7A3C, 0, 0)}, b={0: (0, 0, 0x7A3C)})
    assert len(centre_a) == exp.PAIR_LANE_BYTES
    assert centre_a[79 * 6:80 * 6] == bytes.fromhex("00007a3c0000")
    assert centre_b[0:6] == bytes.fromhex("000000007a3c")
    assert not any(centre_a[:79 * 6]) and not any(centre_b[6:])
    edge_a, edge_b = exp.sparse_pair_lanes(a={0: (0, 0x7A3C, 0)}, b={79: (0, 0x7A3C, 0)})
    assert edge_a[0:6] == bytes.fromhex("7a3c00000000")
    assert edge_b[79 * 6:80 * 6] == bytes.fromhex("7a3c00000000")


def _hold_packet(root: Path, **lifecycle) -> dict:
    body = _mapping_packet(root)
    body["expected_profile"] = {
        "wire_profile": 3,
        "configured_backend": "ws2816_gpt_pair",
        "din_a": "P601",
        "din_b": "P604",
        "emit_enabled": True,
        "brightness": 24,
    }
    body["expected_revision"] = 4
    life = {"hold": exp.MAPPING_HOLD, "restore": True, "completion_timeout_s": 2.0, "fixture_dwell_s": 0}
    life.update(lifecycle)
    body["mapping_lifecycle"] = life
    body["wiring"] = {"din_b_on_p604": True, "din_b_on_p603": False, "break_80_81_verified": True}
    body["mapping"]["fixtures"][0]["generation"] = 5
    body["mapping_lifecycle"].setdefault("blackout_budget_s", 12.0)
    body["mapping_lifecycle"].setdefault("restore_reserve_s", 3.0)
    return body


def test_wrong_profile_before_hold_leaves_show_untouched() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = _hold_packet(root)
        body["expected_profile"]["wire_profile"] = 4
        session = _ShowMapSession(emit_on=1)
        result = exp.dispatch("physical", write_packet(root, body), root / "phys", lambda: session)
        assert result["verdict"] == "DENIED"
        assert "wire_profile" in result["reason"]
        assert_no_writes(session)


def test_stale_generation_before_hold_leaves_show_untouched() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = _hold_packet(root)
        body["mapping"]["fixtures"][0]["generation"] = 1
        session = _ShowMapSession(emit_on=1)
        result = exp.dispatch("physical", write_packet(root, body), root / "phys", lambda: session)
        assert result["verdict"] == "DENIED"
        assert result["reason"] == f"fixture rejected before transmit ({exp.FIXTURE_STALE})"
        assert result["detail"]["generation"] == 1
        assert_no_writes(session)


def test_frozen_led_restoration_fails_even_if_audio_advances() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        session = _ShowMapSession(emit_on=1, freeze_led=True)
        result = exp.dispatch("physical", write_packet(root, _hold_packet(root)), root / "phys", lambda: session)
        assert result["verdict"] == "FAULT"
        assert "LED" in result["reason"]
        assert result["restore_applied"] is True
        assert result["restore_verified"] is False
        assert result.get("ap_hops_delta") not in (0, 0.0, None, exp.UNKNOWN)


def test_hold_budget_overrun_on_slow_transaction() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        clock = _Clock()

        class Slow(_ShowMapSession):
            def transact(self, op: int, payload: bytes = b"") -> dict:
                clock.advance(5.0)
                return super().transact(op, payload)

        body = _hold_packet(root, blackout_budget_s=12.0, restore_reserve_s=4.0)
        body["stages"]["physical_mapping"] = {"max_duration_s": 120, "interval_s": 0}
        session = Slow(emit_on=1)
        result = exp.dispatch(
            "physical", write_packet(root, body), root / "phys",
            lambda: session, sleeper=clock.advance, clock=clock,
        )
        assert result["verdict"] == "FAULT"
        assert "budget" in result["reason"]
        config_subs = [
            exp.config_sub(call[2])
            for call in session.calls
            if call[0] == "transact" and call[1] == exp.OP_CONFIG
        ]
        assert proto.CFG_BEGIN_SET in config_subs
        assert proto.CFG_APPEND_SET in config_subs
        assert proto.CFG_COMMIT_SET not in config_subs
        assert not any(call[0] == "transact" and call[1] == exp.OP_PAIR_FIXTURE for call in session.calls)


def test_host_test_current_limit_does_not_admit_live_mapping() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = _mapping_packet(root)
        body["current_limit"] = {"milliamps": 80, "meter": False, "class": "host-test"}
        opened = {"n": 0}

        def open_session():
            opened["n"] += 1
            raise AssertionError("device opened")

        result = exp.dispatch("physical", write_packet(root, body), root / "phys", open_session)
        assert result["verdict"] == "BLOCKED"
        assert opened["n"] == 0
        assert any("bench measurement" in item for item in result["blockers"])
        assert not any("AUTHORISED_THIS_PAIR_PACKET" in item for item in result["blockers"])


def test_allocation_uses_post_hold_generation() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = _hold_packet(root)
        body["mapping"]["fixtures"][0]["generation"] = 0
        session = _ShowMapSession(emit_on=1, advance_submit=True)
        result = exp.dispatch("physical", write_packet(root, body), root / "phys", lambda: session)
        assert result["verdict"] == "SEQUENCED"
        allocated = result["generation_allocation"]
        preflight = json.loads((root / "phys" / "preflight-observation.json").read_text())
        assert allocated[0]["generation"] > preflight["submitted_generation"]


def test_outstanding_work_drains_after_hold_then_allocates() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = _hold_packet(root)
        body["mapping"]["fixtures"][0]["generation"] = 0
        session = _ShowMapSession(emit_on=1)
        session.palette["submitted_generation"] = 8
        session.palette["completed_generation"] = 6
        result = exp.dispatch("physical", write_packet(root, body), root / "phys", lambda: session)
        assert result["verdict"] == "SEQUENCED"
        preflight = json.loads((root / "phys" / "preflight-observation.json").read_text())
        final = json.loads((root / "phys" / "generation-allocation.json").read_text())
        assert preflight["in_flight"] is True
        assert final["final_last_generation"] >= 8
        assert final["allocated"][0]["generation"] == final["final_last_generation"] + 1


def test_functional_nonzero_clock_origin() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = _sequence_packet(root)
        body["controls"] = [{"palette_a": 34}]
        body["expected_revision"] = 4
        clock = _Clock()
        clock.now = 1000.0
        session = _ControlSession({4, 5, 8, 9})
        result = exp.dispatch(
            "functional", write_packet(root, body), root / "seq",
            lambda: session, player=_RecPlayer(), sleeper=clock.advance, clock=clock,
        )
        assert result["verdict"] == "PASS"
        assert result["stage_bounds"]["quiet"]["start_s"] == 0.0
        assert result["stage_bounds"]["quiet"]["duration_s"] == 15.0
        assert result["stage_bounds"]["music"]["start_s"] == 15.0
        assert result["stage_bounds"]["music"]["duration_s"] == 60.0
        assert result["stage_bounds"]["pause"]["duration_s"] == 15.0
        assert result["stage_bounds"]["resume"]["duration_s"] == 30.0
        music = result["stage_bounds"]["music"]
        assert music["start_s"] < result["controls_applied"]["elapsed_s"] < music["end_s"]
        assert 1000.0 < result["controls_applied"]["commit_monotonic_s"] < 1075.0


def test_mapping_delayed_completion_passes() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        session = _ShowMapSession(delay=3)
        result = exp.dispatch("physical", write_packet(root, _hold_packet(root)), root / "phys", lambda: session)
        assert result["verdict"] == "SEQUENCED"
        assert result["restore_verified"] is True
        assert result["physical_observation"] is False


def test_mapping_partial_lane_does_not_complete() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        result = exp.dispatch(
            "physical", write_packet(root, _hold_packet(root)), root / "phys",
            lambda: _ShowMapSession(partial=True),
        )
        assert result["verdict"] == "FAULT"
        assert "partial" in result["reason"]
        assert result["restore_attempted"] is False


def test_mapping_wrong_epoch_does_not_complete() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        result = exp.dispatch(
            "physical", write_packet(root, _hold_packet(root)), root / "phys",
            lambda: _ShowMapSession(wrong_epoch=True),
        )
        assert result["verdict"] == "FAULT"
        assert "epoch" in result["reason"]
        assert result["restore_attempted"] is False


def test_mapping_missing_fault_is_not_healthy() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        result = exp.dispatch(
            "physical", write_packet(root, _hold_packet(root)), root / "phys",
            lambda: _ShowMapSession(omit_fault=True),
        )
        assert result["verdict"] == "FAULT"
        assert "missing" in result["reason"]
        assert result["restore_attempted"] is False


def test_mapping_black_pair_fault_does_not_restore() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        session = _ShowMapSession(black_fault=True)
        result = exp.dispatch("physical", write_packet(root, _hold_packet(root)), root / "phys", lambda: session)
        assert result["verdict"] == "FAULT"
        assert result["restore_attempted"] is False
        assert not any(
            call[0] == "transact" and call[1] == exp.OP_CONFIG
            and exp.config_sub(call[2]) == proto.CFG_COMMIT_SET
            and proto.unpack_config(session.staged).get("emit_on") == 1
            for call in session.calls
            if call[0] == "transact" and call[1] == exp.OP_CONFIG and exp.config_sub(call[2]) == proto.CFG_COMMIT_SET
        )


def test_mapping_restore_readback_failure() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        result = exp.dispatch(
            "physical", write_packet(root, _hold_packet(root)), root / "phys",
            lambda: _ShowMapSession(restore_mismatch=True),
        )
        assert result["verdict"] == "FAULT"
        assert "readback" in result["reason"]
        assert result["restore_applied"] is True
        assert result["restore_verified"] is False


def test_mapping_emit_on_restoration_succeeds() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        session = _ShowMapSession(emit_on=1)
        result = exp.dispatch("physical", write_packet(root, _hold_packet(root)), root / "phys", lambda: session)
        assert result["verdict"] == "SEQUENCED"
        assert result["restore_verified"] is True
        assert result["physical_observation"] is False
        assert session.cfg["emit_on"] == 1
        assert session.cfg["palette_a"] == 33
        assert session.cfg["brightness"] == 24


def test_sequence_does_not_pass_when_the_board_never_marks_music() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        body = _sequence_packet(root)
        player = _RecPlayer()
        session = _SeqSession(set())
        result = exp.dispatch(
            "sequence",
            write_packet(root, body),
            root / "seq",
            lambda: session,
            player=player,
            sleeper=lambda _s: None,
        )
        assert result["verdict"] == "FAIL"
        assert "musical" in result["reason"]
        assert result["emit_enabled_by_this_run"] is False


def main() -> int:
    test_wrong_build_denies_before_write()
    test_missing_permission_and_wiring_do_not_open()
    test_permission_without_injection_still_does_not_open()
    test_cleared_external_gates_run_mapping_and_black_stop_without_enabling_emit()
    test_stale_revision_denies_before_commit()
    test_missing_metric_stays_unknown_and_zero_stays_zero()
    test_truncated_and_failed_replies_record_raw_and_do_not_write()
    test_recorder_failure_and_interrupt_release_the_lease()
    test_dry_run_does_not_open_and_names_the_forbidden_script()
    test_partial_pair_is_not_complete_and_units_differ()
    test_silence_is_not_named_music()
    test_begin_set_is_a_control_write()
    test_empty_palette_does_not_pass_on_counter_movement()
    test_framing_error_is_a_saved_fault()
    test_deadline_includes_transaction_time()
    test_zero_generations_are_not_a_completed_pair()
    test_malformed_and_partial_fixtures_do_not_open()
    test_stale_generation_is_rejected_before_the_fixture_opcode()
    test_lane_fault_stops_before_the_black_pair()
    test_physical_interrupt_and_recorder_failure_stay_faults()
    test_serial_open_failure_releases_the_handle()
    test_dead_pid_text_does_not_steal_a_held_flock()
    test_fixture_opcode_stays_off_the_read_only_allowlist()
    test_sequence_on_the_old_pin_does_not_open()
    test_silence_clip_does_not_open_the_sequence()
    test_short_stability_playlist_does_not_open_or_loop()
    test_sequence_plays_three_clips_and_does_not_enable_emit()
    test_sequence_does_not_pass_when_pause_never_goes_quiet()
    test_sequence_does_not_pass_when_resume_never_wakes()
    test_sequence_faults_when_playback_ends_early()
    test_functional_applies_a_palette_change_without_clearing_emit()
    test_control_that_clears_emit_is_refused_before_a_write()
    test_p604_mapping_does_not_run_while_the_show_is_emitting()
    test_naming_both_din_pins_does_not_open()
    test_stability_executor_acquires_a_600s_playlist()
    test_stability_frozen_hops_fail()
    test_stability_missing_pair_fault_fails()
    test_stability_does_not_pass_when_elapsed_time_does_not_move()
    test_functional_stale_revision_does_not_write()
    test_functional_missing_controls_does_not_open()
    test_functional_frozen_hops_fail()
    test_pending_replacements_are_not_in_flight()
    test_mapping_restore_on_success_and_not_on_fault()
    test_stability_rising_losses_fail()
    test_stability_progress_then_freeze_fails()
    test_functional_stationary_clock_fails()
    test_functional_noop_control_is_refused()
    test_emit_errors_must_be_zero()
    test_sparse_fixtures_match_pack_order_and_centre_pixels()
    test_wrong_profile_before_hold_leaves_show_untouched()
    test_stale_generation_before_hold_leaves_show_untouched()
    test_frozen_led_restoration_fails_even_if_audio_advances()
    test_hold_budget_overrun_on_slow_transaction()
    test_host_test_current_limit_does_not_admit_live_mapping()
    test_allocation_uses_post_hold_generation()
    test_outstanding_work_drains_after_hold_then_allocates()
    test_functional_nonzero_clock_origin()
    test_mapping_delayed_completion_passes()
    test_mapping_partial_lane_does_not_complete()
    test_mapping_wrong_epoch_does_not_complete()
    test_mapping_missing_fault_is_not_healthy()
    test_mapping_black_pair_fault_does_not_restore()
    test_mapping_restore_readback_failure()
    test_mapping_emit_on_restoration_succeeds()
    test_sequence_does_not_pass_when_the_board_never_marks_music()
    print("A8_BENCH_EXPERIMENT_HOST=PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Lane M host/mock tests for M2–M9. Never opens CDC or Titan NVM."""
from __future__ import annotations

import json
import sys
import tempfile
import time
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE))

from mir_config_store import ConfigStoreError, HostConfigStore  # noqa: E402
from mir_evidence_kinds import (  # noqa: E402
    EVIDENCE_OBSERVATION_REPLAY,
    EVIDENCE_PCM_RECOMPUTE,
    EvidenceKindError,
    assert_not_merged,
    label_payload,
    same_claim_allowed,
)
from mir_ingress_adapters import HostIngressAdapter, IngressError, IngressFrame, SourceEpoch  # noqa: E402
from mir_mock_controls import ControlError, HostControlClient, MockControlEndpoint  # noqa: E402
from mir_observation_recorder import BoundedObservationRecorder  # noqa: E402
from mir_peer_hmi import MockPeerHmi, PeerHmiError, PeerMessage  # noqa: E402
import mir_workflow_cli as cli  # noqa: E402


def test_m2_rotation_gap_and_write_failure_detection():
    with tempfile.TemporaryDirectory(prefix="m2-rec-") as tmp:
        path = Path(tmp) / "obs.jsonl"
        rec = BoundedObservationRecorder(path, max_items=8, max_bytes=400, gap_threshold_s=10.0)
        for i in range(40):
            rec.put({"seq": i, "pad": "x" * 40})
        # Force queue-full gap path
        tiny = BoundedObservationRecorder(Path(tmp) / "tiny.jsonl", max_items=1, max_bytes=10_000_000)
        tiny.put({"a": 1})
        tiny.put({"a": 2})  # should drop / gap
        time.sleep(0.3)
        rec.mark_gap(reason="explicit")
        time.sleep(0.2)
        rec.close()
        tiny.close()
        text = path.read_text()
        assert '"kind": "gap"' in text or '"gap_reason"' in text
        assert rec.stats()["rotations"] >= 1 or rec.stats()["gaps"] >= 1
        assert tiny.stats()["failed"] is True
        assert tiny.stats()["dropped"] >= 1


def test_m3_observation_and_pcm_never_same_evidence():
    obs = label_payload(EVIDENCE_OBSERVATION_REPLAY, {"rows": 3})
    pcm = label_payload(EVIDENCE_PCM_RECOMPUTE, {"hops": 3})
    assert obs["evidence_kind"] != pcm["evidence_kind"]
    try:
        assert_not_merged(obs, pcm)
        raised = False
    except EvidenceKindError:
        raised = True
    assert raised
    assert same_claim_allowed(obs, pcm) is False
    try:
        label_payload(EVIDENCE_PCM_RECOMPUTE, obs)
        relabel_ok = True
    except EvidenceKindError:
        relabel_ok = False
    assert relabel_ok is False


def test_m4_controls_revision_atomic_stale_malformed_reordered_disconnect():
    ep = MockControlEndpoint()
    client = HostControlClient(ep)
    before = ep.readback()
    after = client.change({"mode_a": 7})
    assert after["revision"] == before["revision"] + 1
    assert after["mode_a"] == 7
    assert after["emit_on"] == 0

    try:
        ep.apply({"mode_a": 1}, expected_revision=before["revision"])
        stale_ok = True
    except ControlError as exc:
        stale_ok = False
        assert exc.code == "stale_revision"
    assert stale_ok is False

    try:
        ep.apply({"mode_a": 2}, expected_revision=ep.readback()["revision"], malformed=True)
        mal_ok = True
    except ControlError as exc:
        mal_ok = False
        assert exc.code == "malformed"
    assert mal_ok is False

    ep.apply({"mode_a": 3}, expected_revision=ep.readback()["revision"], sequence=10)
    try:
        ep.apply({"mode_a": 4}, expected_revision=ep.readback()["revision"], sequence=9)
        reorder_ok = True
    except ControlError as exc:
        reorder_ok = False
        assert exc.code == "reordered"
    assert reorder_ok is False

    ep.disconnect()
    try:
        client.change({"mode_a": 5})
        disc_ok = True
    except ControlError as exc:
        disc_ok = False
        assert exc.code == "disconnect"
    assert disc_ok is False


def test_m5_config_save_load_negatives():
    with tempfile.TemporaryDirectory(prefix="m5-store-") as tmp:
        store = HostConfigStore(Path(tmp))
        cfg = {"revision": 1, "mode_a": 32, "emit_on": 0}
        loaded = store.round_trip("preset-a", cfg)
        assert loaded["mode_a"] == 32
        path = Path(tmp) / "preset-a.json"
        path.write_text("{not-json")
        try:
            store.load("preset-a")
            corrupt_ok = True
        except ConfigStoreError as exc:
            corrupt_ok = False
            assert exc.code == "corruption"
        assert corrupt_ok is False

        path.write_text(json.dumps({"schema_version": 1, "persistence_scope": "host_mock"}) + "\n")
        try:
            store.load("preset-a")
            partial_ok = True
        except ConfigStoreError as exc:
            partial_ok = False
            assert exc.code == "partial_record"
        assert partial_ok is False

        path.write_text(
            json.dumps(
                {
                    "schema_version": 99,
                    "persistence_scope": "host_mock",
                    "config": cfg,
                }
            )
            + "\n"
        )
        try:
            store.load("preset-a")
            ver_ok = True
        except ConfigStoreError as exc:
            ver_ok = False
            assert exc.code == "version_mismatch"
        assert ver_ok is False

        path.write_text(
            json.dumps(
                {
                    "schema_version": 1,
                    "persistence_scope": "titan_nvm",
                    "config": cfg,
                    "unknown_field": True,
                }
            )
            + "\n"
        )
        try:
            store.load("preset-a")
            nvm_ok = True
        except ConfigStoreError as exc:
            nvm_ok = False
            assert exc.code == "scope"
        assert nvm_ok is False

        # Interrupted write: leave a .tmp and a prior good file; load still works.
        store.save("preset-b", cfg)
        (Path(tmp) / ".preset-b.interrupted.tmp").write_text("{incomplete")
        assert store.load("preset-b")["config"]["mode_a"] == 32


def test_m6_schema_marks_nvm_not_authorized():
    schema = HERE.parents[1] / "docs" / "contracts" / "M6-persistence-schema.json"
    data = json.loads(schema.read_text())
    assert data["live_nvm_operation"] == "NOT_AUTHORIZED"


def test_m7_ingress_source_epoch_contract():
    adapter = HostIngressAdapter(SourceEpoch("digital-silence", 3))
    adapter.admit(IngressFrame("digital-silence", 3, 1, [0]))
    try:
        adapter.admit(IngressFrame("other", 3, 2, [0]))
        src_ok = True
    except IngressError as exc:
        src_ok = False
        assert exc.code == "source_mismatch"
    assert src_ok is False
    try:
        adapter.admit(IngressFrame("digital-silence", 2, 3, [0]))
        ep_ok = True
    except IngressError as exc:
        ep_ok = False
        assert exc.code == "stale_epoch"
    assert ep_ok is False
    # Shared contract also rejects non-increasing sequence under a matched source/epoch.
    try:
        adapter.admit(IngressFrame("digital-silence", 3, 1, [0]))
        reorder_ok = True
    except IngressError as exc:
        reorder_ok = False
        assert exc.code == "reordered"
    assert reorder_ok is False
    adapter.admit(IngressFrame("digital-silence", 3, 4, [1]))
    adapter.rebind(SourceEpoch("digital-tone", 4))
    adapter.admit(IngressFrame("digital-tone", 4, 1, [2]))
    note = adapter.note()
    assert note["fidelity_claim"] is False
    assert note["accepted"] == 3
    assert note["rejected"] >= 3


def test_m8_peer_hmi_contract():
    peer = MockPeerHmi(owner="mir-host")
    peer.accept(PeerMessage(1, "mir-host", 1, "heartbeat"))
    try:
        peer.accept(PeerMessage(1, "mir-host", 1, "heartbeat"))
        stale_ok = True
    except PeerHmiError as exc:
        stale_ok = False
        assert exc.code == "stale_or_reordered"
    assert stale_ok is False
    # Explicit reordered (sequence regresses after a successful advance).
    peer.accept(PeerMessage(1, "mir-host", 2, "observe"))
    try:
        peer.accept(PeerMessage(1, "mir-host", 2, "observe"))
        reorder_ok = True
    except PeerHmiError as exc:
        reorder_ok = False
        assert exc.code == "stale_or_reordered"
    assert reorder_ok is False
    # Version mismatch.
    try:
        peer.accept(PeerMessage(99, "mir-host", 3, "observe"))
        ver_ok = True
    except PeerHmiError as exc:
        ver_ok = False
        assert exc.code == "version"
    assert ver_ok is False
    # Ownership: non-owner command rejected until handback.
    try:
        peer.accept(PeerMessage(1, "intruder", 3, "control"))
        own_ok = True
    except PeerHmiError as exc:
        own_ok = False
        assert exc.code == "owner"
    assert own_ok is False
    peer.disconnect()
    try:
        peer.accept(PeerMessage(1, "mir-host", 3, "observe"))
        disc_ok = True
    except PeerHmiError as exc:
        disc_ok = False
        assert exc.code == "disconnect"
    assert disc_ok is False
    peer.reconnect()
    peer.accept(PeerMessage(1, "mir-host", 3, "handoff", {"new_owner": "display"}))
    assert peer.owner == "display"
    # Handback complete: former owner may not command; new owner may.
    try:
        peer.accept(PeerMessage(1, "mir-host", 4, "control"))
        post_hand_ok = True
    except PeerHmiError as exc:
        post_hand_ok = False
        assert exc.code == "owner"
    assert post_hand_ok is False
    peer.accept(PeerMessage(1, "display", 4, "observe"))
    try:
        peer.accept(PeerMessage(1, "display", 5, "bogus"))
        mal_ok = True
    except PeerHmiError as exc:
        mal_ok = False
        assert exc.code == "malformed"
    assert mal_ok is False
    assert peer.contract()["mock_only"] is True
    assert peer.contract()["version"] == 1


def test_m9_workflow_cli_end_to_end():
    with tempfile.TemporaryDirectory(prefix="m9-cli-") as tmp:
        tmp_path = Path(tmp)
        assert cli.main(["status"]) == 0
        assert cli.main(["record", "--output", str(tmp_path / "rec"), "--count", "5", "--gap-every", "2", "--as-replay"]) == 0
        obs = tmp_path / "rec" / "observations.jsonl"
        assert obs.is_file()
        text = obs.read_text()
        assert text.count("\n") >= 5
        assert cli.main(["inspect", "--path", str(obs), "--output", str(tmp_path / "inspect.json")]) == 0
        assert cli.main(
            [
                "replay",
                "--mode",
                "observations",
                "--input",
                str(obs),
                "--output",
                str(tmp_path / "obs-replay"),
            ]
        ) == 0
        pcm = tmp_path / "silence.pcm"
        pcm.write_bytes(b"\x00" * 360)  # tiny digital fixture
        assert (
            cli.main(
                [
                    "replay",
                    "--mode",
                    "pcm",
                    "--input",
                    str(pcm),
                    "--output",
                    str(tmp_path / "pcm-replay"),
                    "--digital",
                ]
            )
            == 0
        )
        obs_label = json.loads((tmp_path / "obs-replay" / "OBSERVATION_REPLAY.json").read_text())[
            "evidence_kind"
        ]
        pcm_label = json.loads((tmp_path / "pcm-replay" / "PCM_RECOMPUTE.json").read_text())[
            "evidence_kind"
        ]
        assert obs_label == EVIDENCE_OBSERVATION_REPLAY
        assert pcm_label != obs_label
        assert cli.main(["control", "--mode-a", "11"]) == 0
        assert cli.main(["control", "--disconnect"]) == 0
        assert cli.main(["readback", "--output", str(tmp_path / "rb.json")]) == 0
        store = tmp_path / "store"
        assert cli.main(["save", "--store", str(store), "--name", "p1", "--config", str(tmp_path / "rb.json")]) == 0
        assert cli.main(["load", "--store", str(store), "--name", "p1", "--output", str(tmp_path / "loaded.json")]) == 0
        assert cli.main(["demo-peer"]) == 0
        assert cli.main(["demo-ingress"]) == 0
        m1 = HERE.parents[1] / "docs" / "contracts" / "M1-REAL-PCM-FIXTURE.json"
        assert json.loads(m1.read_text())["REAL_PCM_FIXTURE"] == "BLOCKED"


if __name__ == "__main__":
    test_m2_rotation_gap_and_write_failure_detection()
    test_m3_observation_and_pcm_never_same_evidence()
    test_m4_controls_revision_atomic_stale_malformed_reordered_disconnect()
    test_m5_config_save_load_negatives()
    test_m6_schema_marks_nvm_not_authorized()
    test_m7_ingress_source_epoch_contract()
    test_m8_peer_hmi_contract()
    test_m9_workflow_cli_end_to_end()
    print("lane_m_mir_workflow_ok")

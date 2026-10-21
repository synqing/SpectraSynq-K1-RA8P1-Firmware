"""A vanished CDC node stops the broker so its supervisor re-attaches; a transient error does not."""
from types import SimpleNamespace

from titan_broker import Broker


def _broker(tmp_path, device):
    b = Broker.__new__(Broker)
    b.replay_only = False
    b.bound = None  # Production __init__ establishes this before any attach.
    b.evidence = tmp_path
    b.transport = SimpleNamespace(allowed={1, 6, 17})
    b.poll_i = 0
    b.errors = []
    b.stop = False
    b.device_path = str(device)
    b.device_lost = None
    rows = []
    b.recorder = SimpleNamespace(put=rows.append)

    def transact(op, payload, **kwargs):
        raise OSError(6, "Device not configured")

    b.transact = transact
    return b, rows


def test_missing_device_node_stops_with_device_lost(tmp_path):
    b, rows = _broker(tmp_path, tmp_path / "cu.usbmodem-gone")
    b.poll_once()
    assert b.stop is True
    assert b.device_lost["device"].endswith("cu.usbmodem-gone")
    assert [r["kind"] for r in rows] == ["poll_error", "device_lost"]


def test_poll_error_with_device_present_keeps_running(tmp_path):
    node = tmp_path / "cu.usbmodem-present"
    node.write_text("")
    b, rows = _broker(tmp_path, node)
    b.poll_once()
    assert b.stop is False
    assert b.device_lost is None
    assert [r["kind"] for r in rows] == ["poll_error"]

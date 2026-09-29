"""One Emotiscope MIDI owner. It never opens the Titan serial port."""
from __future__ import annotations

from collections import deque
import sys
import threading
import time

from .broker_client import BrokerClient, UnixBroker
from .emotiscope_dashboard import make_emotiscope_dashboard
from .emotiscope_engine import load_profile
from .emotiscope_engine import EmotiscopeEngine
from .emotiscope_store import EmotiscopeStore
from .live_loop import RateLoop

LAUNCH = (
    "cd /Users/spectrasynq/Workspace_Management/Software/SMC-Mixer/.worktrees/titan-control-001\n"
    "PYTHONPATH=host/titan_control host/titan_control/.venv/bin/python -m smc_titan_control live "
    "--strategy emotiscope --socket /tmp/titan-emotiscope-control/titan-control.sock "
    "--port 'SINCO SMC-Mixer-Master'"
)


class PortError(ValueError):
    pass


def admit_endpoint(rows, port_name: str, profile: dict | None = None) -> dict:
    """Exactly one matching input. Private and unknown names are refused."""
    profile = profile or load_profile()
    spec = profile["endpoints"].get(port_name)
    if spec is None:
        raise PortError("endpoint is not an admitted SMC mixer input")
    matches = [
        row for row in rows
        if row.get("name") == port_name
        and row.get("driver") == spec["driver"]
        and row.get("unique_id") == spec["unique_id"]
        and row.get("offline") == 0
    ]
    if len(matches) != 1:
        raise PortError(f"need exactly one online {port_name}, found {len(matches)}")
    return spec


class GestureInbox:
    """MIDI callback only queues. Absolute runs keep the ends and the extremes."""

    def __init__(self, absolute_keys: set[tuple[int, int]]):
        self.absolute = absolute_keys
        self.groups = deque()
        self.overflow = False
        self.recent = deque(maxlen=32)
        self._lock = threading.Lock()
        self.port = None

    def receive(self, message) -> None:
        if getattr(message, "type", None) != "control_change":
            return
        key = (int(message.channel), int(message.control))
        value = int(message.value)
        with self._lock:
            self.recent.append((key, value))
            if self.groups and self.groups[-1][0] == key and key in self.absolute:
                values = self.groups[-1][1]
                values.append(value)
                if len(values) > 4:
                    first, last = values[0], values[-1]
                    ordered = [first]
                    for sample in (min(values), max(values), last):
                        if ordered[-1] != sample:
                            ordered.append(sample)
                    self.groups[-1] = (key, ordered)
                return
            if len(self.groups) >= 512:
                self.overflow = True
                return
            self.groups.append((key, [value]))

    def drain(self):
        with self._lock:
            batch = list(self.groups)
            self.groups.clear()
            return batch

    def close(self) -> None:
        port = self.port
        self.port = None
        if port is not None:
            port.close()


def run_emotiscope(args, endpoint_reader=None, opener=None) -> int:
    if not 1 <= args.hz <= 25:
        raise ValueError("Emotiscope host rate must be 1..25 Hz")
    profile = load_profile()
    if endpoint_reader is None:
        from .midi_endpoints import endpoints as endpoint_reader
    spec = admit_endpoint(endpoint_reader(), args.port, profile)
    transport = UnixBroker(str(args.socket))
    client = BrokerClient(transport)
    store = EmotiscopeStore(args.emotiscope_assignments, args.emotiscope_scenes)
    engine = EmotiscopeEngine(client, profile, store)
    try:
        engine.attach()
    except (OSError, ConnectionError) as exc:
        engine.last_error = str(exc)
        engine.ready = False
    if not engine.ready:
        sys.stderr.write(f"TITAN_CONTROL_UNAVAILABLE {engine.last_error}\n")
        transport.close()
        return 2
    absolute = {
        (int(rule["channel"]), int(rule["cc"]))
        for rule in profile["bindings"].values() if rule["kind"] == "absolute"
    }
    inbox = GestureInbox(absolute)
    if opener is None:
        import mido

        mido.set_backend("mido.backends.rtmidi")
        opener = mido.open_input
    try:
        inbox.port = opener(args.port, callback=inbox.receive)
    except (OSError, RuntimeError) as exc:
        engine.abandon()
        transport.close()
        sys.stderr.write(f"MIDI_UNAVAILABLE {exc}\n")
        return 2
    ble = None
    if spec["transport"] == "bluetooth":
        from .ble_monitor import BluetoothMonitor
        ble = BluetoothMonitor()
    owner = threading.RLock()

    def tick():
        with owner:
            if inbox.overflow:
                engine.note_overflow()
                return
            if spec["transport"] == "bluetooth" and ble is not None:
                snap = ble.snapshot()
                if spec["peripheral_id"] not in snap.get("connected_ids", []):
                    engine.abandon()
                    return
            for key, values in inbox.drain():
                for value in values:
                    engine.observe(key[0], key[1], value)
            engine.tick(time.monotonic())

    def status():
        with owner:
            body = engine.status()
            body["endpoint"] = args.port
            body["launch"] = LAUNCH
            return body

    loop = RateLoop(args.hz, tick)
    http = make_emotiscope_dashboard(status, engine, args.dashboard_port)
    status_http = make_emotiscope_dashboard(status, engine, args.status_port)
    status_thread = threading.Thread(target=status_http.serve_forever, daemon=True, name="emotiscope-status")
    status_thread.start()
    loop.start()
    print("EMOTISCOPE_DASHBOARD http://127.0.0.1:" + str(args.dashboard_port), flush=True)
    try:
        http.serve_forever()
    except KeyboardInterrupt:
        return 0
    finally:
        loop.close()
        with owner:
            engine.abandon()
            inbox.close()
            if ble is not None:
                ble.close()
            transport.close()
        status_http.shutdown()
        status_http.server_close()
        http.server_close()
    return 0 if loop.healthy else 3

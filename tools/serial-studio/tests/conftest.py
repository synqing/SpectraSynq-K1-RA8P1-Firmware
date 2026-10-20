"""Keep fake-broker TCP sockets separate from the live Titan owner."""
import sys
from pathlib import Path
import pytest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

@pytest.fixture(autouse=True)
def isolated_broker_publisher(monkeypatch):
    import titan_broker
    monkeypatch.setattr(titan_broker, "BIND", ("127.0.0.1", 0))
@pytest.fixture(autouse=True)
def isolated_maintenance_owner(monkeypatch, tmp_path):
    import titan_control_server
    monkeypatch.setattr(titan_control_server, 'OWNER_LEASE_PATH', tmp_path/'owner.json')

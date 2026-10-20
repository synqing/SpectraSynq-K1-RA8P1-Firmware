"""Background read-only CoreBluetooth observations; no work on the owner thread."""
import json
from pathlib import Path
import subprocess
import threading
import time

class BluetoothMonitor:
    def __init__(self):
        self.lock=threading.Lock();self.observation={};self.at=0.;self.process=None
        path=Path(__file__).resolve().parents[1]/'tools/smc-ble-monitor'
        try:
            self.process=subprocess.Popen([str(path)],stdout=subprocess.PIPE,stderr=subprocess.DEVNULL,text=True)
            self.thread=threading.Thread(target=self._read,daemon=True,name='bluetooth-observer');self.thread.start()
        except OSError:pass
    def _read(self):
        for line in self.process.stdout:
            try:row=json.loads(line)
            except ValueError:continue
            with self.lock:self.observation=row;self.at=time.monotonic()
    def snapshot(self):
        with self.lock:return {**self.observation,'fresh':time.monotonic()-self.at<2}
    def close(self):
        if self.process:
            self.process.terminate()
            try:self.process.wait(timeout=2)
            except subprocess.TimeoutExpired:self.process.kill();self.process.wait()

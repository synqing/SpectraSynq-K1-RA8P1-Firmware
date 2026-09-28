"""Bounded read-only framing recovery; preserves drained bytes; never reset/emit."""
import json,sys,time,traceback
from pathlib import Path
ROOT=Path(__file__).resolve().parents[5]
sys.path.insert(0,str(ROOT/'tools/serial-studio'))
from titan_broker import find_titan
from titan_transport import open_serial,close_serial,Transport,EXPECTED_UID
import cdc_lock
E=Path(__file__).parent
usb=find_titan(inventory_lsof=False)
handle=cdc_lock.acquire(usb['device'],'ap-read-only-identity');port=None
result={'usb':usb,'hardware_reset':False,'emission_changed':False}
try:
 port=open_serial(usb['device'],handle)
 raw=bytearray();end=time.monotonic()+2.0
 while time.monotonic()<end and len(raw)<32768:
  chunk=port.read(1024)
  raw.extend(chunk)
  if not chunk:break
 (E/'preserved-drained-bytes.bin').write_bytes(raw)
 t=Transport(port,allowed={1})
 info=t.info(timeout=2.0);result['info']=info
 if info['info'].get('uid')!=EXPECTED_UID:raise RuntimeError('UID mismatch')
 t.allow({1,17}); result['palette_status']=t.transact(17,b'',timeout=2.0)
 result['success']=True
except Exception as e:
 result['success']=False;result['error']=repr(e);result['traceback']=traceback.format_exc()
finally:
 close_serial(port,handle);cdc_lock.release(handle)
 # Transport bodies are JSON decoded by transact when applicable.
 (E/'resynchronised-result.json').write_text(json.dumps(result,indent=2,default=lambda x:x.hex() if isinstance(x,bytes) else str(x))+'\n')
 print(json.dumps(result,default=lambda x:x.hex() if isinstance(x,bytes) else str(x)))
raise SystemExit(0 if result.get('success') else 1)

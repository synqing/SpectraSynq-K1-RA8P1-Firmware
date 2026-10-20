"""Run the permanent real-time ownership regressions; never operate hardware."""
from pathlib import Path
import argparse,json,subprocess,hashlib,time,sys
ROOT=Path(__file__).resolve().parents[1]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True);args=ap.parse_args()
 args.output.mkdir(parents=True,exist_ok=False)
 contract=ROOT/'docs/contracts/titan-realtime-regressions.json';cfg=json.loads(contract.read_text())
 result=dict(scope=cfg['scope'],contract_sha256=hashlib.sha256(contract.read_bytes()).hexdigest(),checks=[])
 for c in cfg['checks']:
  start=time.time_ns();r=subprocess.run(c['argv'],cwd=ROOT,capture_output=True,text=True,timeout=240)
  log=args.output/(c['id']+'.log');log.write_text(r.stdout+r.stderr)
  row=dict(**c,exit_code=r.returncode,start_ns=start,end_ns=time.time_ns(),log_sha256=hashlib.sha256(log.read_bytes()).hexdigest());result['checks'].append(row)
  (args.output/'result.json').write_text(json.dumps(result,indent=2)+'\n')
  print(c['id'],c['purpose'],r.returncode,flush=True)
  if r.returncode:return r.returncode
 return 0
if __name__=='__main__':sys.exit(main())

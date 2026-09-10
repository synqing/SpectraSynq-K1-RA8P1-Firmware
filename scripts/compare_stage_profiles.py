#!/usr/bin/env python3
"""Compare paired instrumented/control Titan schedule receipts."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


def _sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def _load(directory: Path) -> tuple[dict, Path]:
    path=directory/'receipt.json'
    receipt=json.loads(path.read_text())
    if not receipt.get('measurement_validated') or not receipt.get('result',{}).get('finished'):
        raise RuntimeError(f'unvalidated or incomplete schedule measurement: {path}')
    return receipt,path


def compare(profile_directory: Path,control_directory: Path) -> dict:
    profiled,profile_path=_load(profile_directory)
    control,control_path=_load(control_directory)
    for field in ('loops','mode','mutation','qualification'):
        if profiled.get(field)!=control.get(field):
            raise RuntimeError(f'paired receipt mismatch: {field}')
    if profiled.get('mode')!='scalar' or profiled.get('mutation'):
        raise RuntimeError('stage overhead comparison requires ordinary scalar runs')
    if profiled.get('identities',{}).get('profile_sha256')!=control.get('identities',{}).get('profile_sha256'):
        raise RuntimeError('paired workload profile mismatch')
    if profiled.get('runtime',{}).get('uid')!=control.get('runtime',{}).get('uid'):
        raise RuntimeError('paired target UID mismatch')
    if profiled.get('runtime',{}).get('source')!=control.get('runtime',{}).get('source'):
        raise RuntimeError('paired source pin mismatch')
    stage_profile=profiled['result'].get('stage_profile')
    if not isinstance(stage_profile,dict):
        raise RuntimeError('instrumented receipt has no stage profile')
    if 'stage_profile' in control['result']:
        raise RuntimeError('control receipt unexpectedly contains a stage profile')
    clock_hz=profiled['runtime'].get('clock_hz')
    if not isinstance(clock_hz,int) or clock_hz<=0 or clock_hz!=control['runtime'].get('clock_hz'):
        raise RuntimeError('paired target clock mismatch')

    def delta(name: str) -> dict:
        p=profiled['result'][name]
        c=control['result'][name]
        difference=p['mean_us']-c['mean_us']
        return {
            'profile_mean_us':p['mean_us'],
            'control_mean_us':c['mean_us'],
            'overhead_mean_us':round(difference,3),
            'overhead_percent':round(100.0*difference/c['mean_us'],3) if c['mean_us'] else None,
        }

    converted={}
    for name,measurement in stage_profile['stages'].items():
        converted[name]={
            'count':measurement['count'],
            'mean_us':round(measurement['mean_cycles']*1_000_000.0/clock_hz,3),
            'p50_bin_lower_us':round(measurement['p50_bin_lower_cycles']*1_000_000.0/clock_hz,3),
            'p95_bin_lower_us':round(measurement['p95_bin_lower_cycles']*1_000_000.0/clock_hz,3),
            'p99_bin_lower_us':round(measurement['p99_bin_lower_cycles']*1_000_000.0/clock_hz,3),
            'max_us':round(measurement['max_cycles']*1_000_000.0/clock_hz,3),
        }
    ranking=sorted(
        ({'stage':name,'mean_us':value['mean_us']} for name,value in converted.items()),
        key=lambda item:item['mean_us'],reverse=True,
    )
    return {
        'label':'ON-SILICON',
        'status':'PAIRED_STAGE_MEASUREMENT',
        'profile_receipt':str(profile_path),
        'profile_receipt_sha256':_sha(profile_path),
        'control_receipt':str(control_path),
        'control_receipt_sha256':_sha(control_path),
        'uid':profiled['runtime']['uid'],
        'source_pin':profiled['runtime']['source'],
        'clock_hz':clock_hz,
        'profile_build_id':profiled['runtime']['build'],
        'control_build_id':control['runtime']['build'],
        'overhead':{name:delta(name) for name in ('total','tempo','ordinary','render')},
        'stage_measurements':converted,
        'ranked_by_mean_us':ranking,
        'warning':'Nested measurements overlap and must not be summed.',
    }


def main() -> None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile-run',type=Path,required=True)
    parser.add_argument('--control-run',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    if args.output.exists(): raise RuntimeError('output already exists')
    result=compare(args.profile_run,args.control_run)
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))


if __name__=='__main__': main()

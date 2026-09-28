#!/usr/bin/env python3
"""One bounded run through the existing broker; always request disarm on exit."""
import argparse
import json
import time
from pathlib import Path
from titan_window_health import WindowHealth, HealthError


def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--evidence',type=Path,required=True)
    ap.add_argument('--capture',action='store_true')
    ap.add_argument('--wire-snapshots',action='store_true',
                    help='read the retained source/packed frame once per second; adds USB traffic')
    args=ap.parse_args(); root=args.evidence
    checkpoint=json.loads((root/'accepted-checkpoint.json').read_text())
    health=WindowHealth(checkpoint['uid'],checkpoint['build'],checkpoint['schema_sha256'])
    log=root/'diagnostic.jsonl'; result=root/'diagnostic-result.json'
    if log.exists() or result.exists(): raise SystemExit('refuse to reuse diagnostic receipts')
    def read(phase, emitting=False):
        try: row=json.loads((root/'live-stats.json').read_text())
        except (OSError,ValueError) as exc: raise HealthError('telemetry unavailable') from exc
        now=time.monotonic()
        with log.open('a') as f: f.write(json.dumps(dict(phase=phase,mono=now,live=row))+'\n')
        health.check(row,now,emitting)
        return row
    verdict='INVALID'; reason='not started'; last={}; armed_at=None
    start_comp=start_gen=None
    try:
        deadline=time.monotonic()+10
        first=read('pre-arm'); start_hops=first['pdm']['ap_hops']
        while True:
            last=read('pre-arm')
            if last['palette']['emit_enabled'] is not False: raise HealthError('pre-arm emission not off')
            if last['pdm']['ap_hops']>start_hops: break
            if time.monotonic()>=deadline: raise HealthError('pre-arm no audio progress')
            time.sleep(.2)
        start_comp=last['palette']['pair_completions'];start_gen=last['palette']['submitted_generation']
        if args.capture:
            (root/'LOOPBACK_REQUEST').write_text('next-emitted-generation\n')
            deadline=time.monotonic()+3
            while not (root/'loopback-request.json').exists():
                if time.monotonic()>deadline:raise HealthError('capture request timeout')
                time.sleep(.1)
            if not json.loads((root/'loopback-request.json').read_text()).get('ok'):
                raise HealthError('capture request rejected')
        armed_at=time.monotonic(); (root/'EMIT_ARM').write_text('bounded8seconds\n')
        next_wire = armed_at + 0.5
        while time.monotonic()-armed_at<8:
            last=read('run',True)
            if (root/'emit-error.json').exists():raise HealthError('broker operation error')
            if (args.wire_snapshots and time.monotonic() >= next_wire
                    and last['palette']['emit_enabled'] is True):
                request = root/'WIRE_SNAPSHOT_REQUEST'
                if not request.exists():
                    request.write_text('bounded\n')
                    next_wire = time.monotonic()+1.0
            time.sleep(.2)
        if last['palette']['pair_completions']-start_comp < 30:
            raise HealthError('insufficient cumulative completions','FAIL')
        if last['palette']['completed_generation'] <= start_gen:
            raise HealthError('no completed generation progress','FAIL')
        verdict='PASS';reason='bounded software progress only'
    except HealthError as exc:
        verdict=exc.verdict;reason=str(exc)
    except Exception as exc:
        verdict='INVALID';reason=repr(exc)
    finally:
        disarm_at=time.monotonic()
        (root/'EMIT_DISARM').write_text('stop\n')
        deadline=time.monotonic()+4; off=False
        while time.monotonic()<deadline:
            try:
                row=json.loads((root/'live-stats.json').read_text())
                t=row.get('topic_mono',{}).get('palette',0)
                pal=row.get('palette',{})
                if (t >= disarm_at and time.monotonic()-t<4 and pal.get('emit_enabled') is False
                    and all((row.get('identity') or {}).get(k)==v for k,v in health.expected.items())
                    and ((row.get('identity') or {}).get('boot_id'),row.get('epoch'),row.get('pid')) == health.identity
                    and type(pal.get('completed_generation')) is int
                    and pal['completed_generation']>=pal.get('submitted_generation',float('inf'))
                    and (root/'emit-disarm.json').exists()):
                    off=True;last=row;break
            except (OSError,ValueError,TypeError):pass
            time.sleep(.1)
        if not off:
            verdict='FAIL';reason+='; transmission off not verified — REMOVE LED POWER'
        if verdict != 'PASS': (root/'DIAGNOSTIC_FAULT').write_text(reason+'\n')
        result.write_text(json.dumps(dict(verdict=verdict,reason=reason,scope='software progress and telemetry',
            transmission_off_verified=off,optical='UNKNOWN',start_completions=start_comp,
            start_generation=start_gen,elapsed_from_arm=None if armed_at is None else time.monotonic()-armed_at,
            final=last),indent=2)+'\n')
        print(result.read_text())
    return 0 if verdict=='PASS' else 2

if __name__=='__main__':raise SystemExit(main())

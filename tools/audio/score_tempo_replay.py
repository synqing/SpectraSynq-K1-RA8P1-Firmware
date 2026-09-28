#!/usr/bin/env python3
"""Independent descriptive replay scoring under the frozen PCM rules.

No estimator threshold, model or held-out tuning is performed. A zero exit means
input identity and parity receipts validate, not musical-quality admission.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import statistics
import sys

ROOT=Path(__file__).resolve().parents[2]
EVIDENCE=ROOT/'docs/evidence/tempo-field/20260928'
MANIFEST=ROOT/'tests/fixtures/tempo_field/v1/pcm/manifest.json'

def sha(path):
    h=hashlib.sha256()
    with path.open('rb') as source:
        for block in iter(lambda:source.read(1024*1024),b''):h.update(block)
    return h.hexdigest()

def distribution(values):
    if not values:return {'count':0,'mean':None,'median':None,'p95':None,'maximum':None}
    values=sorted(values)
    return {'count':len(values),'mean':statistics.fmean(values),'median':statistics.median(values),
            'p95':values[max(0,math.ceil(.95*len(values))-1)],'maximum':values[-1]}

def attacks(events,detections):
    if events is None:return {'status':'NOT ANNOTATED','detected_events':len(detections)}
    unused=set(range(len(events)));matched=[];false=[]
    for event in detections:
        # Frozen policy: nearest unused annotated attack, causal 0..150 ms.
        eligible=[i for i in unused if 0<=event['event_ms']-events[i]['sample']/24<=150]
        if not eligible:false.append(event);continue
        i=min(eligible,key=lambda j:event['event_ms']-events[j]['sample']/24)
        unused.remove(i)
        matched.append({'annotation_index':i,**event,
                        'event_delay_ms':event['event_ms']-events[i]['sample']/24,
                        'first_available_delay_ms':event['available_ms']-events[i]['sample']/24})
    return {'status':'DESCRIPTIVE ONLY','annotated_attacks':len(events),'detected_events':len(detections),
            'matched':len(matched),'misses':len(unused),'false_positives':len(false),
            'event_delay_ms':distribution([m['event_delay_ms'] for m in matched]),
            'first_available_delay_ms':distribution([m['first_available_delay_ms'] for m in matched]),
            'missed_annotation_indices':sorted(unused),'false_events':false,'matches':matched}

def reference(fixture,t):
    events=fixture['events']
    if not events:return None
    start=events[0]['sample']/24000;end=events[-1]['sample']/24000+.03
    if not start<=t<=end:return None
    if fixture['id']=='sparse_reentry' and 12<=t<22:return None
    if fixture['id']=='tempo_step':return 90 if t<18 else 135
    return fixture['reference_bpm']

def score_fixture(fixture):
    name=fixture['id'];path=EVIDENCE/'candidate'/f'{name}.jsonl'
    errors=[];families=[];locked_errors=[];candidate_best=[];weighted_errors=[];phase_errors=[]
    ages=[];raw_ages=[];counts=[];onsets=[];seen_onsets=set();summaries=[]
    scene=[{'total_error':[],'shape_tv':[],'peak':0.,'nonzero_hops':0,'difference_hops':0} for _ in range(8)]
    attribution={key:[] for key in ('onsets_only_1_vs_0','phase_only_4_vs_0','full_without_onsets_7_vs_6','full_without_phase_7_vs_3')}
    authority=coasting=warm_hops=postwarm_authority=active_hops=0
    first_lock=first_phase=None;previous_gen={};generation_restarts=0
    silence_runs=[];silence_run=None;silence_violations=[]
    with path.open() as source:
      for line in source:
        row=json.loads(line);t=row['media_end_48k']/48000;f=row['features'];field=row['tempo_field'];tempo=row['tempo']
        truth=reference(fixture,t)
        if truth is not None:
            err=abs(tempo['bpm']-truth);errors.append(err);families.append(min(abs(tempo['bpm']-truth*r) for r in (.5,1,2)))
            if tempo['locked']:locked_errors.append(err)
        if tempo['locked'] and first_lock is None:first_lock=t
        if field['phase_authority']:
            authority+=1
            if first_phase is None:first_phase=t
        if field['warming']:warm_hops+=1
        elif field['phase_authority']:postwarm_authority+=1
        if field['coasting']:coasting+=1
        if truth is not None:active_hops+=1
        valid=[c for c in field['candidates'] if c['phase_valid']]
        if field['silence']:
            if silence_run is None:
                silence_run={'start_frame':row['media_end_48k'],'end_frame':row['media_end_48k'],
                             'last_direct_observed_frame':max((c['last_observed_frame'] for c in field['candidates']),default=0),
                             'last_authoritative_frame':None,'first_no_authority_frame':None,
                             'fresh_phase_hops':0,'observations':{}}
                silence_runs.append(silence_run)
            silence_run['end_frame']=row['media_end_48k']
            silence_run['fresh_phase_hops']+=int(field['fresh_phase_count']>0)
            if field['phase_authority']:silence_run['last_authoritative_frame']=row['media_end_48k']
            elif silence_run['first_no_authority_frame'] is None:silence_run['first_no_authority_frame']=row['media_end_48k']
            if field['fresh_phase_count'] and len(silence_violations)<20:
                silence_violations.append({'frame':row['media_end_48k'],'reason':'fresh phase during declared silence'})
            for c in field['candidates']:
                key=f"{c['bin_id']}:{c['generation']}";old=silence_run['observations'].get(key)
                if old is not None and c['last_observed_frame']!=old and len(silence_violations)<20:
                    silence_violations.append({'frame':row['media_end_48k'],'reason':'observation refreshed in silence','track':key})
                silence_run['observations'][key]=c['last_observed_frame']
                if c['phase_valid'] and row['media_end_48k']-c['last_observed_frame']>48000 and len(silence_violations)<20:
                    silence_violations.append({'frame':row['media_end_48k'],'reason':'phase exceeds coast cutoff','track':key})
        else:silence_run=None
        counts.append(len(valid))
        for c in valid:
            ages.append(c['observation_age_frames']/48)
            if c['raw_observation_valid']:raw_ages.append(c['raw_observation_age_frames']/48)
            if truth is not None and c['bpm']==math.floor(truth+.5):
                projected=(c['phase01']+(row['media_end_48k']-c['reference_frame'])*(2**32)/c['beat_period_q32'])%1
                anchor=18 if name=='tempo_step' and t>=18 else fixture['events'][0]['sample']/24000
                expected=((t-anchor)*truth/60)%1
                phase_errors.append(abs((projected-expected+.5)%1-.5)*60000/truth)
            old=previous_gen.get(c['bin_id'])
            if old is not None and old!=c['generation']:generation_restarts+=1
            previous_gen[c['bin_id']]=c['generation']
        if truth is not None and valid:
            candidate_best.append(min(abs(c['bpm']-truth) for c in valid))
            weight=sum(c['salience'] for c in valid)
            if weight>0:weighted_errors.append(sum(c['salience']*abs(c['bpm']-truth) for c in valid)/weight)
        eid=f['onset_event_id']
        if eid and eid not in seen_onsets:
            seen_onsets.add(eid);onsets.append({'id':eid,'event_ms':f['onset_event_ms'],'available_ms':row['available_us']/1000})
        variants=row['scene_variants']
        if len(variants)!=8:raise ValueError('Missing scene masks: '+name)
        normalised=[]
        for mask,v in enumerate(variants):
            a=v['baseline'];b=v['candidate'];sa=math.fsum(a);sb=math.fsum(b)
            scene[mask]['total_error'].append(abs(sa-sb)*2) # mirrored 80-value half-strip
            scene[mask]['peak']=max(scene[mask]['peak'],max(a+b))
            shape=[x/sb for x in b] if sb>0 else [0.]*len(b);normalised.append(shape)
            if sa>0 and sb>0:
                scene[mask]['nonzero_hops']+=1
                tv=.5*math.fsum(abs(x/sa-y/sb) for x,y in zip(a,b))
                scene[mask]['shape_tv'].append(tv)
                if a!=b:scene[mask]['difference_hops']+=1
        for label,i,j in [('onsets_only_1_vs_0',1,0),('phase_only_4_vs_0',4,0),
                          ('full_without_onsets_7_vs_6',7,6),('full_without_phase_7_vs_3',7,3)]:
            if sum(variants[i]['candidate'])>0 and sum(variants[j]['candidate'])>0:
                attribution[label].append(.5*math.fsum(abs(a-b) for a,b in zip(normalised[i],normalised[j])))
        summaries.append({'t':t,'bpm':tempo['bpm'],'locked':bool(tempo['locked']),
                          'phase':bool(field['phase_authority']),'valid_count':len(valid),
                          'silence':bool(field['silence']),
                          'correct_bin':truth is not None and tempo['bpm']==math.floor(truth+.5)})
    n=len(summaries);events=fixture['events'];start=events[0]['sample']/24000 if events else None
    acquisition={}
    def first_at(label,origin,predicate,end=None):
        rows=[r for r in summaries if r['t']>=origin and (end is None or r['t']<end)]
        value=next((r['t'] for r in rows if predicate(r)),None)
        acquisition[label]={'origin_seconds':origin,'first_seconds':value,
                            'delay_seconds':None if value is None else value-origin,
                            'already_true_at_first_hop':bool(rows and predicate(rows[0])),
                            'meaning':'first occurrence only; no unstated stable-lock or accuracy threshold'}
    if start is not None:
        first_at('first_legacy_lock',start,lambda r:r['locked'])
        first_at('first_nearest_bank_bin',start,lambda r:r['correct_bin'])
        first_at('first_phase_authority',start,lambda r:r['phase'])
    if name=='sparse_reentry':
        first_at('reentry_lock',22,lambda r:r['locked'])
        first_at('reentry_phase',22,lambda r:r['phase'])
        acquisition['gap_phase_authority_fraction']=statistics.fmean(r['phase'] for r in summaries if 12<=r['t']<22)
        acquisition['gap_declared_silence_fraction']=statistics.fmean(r['silence'] for r in summaries if 12<=r['t']<22)
    for run in silence_runs:
        run['observed_track_timestamps']=run.pop('observations')
        direct=run['last_direct_observed_frame']
        run['coast_cutoff_frame']=direct+48000 if direct else None
        run['stale_cutoff_frame']=direct+96000 if direct else None
    if name=='tempo_step':
        first_at('step_to_135_nearest_bin',18,lambda r:r['bpm']==135)
        first_at('step_to_135_locked',18,lambda r:r['bpm']==135 and r['locked'])
        acquisition['step_first_second_abs_bpm_error']=distribution([abs(r['bpm']-135) for r in summaries if 18<=r['t']<19])
        acquisition['step_last_second_abs_bpm_error']=distribution([abs(r['bpm']-135) for r in summaries if 33<=r['t']<34])
    return {'fixture':name,'split':fixture['split'],'hops':n,'duration_seconds':fixture['duration_seconds'],
            'truth_scope':fixture['annotation'],'trace_sha256':sha(path),
            'incumbent_exact_abs_bpm_error':distribution(errors),'incumbent_half_exact_double_family_abs_bpm_error':distribution(families),
            'locked_exact_abs_bpm_error':distribution(locked_errors),
            'best_valid_candidate_abs_bpm_error_not_a_winner':distribution(candidate_best),
            'salience_weighted_candidate_abs_bpm_error_not_a_winner':distribution(weighted_errors),
            'phase_error_ms_exploratory_nearest_bin_only':distribution(phase_errors),
            'phase_error_qualification':'UNQUALIFIED musical metric; no registered beat-accuracy tolerance',
            'onsets':attacks(events,onsets),'acquisition':acquisition,
            'field':{'authority_hop_fraction':authority/n,'postwarm_authority_fraction':postwarm_authority/(n-warm_hops) if n>warm_hops else None,
                     'coasting_hop_fraction':coasting/n,'warming_hops':warm_hops,'valid_track_count':distribution(counts),
                     'phase_observation_age_ms':distribution(ages),'raw_bank_observation_age_ms':distribution(raw_ages),
                     'observed_generation_changes_per_bin':generation_restarts,
                     'silence_lifecycle_pass':not silence_violations,'silence_violations_first20':silence_violations,
                     'silence_runs':silence_runs},
            'scene_masks':[{'mask':i,'bits':'1 onset, 2 incumbent clock, 4 candidate phase',
                            'paired_total_abs_error':distribution(s['total_error']),
                            'normalised_shape_total_variation':distribution(s['shape_tv']),
                            'maximum_level':s['peak'],'nonzero_hops':s['nonzero_hops'],
                            'different_hops':s['difference_hops']} for i,s in enumerate(scene)],
            'normalised_shape_attribution':{k:distribution(v) for k,v in attribution.items()}}

def write_report(output,path):
    def num(value):return 'unannotated' if value is None else f'{value:.3f}'
    lines=['THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS.','',
           'Independent replay scoring; descriptive host evidence, 28 September 2026.','',
           'All 30 candidate / field-off / phase-off parity receipts match their current trace, baseline and frozen PCM identities. '
           'This preserves the incumbent detector trajectory; it does not improve detector accuracy.','',
           '| Excerpt | Exact BPM MAE | Half/exact/double-family MAE | Matched / missed / false attacks | Phase authority coverage | Phase shape contribution |',
           '|---|---:|---:|---|---:|---:|']
    for f in output['fixtures']:
        onset=f['onsets'];attack='unannotated' if 'matched' not in onset else f"{onset['matched']} / {onset['misses']} / {onset['false_positives']}"
        lines.append(f"| {f['fixture']} ({f['split']}) | {num(f['incumbent_exact_abs_bpm_error']['mean'])} | "
                     f"{num(f['incumbent_half_exact_double_family_abs_bpm_error']['mean'])} | {attack} | "
                     f"{100*f['field']['authority_hop_fraction']:.1f}% | {num(f['normalised_shape_attribution']['full_without_phase_7_vs_3']['mean'])} |")
    max_level_error=max(s['paired_total_abs_error']['maximum'] for f in output['fixtures'] for s in f['scene_masks'])
    lookup={f['fixture']:f for f in output['fixtures']}
    step=lookup['tempo_step']['acquisition'];sparse=lookup['sparse_reentry']
    lines.extend(['',
        'BPM errors use annotated active spans and include acquisition. The family metric allows only half/exact/double references; '
        'a 2:3 answer is not silently counted as exact. Blank numeric truth is unavailable, never a perfect zero. '
        'The attack window is the frozen causal 0..150 ms rule using unique detected event IDs and nearest unused annotations.','',
        f"The 90→135 BPM step first reaches the 135 bin after {step['step_to_135_nearest_bin']['delay_seconds']:.4f} s and "
        f"first locks there after {step['step_to_135_locked']['delay_seconds']:.4f} s. These are first occurrences, not a new stability threshold. "
        'Syncopation remains a substantial tempo-estimation weakness, and the attack counts retain misses and false positives.','',
        f'All eight scene masks were scored. Maximum paired total-output mismatch is {max_level_error:.9g} level units. '
        'Phase shape contribution is mean total-variation distance between the normalised full candidate and the same candidate with phase disabled; '
        '0 means identical distribution and 1 means disjoint distribution. Normalisation removes brightness as an explanation. '
        'Nonzero values prove an independent phase contribution reaches the fixed scene. They do not establish attractive motion or listener preference.','',
        f"The dense fixture has only {100*lookup['dense']['field']['authority_hop_fraction']:.1f}% phase-authority coverage and "
        f"mean phase shape contribution {lookup['dense']['normalised_shape_attribution']['full_without_phase_7_vs_3']['mean']:.6f}; "
        'there is no measured phase contribution to its visible scene. The conservative inherited silence decision suppresses phase observations '
        'and the scene envelope. This is a recorded availability limitation, not a successful dense-music result. '
        f"Held-out song-1 retains {100*lookup['song-1']['field']['authority_hop_fraction']:.1f}% phase-authority coverage. ", '',
        f"Sparse-gap authority coverage during 12..22 s is {100*sparse['acquisition']['gap_phase_authority_fraction']:.3f}%; "
        f"the detector declares silence for {100*sparse['acquisition']['gap_declared_silence_fraction']:.3f}% of that span. "
        f"Silence lifecycle checks across all excerpts: {'PASS' if output['all_silence_lifecycle_checks_pass'] else 'FAIL'}. "
        'Exact last-observed/coast/stale frame coordinates are retained below and in JSON.','',
        '| Sparse silence start/end frames | Last direct observation | Coast cutoff | Last authoritative frame | First no-authority frame | Fresh-phase hops |',
        '|---|---:|---:|---:|---:|---:|'])
    for r in sparse['field']['silence_runs']:
        lines.append(f"| {r['start_frame']} / {r['end_frame']} | {r['last_direct_observed_frame']} | {r['coast_cutoff_frame']} | "
                     f"{r['last_authoritative_frame']} | {r['first_no_authority_frame']} | {r['fresh_phase_hops']} |")
    lines.extend(['','The first attempt kept phase authority throughout the sparse silent gap by correlating historic energy. '
                  'Its scored result is preserved in replay-metrics-initial-failure.json. The repair prevents fresh phase observations '
                  'when the current field declares silence, preserving the last observation for bounded coasting. AP-A historical salience remains available.','',
                  'AP-AC05 musical phase qualification remains NOT TESTED: exploratory phase-to-annotation error distributions are reported without '
                  'inventing an acceptance tolerance. song-0 and held-out song-1 have no attack/beat truth and receive no beat-accuracy result. '
                  'No held-out tuning occurred. Physical target timing, optical output and musical preference are outside this scorer.','',
                  f"Scorer SHA-256: `{output['source_sha256']}`. Manifest SHA-256: `{output['manifest_sha256']}`."])
    path.write_text('\n'.join(lines)+'\n')

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,default=EVIDENCE/'replay-metrics.json')
    parser.add_argument('--self-test',action='store_true');args=parser.parse_args()
    if args.self_test:
        events=[{'sample':24000},{'sample':48000}]
        detections=[{'id':1,'event_ms':1000,'available_ms':1007.5},
                    {'id':2,'event_ms':2150,'available_ms':2152.5},
                    {'id':3,'event_ms':2150,'available_ms':2152.5}]
        result=attacks(events,detections)
        assert (result['matched'],result['misses'],result['false_positives'])==(2,0,1)
        assert attacks(events,[{'id':1,'event_ms':2150.001,'available_ms':2155}])['matched']==0
        assert attacks(events,[{'id':1,'event_ms':999.999,'available_ms':1000}])['matched']==0
        assert attacks(None,[])['status']=='NOT ANNOTATED'
        assert distribution([])['mean'] is None
        assert distribution([1,2,3,4])['median']==2.5
        print('Scorer self-tests PASS: causal0/150 inclusive, out-of-window, duplicate, missing annotation, distributions')
        return
    manifest=json.loads(MANIFEST.read_text());receipt_path=EVIDENCE/'comparison-results.json';receipt=json.loads(receipt_path.read_text())
    if sha(MANIFEST)!=receipt['manifest_sha256']:raise ValueError('Manifest identity differs from parity receipt')
    if sha(ROOT/'tools/audio/freeze_tempo_fixtures.py')!=manifest['generator_sha256']:raise ValueError('Generator differs from frozen manifest')
    for f in manifest['fixtures']:
        if sha(MANIFEST.parent/f['path'])!=f['sha256']:raise ValueError('PCM hash mismatch: '+f['id'])
    parity=[]
    for row in receipt['rows']:
        path=EVIDENCE/row['variant']/(row['fixture']+'.jsonl');base=EVIDENCE/'baseline'/(row['fixture']+'.jsonl')
        good=row['parity'] and row['returncode']==0 and row['first_difference'] is None and sha(path)==row['trace_sha256'] and sha(base)==row['baseline_sha256']
        parity.append({'fixture':row['fixture'],'variant':row['variant'],'receipt_and_hashes_valid':good,'hops':row['hops_compared']})
    if len(parity)!=30 or not all(r['receipt_and_hashes_valid'] for r in parity):raise ValueError('Incomplete or mismatched parity receipts')
    results=[]
    for fixture in manifest['fixtures']:
        results.append(score_fixture(fixture));print('scored '+fixture['id'],flush=True)
    output={'schema':'tempo-independent-replay-metrics.v1','scoring_frozen':manifest['scoring_frozen'],
            'qualification':'Descriptive host metrics; no new estimator acceptance threshold or visual preference claim',
            'source_sha256':sha(Path(__file__)),'manifest_sha256':sha(MANIFEST),
            'comparison_receipt_sha256':sha(receipt_path),'parity':parity,'fixtures':results,
            'interpretation_rules':['Same incumbent traces imply no improved detector accuracy from publication.',
                                    'Nonzero equal-level shape attribution proves a contribution reaches the scene, not preference.',
                                    'First lock/nearest-bin occurrence is descriptive and not stable acquisition qualification.',
                                    'Unannotated song-1 is held out; no tuning or musical accuracy label is produced.']}
    output['all_silence_lifecycle_checks_pass']=all(f['field']['silence_lifecycle_pass'] for f in results)
    args.output.write_text(json.dumps(output,indent=2,allow_nan=False)+'\n');print(args.output)
    write_report(output,args.output.with_suffix('.md'))
    return 0 if output['all_silence_lifecycle_checks_pass'] else 1

if __name__=='__main__':sys.exit(main())

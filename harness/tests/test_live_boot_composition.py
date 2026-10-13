#!/usr/bin/env python3
"""Host-only complete boot composition validation and actual setter readback."""
import copy,hashlib,json,subprocess,sys
from pathlib import Path
R=Path(__file__).resolve().parents[2];sys.path.insert(0,str(R/'scripts'))
from live_boot_composition import validate_composition,render_composition,literal,fields_from_schema,OUTPUT,PAIR
O=R/'_scratch/waveform-migration-001/boot-composition-host-001';O.mkdir(parents=True,exist_ok=True)
raw=(R/'docs/contracts/titan-emotiscope-v1.json').read_bytes();schema=json.loads(raw)
candidate=R/'docs/emotiscope-port-001/presets/UNSELECTED-sq5-live-defaults-v8.json'
d=json.loads(candidate.read_text());c=d['composition']
assert validate_composition(d,schema,raw)==c
(O/'UNSELECTED-sq5-live-defaults.json').write_text(json.dumps(d,indent=2)+'\n')
assert validate_composition({'mode_a':1,'mode_b':1,'brightness':255,'emit_default':'on','donor_palette':1},schema,raw) is None
# Execute the actual builder defaults branch without staging a BSP or compiling ARM.
import ast
from types import SimpleNamespace
builder=ast.parse((R/'scripts/build_scalar.py').read_text())
branch=next(n for n in ast.walk(builder) if isinstance(n,ast.If) and n.lineno>750 and isinstance(n.test,ast.Attribute) and n.test.attr=='live_defaults')
compiled=compile(ast.Module(body=[branch],type_ignores=[]),'actual-build-defaults-branch','exec')
stage=O/'stage';(stage/'src').mkdir(parents=True,exist_ok=True)
def build_defaults(path, **changes):
 args=SimpleNamespace(live_defaults=path,emotiscope_port=True,titan_top_output=True,palette_autostart=True,live_emit=True,palette_ws2816_gpt_pair=True)
 for key,value in changes.items():setattr(args,key,value)
 env=dict(args=args,json=json,hashlib=hashlib,schema=raw,stage=stage,defines=[],receipt={'emotiscope_port':{}},validate_composition=validate_composition,render_composition=render_composition)
 exec(compiled,env)
 return env
legacy=build_defaults(R/'docs/contracts/titan-emotiscope-spectrum-defaults-v1.json')
assert '-DK1_LIVE_BOOT_COMPOSITION=1' not in legacy['defines']
assert legacy['receipt']['emotiscope_port']['boot_donor_palette']==1
complete=build_defaults(candidate)
assert '-DK1_LIVE_BOOT_COMPOSITION=1' in complete['defines']
assert complete['receipt']['emotiscope_port']['boot_donor_palette']==[32,41]
assert (stage/'src/live_boot_composition.h').read_text()==render_composition(c)
for option in ('emotiscope_port','titan_top_output','palette_autostart','live_emit','palette_ws2816_gpt_pair'):
 try:build_defaults(candidate,**{option:False})
 except RuntimeError:pass
 else:raise AssertionError('missing required capability '+option)
rejections=0
for group in [('boards','0'),('boards','1'),('pair',),('output',)]:
 values=c
 for key in group: values=values[key]
 for field in values:
  for value in ('MISSING','invalid'):
   bad=copy.deepcopy(d);target=bad['composition']
   for key in group:target=target[key]
   if value=='MISSING':del target[field]
   else:target[field]=value
   try:validate_composition(bad,schema,raw)
   except RuntimeError:rejections+=1
   else:raise AssertionError((group,field,value))
for path,value in [(('boards','0','current_mode'),9),(('boards','1','palette'),0),(('boards','0','inhouse_mood'),float('nan')),(('boards','0','inhouse_sensitivity'),21),(('output','hardware_output_brightness'),99),(('output','emission_enabled'),False),(('output','period_us'),8333),(('profile_version',),7),(('schema_sha256',),'wrong'),(('boards','0','scene_initiator'),1), (('pair','unknown'),1)]:
 bad=copy.deepcopy(d);target=bad['composition']
 for key in path[:-1]:target=target[key]
 target[path[-1]]=value
 try:validate_composition(bad,schema,raw)
 except RuntimeError:rejections+=1
 else:raise AssertionError(path)
# Distinct non-default values ensure every authored field reaches its owner.
probe=copy.deepcopy(c)
fields=fields_from_schema(schema)
for board in range(2):
 for key,value in probe['boards'][str(board)].items():
  f=fields[key]
  if key=='current_mode':continue
  if f['kind']=='bool': value=not value
  elif f['kind']=='f32':value=f['min']+(f['max']-f['min'])*(.37 if board==0 else .61)
  else:value=f['max']-board
  probe['boards'][str(board)][key]=value
for key in ('scene_choreography','scene_initiator','scene_answer_delay_ms'):
 probe['boards']['1'][key]=probe['boards']['0'][key]
probe['pair'].update(pair_link=2,pair_colour_enabled=True,pair_colour_mode=3,pair_colour_strength=.7,pair_colour_spread=42.,pair_colour_spatial=1)
probe['output'].update(output_precision=True,output_intensity_a=.6,output_intensity_b=.7,primary_intensity=.8,secondary_intensity=.9)
pd=copy.deepcopy(d);pd['composition']=probe;validate_composition(pd,schema,raw)
(O/'live_boot_composition.h').write_text(render_composition(probe))
code=['#include "emotiscope_control.h"','#include "palette_runtime.h"','#include "k1_live_runtime.h"','#include "live_boot_composition.h"','#include <cassert>','#include <cstdio>','static bool admit(){return true;}','int main(){','assert(k1_emotiscope_init(1)); assert(k1_secondary_emotiscope_init(2));','static k1::titan::PaletteRuntime palettes; static k1::titan::LiveAudioRuntime audio;','k1::titan::PaletteConfig pc;pc.mode_a=pc.mode_b=1;pc.palette_a=pc.palette_b=0;pc.flags=5;assert(palettes.configure(pc,0));','assert(k1_apply_live_boot_composition(palettes,audio,admit));']
def check(expr,value):code.append(f'assert({expr}=={literal(value)});')
for board in range(2):
 b=probe['boards'][str(board)];pre='k1_secondary_emotiscope' if board else 'k1_emotiscope'
 code.append('{ K1EmotiscopeConfig cfg{};'+pre+'_get_config(&cfg);')
 for key in ('brightness','softness','warmth','speed','background','current_mode','mirror_mode','screensaver','temporal_dithering','auto_color_cycle','reverse_color_range'):check('cfg.'+key,b[key])
 for getter,key in [('palette','palette'),('response_gain','response_gain'),('edge_response','edge_response'),('edge_depth','edge_depth')]:check(pre+'_'+getter+'()',b[key])
 for typ,getter,mapping in [('K1ComposedSceneConfig','get_scene_config',{k:('scene' if k=='scene' else k[6:]) for k in b if k=='scene' or k.startswith('scene_')}),('K1EmotiscopeEdgeColourConfig','get_edge_colour',{'edge_colour_'+k:('spread_degrees' if k=='spread' else k) for k in ('enabled','mode','spatial','strength','spread')}),('K1EmotiscopeMusicalConfig','get_musical_config',{'musical_'+k:k for k in ('role','strength','attack_ms','release_ms')}),('K1EmotiscopeMlConfig','get_ml_config',{'ml_enabled':'enabled','ml_strength':'strength'})]:
  code.append('{ '+typ+' v{};'+pre+'_'+getter+'(&v);')
  for key,member in mapping.items():check('v.'+member,b[key])
  code.append('}')
 check(f'audio.inhouseMood({board})',b['inhouse_mood'])
 for key,member in [('inhouse_sensitivity','sensitivity'),('inhouse_square_iter','square_iter'),('inhouse_injection_photons','photons')]:check(f'palettes.inhouseParameters({board}).{member}',b[key])
 code.append('}')
code.append('K1EmotiscopeEdgePairConfig pair{};k1_emotiscope_get_edge_pair(&pair);')
for key,member in [('pair_link','link'),('pair_colour_enabled','colour.enabled'),('pair_colour_mode','colour.mode'),('pair_colour_spatial','colour.spatial'),('pair_colour_strength','colour.strength'),('pair_colour_spread','colour.spread_degrees')]:check('pair.'+member,probe['pair'][key])
for expr,key in [('palettes.config().brightness','hardware_output_brightness'),('palettes.emitEnabled()','emission_enabled'),('palettes.outputPrecision()','output_precision'),('palettes.secondaryOutputEnabled()','secondary_output_enabled'),('palettes.outputIntensity(0)','output_intensity_a'),('palettes.outputIntensity(1)','output_intensity_b'),('palettes.boardIntensity(0)','primary_intensity'),('palettes.boardIntensity(1)','secondary_intensity')]:check(expr,probe['output'][key])
code+=['assert(palettes.config().mode_a==32 && palettes.config().mode_b==18);', 'assert(!k1_apply_live_boot_composition(palettes,audio,+[](){return false;}));', 'assert(!palettes.secondaryOutputEnabled() && palettes.emitEnabled());', 'puts("all complete boot composition owning setter readbacks passed");','}']
(O/'consumer.cpp').write_text('\n'.join(code)+'\n')
sources=['platform/ra8p1/palette_runtime.cpp','platform/ra8p1/emotiscope_port.cpp','platform/ra8p1/emotiscope_secondary.cpp','platform/ra8p1/emotiscope_ml.cpp','platform/ra8p1/inhouse_audio.cpp','src/k1/core/audio/gdft_goertzel.cpp','src/k1/core/audio/tempo_tracker.cpp','src/k1/core/audio/tempo_acf.cpp','src/k1/core/visual/channel_render_state.cpp','src/k1/core/visual/audio_focus.cpp','src/k1/core/visual/product_palette.cpp','src/k1/core/visual/product_palette_data.cpp','src/k1/core/visual/inhouse_waveform.cpp']
cmd=['clang++','-std=c++17','-O1','-g','-Wno-narrowing','-fsanitize=address,undefined','-ffp-contract=off','-DK1_EMOTISCOPE_PORT=1','-DK1_TITAN_TOP_OUTPUT=1','-DK1_PALETTE_MORPH=1','-Isrc','-Isrc/k1','-Iplatform/ra8p1','-Iplatform/ra8p1/emotiscope_donor','-Iplatform/ra8p1/emotiscope_shims','-I'+str(O),str(O/'consumer.cpp'),*sources,'-o',str(O/'consumer')]
r=subprocess.run(cmd,cwd=R,text=True,capture_output=True);(O/'compile.log').write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr
result=subprocess.check_output([str(O/'consumer')],cwd=R,text=True)
fixture=(R/'platform/ra8p1/fixture_app.cpp').read_text();assert fixture.index('live_runtime.initialise(hz);')<fixture.index('k1_apply_live_boot_composition(palettes, live_runtime, secondary_admit)')<fixture.index('  live_ensure_boot_id();',fixture.index('extern "C" void k1_fixture_initialise'))
inputs=sources+[str(candidate.relative_to(R)),'scripts/live_boot_composition.py','scripts/build_scalar.py','platform/ra8p1/fixture_app.cpp',str(Path(__file__).relative_to(R)),'docs/contracts/titan-emotiscope-v1.json']
(O/'receipt.json').write_text(json.dumps({'scope':'host generated production boot consumer; unselected candidate only, no hardware','rejected_invalid':rejections,'readback':result,'command':cmd,'source_sha256':{x:hashlib.sha256((R/x).read_bytes()).hexdigest() for x in inputs}},indent=2)+'\n')
print(result.strip(),f'; invalid inputs rejected: {rejections}')

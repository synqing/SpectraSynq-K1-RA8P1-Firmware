#!/usr/bin/env python3
"""Compile the leased production conditioning helper; retain raw host evidence."""
import argparse
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = r"""
#include "musical_conditioning.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <initializer_list>
static const float hz[64]={FREQUENCIES};
static K1EmotiscopeAudioPublication input(unsigned seq=1,unsigned us=10000){
 K1EmotiscopeAudioPublication p{};p.sequence=seq;p.audio_hops=64+seq*2;p.now_us=us;p.valid=1;
 p.novelty_normalised=.5f;p.vu_level=.4f;p.tempi_phase[17]=.123f;
 p.tempi_smooth[4]=.4f;p.tempi_smooth[17]=.6f;p.tempi_power_sum=1;
 for(unsigned i=0;i<64;++i){p.spectrum[i]=.6f;p.spectrum_smooth[i]=.6f;p.frequency_magnitude[i]=.9f;}
 for(float &x:p.chromagram)x=.4f;return p;
}
static void setup(K1MusicalConditioning &c,unsigned role,float strength=1){
 assert(c.initialise_frequencies(hz));assert(c.configure({role,strength,40,160}));
}
int main(){
 assert(hz[0]==77.78175f && hz[63]==2959.956f);
 K1MusicalConditioning neutral;K1EmotiscopeAudioPublication p=input(),out{};
 assert(!neutral.apply(p,p,.01f,p.now_us));
 assert(!neutral.apply(p,out,.01f,p.now_us));assert(!std::memcmp(&p,&out,sizeof(p)));
 setup(neutral,1,0);assert(!neutral.apply(p,out,.01f,p.now_us));assert(!std::memcmp(&p,&out,sizeof(p)));
 assert(!neutral.configure({3,1,40,160}));assert(!neutral.configure({1,1,0,160}));
 assert(!neutral.configure({1,std::numeric_limits<float>::quiet_NaN(),40,160}));
 float badhz[64];std::memcpy(badhz,hz,sizeof(hz));badhz[7]=badhz[6];assert(!neutral.initialise_frequencies(badhz));
 K1MusicalConditioning body,detail;setup(body,1);setup(detail,2);
 auto before=p;K1EmotiscopeAudioPublication b{},d{};
 assert(body.apply(p,b,.01f,p.now_us));assert(detail.apply(p,d,.01f,p.now_us));
 assert(!std::memcmp(&p,&before,sizeof(p)));assert(b.sequence==p.sequence&&d.sequence==p.sequence);
 assert(b.vu_level!=p.vu_level&&d.tempi_phase[17]==p.tempi_phase[17]);
 // Independent analytical one-step spectral envelope; high detail weight=1.
 const float expected=.6f*(1-std::exp(-.01f/.04f));
 assert(std::fabs(d.spectrum[63]-expected)<1e-6f);
 assert(b.spectrum[0]/b.spectrum[63]>4.9f);
 assert(d.spectrum[63]/d.spectrum[0]>4.9f);
 assert(std::fabs(b.spectrum[0]/b.spectrum[63]-d.spectrum[0]/d.spectrum[63])>4);
 unsigned cursor=body.event_cursor();assert(cursor==1);auto first=b;
 assert(body.apply(p,b,0.0f,p.now_us));assert(body.event_cursor()==cursor);
 assert(!std::memcmp(&b,&first,sizeof(b)));
 assert(body.configure({1,.6f,40,160}));
 assert(!body.apply(p,b,.01f,p.now_us));assert(!std::memcmp(&b,&p,sizeof(p)));
 assert(body.event_cursor()==cursor);
 // Same elapsed time at distinct supported intervals converges to the same envelope.
 K1MusicalConditioning fast,slow;setup(fast,2);setup(slow,2);K1EmotiscopeAudioPublication f{},s{};
 for(unsigned i=1;i<=20;++i){auto q=input(i,i*5000);assert(fast.apply(q,f,.005f,q.now_us));}
 for(unsigned i=1;i<=10;++i){auto q=input(i,i*10000);assert(slow.apply(q,s,.01f,q.now_us));}
 assert(std::fabs(f.spectrum[63]-s.spectrum[63])<2e-6f);
 assert(std::fabs(s.spectrum[63]-.6f*(1-std::exp(-.1f/.04f)))<2e-6f);
 // Magnitude adaptation never amplifies a newly quiet raw bin to carry history.
 auto quieter=input(2,20000);for(float &x:quieter.spectrum)x=.00001f;
 assert(slow.apply(quieter,s,.01f,quieter.now_us));
 for(unsigned i=0;i<64;++i)assert(s.frequency_magnitude[i]<=quieter.frequency_magnitude[i]);
 // A steady rising energy ramp produces cadence-consistent positive flux.
 K1MusicalConditioning ramp5,ramp10;setup(ramp5,1);setup(ramp10,1);
 auto baseline=input(1,0);baseline.novelty_normalised=0;
 for(float &x:baseline.spectrum)x=0;
 assert(ramp5.apply(baseline,f,.005f,0));assert(ramp10.apply(baseline,s,.01f,0));
 for(unsigned i=1;i<=40;++i){auto q=input(i+1,i*5000);q.novelty_normalised=0;
  for(float &x:q.spectrum)x=float(i)*.005f;assert(ramp5.apply(q,f,.005f,q.now_us));}
 for(unsigned i=1;i<=20;++i){auto q=input(i+1,i*10000);q.novelty_normalised=0;
  for(float &x:q.spectrum)x=float(i)*.01f;assert(ramp10.apply(q,s,.01f,q.now_us));}
 assert(std::fabs(ramp5.value()-ramp10.value())<2e-6f);
 // A first steady observation is not a rise at any supported cadence.
 for(unsigned step: {5000u,10000u,20000u}){
  K1MusicalConditioning initial;setup(initial,1);auto q=input(1,step);q.novelty_normalised=0;
  for(float &x:q.spectrum)x=.015f;
  assert(initial.apply(q,f,float(step)*.000001f,step));assert(initial.event_cursor()==0);
 }
 // Distinct candidate histories change the relative tempo mixture, not all gain.
 auto tq=input(2,20000);tq.tempi_smooth[4]=.9f;tq.tempi_smooth[17]=.6f;
 assert(body.apply(tq,b,.01f,tq.now_us));assert(detail.apply(tq,d,.01f,tq.now_us));
 assert(std::fabs(b.tempi_smooth[4]/b.tempi_smooth[17]-d.tempi_smooth[4]/d.tempi_smooth[17])>.01f);
 // Release is also measured in time, and a true zero adds no cosmetic light.
 auto zero=input(21,105000);for(float &x:zero.spectrum)x=0;
 for(float &x:zero.spectrum_smooth)x=0;for(float &x:zero.chromagram)x=0;
 zero.novelty_normalised=0;assert(fast.apply(zero,f,.005f,zero.now_us));
 assert(std::fabs(f.spectrum[63]-(.6f*(1-std::exp(-.1f/.04f)))*std::exp(-.005f/.16f))<2e-6f);
 K1MusicalConditioning fresh_zero;setup(fresh_zero,2);assert(fresh_zero.apply(zero,f,.01f,zero.now_us));
 for(float x:f.spectrum)assert(x==0);for(float x:f.chromagram)assert(x==0);
 // Explicit capture-gap reset keeps user settings and event identity.
 const auto saved=body.config();const auto saved_cursor=body.event_cursor();body.reset();
 assert(body.config().role==saved.role&&body.config().strength==saved.strength);
 assert(body.value()==0&&body.event_cursor()==saved_cursor);
 // Repeated cached result still expires against real frame time.
 assert(!detail.apply(p,d,.01f,p.now_us+100001));assert(!std::memcmp(&p,&d,sizeof(p)));
 auto invalid=p;invalid.valid=0;assert(!body.apply(invalid,b,.01f,p.now_us));
 assert(body.value()==0);assert(body.event_cursor()==cursor);
 auto nonfinite=p;nonfinite.spectrum[4]=std::numeric_limits<float>::infinity();
 assert(!detail.apply(nonfinite,d,.01f,p.now_us));assert(detail.value()==0);
 // New publication timestamps cannot hide a frozen PCM producer.
 K1MusicalConditioning stalled;setup(stalled,1);auto frozen=input();
 assert(stalled.apply(frozen,b,.01f,frozen.now_us));
 frozen.sequence=2;frozen.now_us+=100001;
 assert(!stalled.apply(frozen,b,.01f,frozen.now_us));assert(stalled.value()==0);
 // Regression and gap reinitialise envelope, without rewriting publication chronology.
 auto restart=input(1,400000);restart.audio_hops=200;assert(detail.apply(restart,d,.01f,restart.now_us));
 assert(std::fabs(d.spectrum[63]-expected)<1e-6f);
 auto gap=input(2,700000);gap.audio_hops=202;assert(detail.apply(gap,d,.01f,gap.now_us));
 assert(std::fabs(d.spectrum[63]-expected)<1e-6f);
 // Legitimate 32-bit clock wrap; generation is independently wrap-safe.
 K1MusicalConditioning wrap;setup(wrap,2);auto w=input(0xfffffffeu,0xfffff000u);
 assert(wrap.apply(w,d,.01f,w.now_us));w=input(0xffffffffu,5904u);assert(wrap.apply(w,d,.01f,w.now_us));
 // Two histories are independent under A-only control edits and reversed call order.
 K1MusicalConditioning a1,a2,c1,c2;setup(a1,1);setup(a2,1);setup(c1,2);setup(c2,2);
 for(unsigned i=1;i<50;++i){auto q=input(i,i*10000);q.spectrum[0]=i%4?.15f:.9f;
  K1EmotiscopeAudioPublication a{},a_ref{},c{},c_ref{};
  assert(a1.apply(q,a,.01f,q.now_us));assert(c1.apply(q,c,.01f,q.now_us));
  assert(c2.apply(q,c_ref,.01f,q.now_us));assert(a2.apply(q,a_ref,.01f,q.now_us));
  assert(!std::memcmp(&c,&c_ref,sizeof(c)));if(i==20)assert(a1.configure({2,.7f,20,300}));
 }
 std::printf("conditioning analytical/identity/contour/time/freshness/independence checks passed; object=%zu bytes\n",sizeof(K1MusicalConditioning));
}
"""

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    donor = (ROOT/'platform/ra8p1/emotiscope_donor/goertzel.h').read_text()
    notes = [float(x) for x in re.search(r'const float notes\[\] = \{(.*?)\};', donor, re.S).group(1).split(',') if x.strip()]
    frequencies = [notes[12+2*i] for i in range(64)]
    source = args.output/'conditioning.cpp'
    source.write_text(CPP.replace('FREQUENCIES', ','.join(f'{x}f' for x in frequencies)))
    binary = args.output/'conditioning'
    command = ['/usr/bin/clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-Wdouble-promotion','-Werror=double-promotion','-fno-fast-math','-fsanitize=address,undefined','-I'+str(ROOT/'platform/ra8p1'),str(source),'-o',str(binary)]
    compiled = subprocess.run(command, text=True, capture_output=True)
    (args.output/'compile.log').write_text(compiled.stdout+compiled.stderr)
    if compiled.returncode:
        print(compiled.stderr)
        return compiled.returncode
    result = subprocess.run([str(binary)], text=True, capture_output=True)
    (args.output/'run.log').write_text(result.stdout+result.stderr)
    print(result.stdout+result.stderr)
    if result.returncode:
        return result.returncode
    # A gain-only replacement must fail the independently specified contour
    # and analytical-envelope checks. Keep the mutation outside production.
    mutation_dir = args.output/'gain-only-mutation'
    mutation_dir.mkdir()
    header = (ROOT/'platform/ra8p1/musical_conditioning.h').read_text()
    mutation = header.replace(
        'out.spectrum[i]=raw.spectrum[i]+strength*(shaped-raw.spectrum[i]);',
        'out.spectrum[i]=raw.spectrum[i]*(1.0f-strength+strength*value_);')
    if mutation == header:
        raise RuntimeError('gain-only mutation site not found')
    (mutation_dir/'musical_conditioning.h').write_text(mutation)
    mutated_source = mutation_dir/'conditioning.cpp'
    mutated_source.write_text(source.read_text())
    mutated_binary = mutation_dir/'conditioning'
    mutated_command = list(command)
    mutated_command[mutated_command.index(str(source))] = str(mutated_source)
    mutated_command[mutated_command.index(str(binary))] = str(mutated_binary)
    compiled = subprocess.run(mutated_command, text=True, capture_output=True)
    (mutation_dir/'compile.log').write_text(compiled.stdout+compiled.stderr)
    if compiled.returncode:
        print(compiled.stderr)
        return compiled.returncode
    negative = subprocess.run([str(mutated_binary)], text=True, capture_output=True)
    (mutation_dir/'run.log').write_text(negative.stdout+negative.stderr)
    if negative.returncode == 0:
        raise RuntimeError('gain-only mutation escaped the contour oracle')
    print('gain-only mutation rejected by actual contour/envelope oracle')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())

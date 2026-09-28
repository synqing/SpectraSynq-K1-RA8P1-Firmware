#!/usr/bin/env python3
"""THE TITAN MUST ALWAYS BE VISUALISING MUSIC. Local C++ authoring loop; no hardware I/O."""
import argparse
import hashlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import subprocess
import threading
import urllib.parse
import wave
from build_ribbon_replay import ROOT, build, sha

FIXTURES=ROOT/'tests/fixtures/tempo_field/v1/pcm'
ALLOWED=['synthetic','song-0','sparse_reentry','clear120','dense','fractional12743','half_time','silence','syncopation','tempo_step']
DEFAULTS=dict(velocity=64,spread=.12,lifetime=1.6,width=2,spacing=.70,gap=.32,
              return_strength=.24,injection=1.6,persistence=0,palette=33,recolour=0,knee=0,liveiness=.5,pair_width=7,pair_velocity=40,pair_delay=.18,pair_articulation=.7,pair_spread=0,pair_lifetime=2.8,pair_gap=.32,pair_return=.22)
RANGES=dict(velocity=(4,240),spread=(0,30),lifetime=(.1,12),width=(.5,10),spacing=(.08,3),gap=(.05,2),
 return_strength=(0,.45),injection=(0,3),persistence=(0,.15),palette=(0,43),recolour=(0,1),knee=(0,1),liveiness=(0,1),
 pair_width=(3,14),pair_velocity=(4,80),pair_delay=(0,.5),pair_articulation=(0,1),pair_spread=(0,10),pair_lifetime=(.2,8),pair_gap=(.05,2),pair_return=(0,.45))
LOCK=threading.Lock()

def validate_build(exe):
    receipt=json.loads((exe.parent/'build-receipt.json').read_text())
    if sha(exe)!=receipt['executable_sha256']:raise ValueError('Executable differs from its build receipt')
    for name,digest in receipt['sources'].items():
        if sha(ROOT/name)!=digest:raise ValueError('Rebuild required: source changed: '+name)

def export(exe, folder, request):
    fixture=request.get('fixture','song-0')
    if fixture not in ALLOWED: raise ValueError('Unknown or held-out fixture')
    settings=dict(DEFAULTS)
    supplied=request.get('settings',{})
    if set(supplied)-settings.keys(): raise ValueError('Unknown settings')
    settings.update(supplied)
    for name,value in settings.items():
        lo,hi=RANGES[name]
        if type(value) not in (int,float) or not lo<=value<=hi:raise ValueError('Invalid parameter: '+name)
        if name in ('palette','recolour','knee') and int(value)!=value:raise ValueError('Integer required: '+name)
    pcm=FIXTURES/(fixture+'.pcm')
    identity=dict(fixture=fixture,input_sha256=sha(pcm) if fixture!='synthetic' else 'declared-six-studies-v1',
                  settings=settings,executable_sha256=sha(exe))
    key=hashlib.sha256(json.dumps(identity,sort_keys=True).encode()).hexdigest()[:20]
    destination=folder/key;prefix=destination/'frames'
    with LOCK:
        if not (destination/'identity.json').exists():
            destination.mkdir(parents=True,exist_ok=False)
            command=[str(exe),'--output',str(prefix)]
            command+=['--synthetic','1'] if fixture=='synthetic' else ['--pcm',str(pcm)]
            for k,v in settings.items():command+=['--'+('return' if k=='return_strength' else k),str(v)]
            result=subprocess.run(command,capture_output=True,text=True,timeout=180)
            (destination/'replay.log').write_text(result.stdout+result.stderr)
            if result.returncode:raise RuntimeError(result.stderr)
            if fixture!='synthetic':
                with wave.open(str(destination/'audio.wav'),'wb') as audio:
                    audio.setnchannels(1);audio.setsampwidth(2);audio.setframerate(24000);audio.writeframes(pcm.read_bytes())
            identity.update(command=command,files={p.name:sha(p) for p in destination.iterdir() if p.is_file()},physical_execution=False)
            (destination/'identity.json').write_text(json.dumps(identity,indent=2)+'\n')
    return dict(key=key,base='/data/'+key+'/',identity=identity)

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--build',type=Path);parser.add_argument('--port',type=int,default=8769)
    args=parser.parse_args();folder=args.output.resolve();folder.mkdir(parents=True,exist_ok=True)
    exe=args.build.resolve() if args.build else build(folder/'build')
    validate_build(exe)
    class Handler(BaseHTTPRequestHandler):
        def response(self,data,kind='application/json',status=200):
            self.send_response(status);self.send_header('Content-Type',kind);self.send_header('Content-Length',str(len(data)))
            self.send_header('Cache-Control','no-store');self.end_headers();self.wfile.write(data)
        def do_GET(self):
            path=urllib.parse.unquote(urllib.parse.urlparse(self.path).path)
            if path=='/paired': self.response((ROOT/'tools/audio/paired_audition.html').read_bytes(),'text/html; charset=utf-8')
            elif path=='/': self.response((ROOT/'tools/audio/ribbon_audition.html').read_bytes(),'text/html; charset=utf-8')
            elif path=='/previous-paired-song0.rgb16le':
                self.response((ROOT/'out/paired-hierarchy-verification-005/playback/240f70e075a2dfaeb0f9/frames.paired.rgb16le').read_bytes(),'application/octet-stream')
            elif path=='/rejected-paired-song0.rgb16le':
                self.response((ROOT/'out/paired-review-001/f74edeb5bd5c8607d7ab/frames.paired.rgb16le').read_bytes(),'application/octet-stream')
            elif path=='/rejected-song0.rgb16le':
                self.response((ROOT/'out/ribbon-review-001/1ae3a094fefbf0fe4921/frames.rgb16le').read_bytes(),'application/octet-stream')
            elif path=='/config':self.response(json.dumps(dict(defaults=DEFAULTS,fixtures=ALLOWED)).encode())
            elif path.startswith('/data/'):
                file=(folder/path[6:]).resolve()
                if not file.is_relative_to(folder) or not file.is_file():self.send_error(404);return
                kind='audio/wav' if file.suffix=='.wav' else 'application/json' if file.suffix=='.json' else 'application/octet-stream'
                data=file.read_bytes()
                if kind=='audio/wav' and self.headers.get('Range','').startswith('bytes='):
                    bounds=self.headers['Range'][6:].split('-',1)
                    try:
                        start=int(bounds[0]);end=min(len(data)-1,int(bounds[1]) if bounds[1] else len(data)-1)
                        if not 0<=start<=end<len(data):raise ValueError()
                    except ValueError:self.send_error(416);return
                    self.send_response(206);self.send_header('Content-Type',kind)
                    self.send_header('Accept-Ranges','bytes');self.send_header('Content-Range',f'bytes {start}-{end}/{len(data)}')
                    self.send_header('Content-Length',str(end-start+1));self.end_headers();self.wfile.write(data[start:end+1])
                else:self.response(data,kind)
            else:self.send_error(404)
        def do_POST(self):
            if self.path!='/render':self.send_error(404);return
            try:
                size=int(self.headers.get('Content-Length',0))
                if not 0<size<8192:raise ValueError('Request size')
                validate_build(exe)
                result=export(exe,folder,json.loads(self.rfile.read(size)))
                self.response(json.dumps(result).encode())
            except Exception as error:self.response(json.dumps(dict(error=str(error))).encode(),status=400)
    print(f'RIBBON_AUDITION http://127.0.0.1:{args.port}/ executable={exe}',flush=True)
    ThreadingHTTPServer(('127.0.0.1',args.port),Handler).serve_forever()

if __name__=='__main__':main()

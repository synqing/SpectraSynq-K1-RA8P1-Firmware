#!/usr/bin/env python3
"""THE TITAN MUST ALWAYS BE VISUALISING MUSIC. Freeze actual C++ frames and PCM into one offline HTML."""
import argparse
import base64
import gzip
import json
from pathlib import Path
from build_ribbon_replay import ROOT,sha

def package(folder,output):
    identity=json.loads((folder/'identity.json').read_text())
    assert identity['fixture']=='song-0','This comparison package reserves song-1'
    for name,digest in identity['files'].items():
        assert sha(folder/name)==digest,'Export drift: '+name
    rejected=ROOT/'out/paired-review-001/f74edeb5bd5c8607d7ab/frames.paired.rgb16le'
    files={name:base64.b64encode(gzip.compress((folder/name).read_bytes(),mtime=0)).decode()
           for name in ['frames.json','frames.paired.rgb16le','frames.paired.matched.rgb16le']}
    previous=ROOT/'out/paired-retained-verification-004/playback/a7345e2824f27903183b/frames.paired.rgb16le'
    files['previous']=base64.b64encode(gzip.compress(previous.read_bytes(),mtime=0)).decode()
    files['rejected']=base64.b64encode(gzip.compress(rejected.read_bytes(),mtime=0)).decode()
    manifest=dict(defaults=identity['settings'],fixtures=['song-0'])
    receipt=dict(key=folder.name,base='embedded/',identity=identity)
    loader='''
const frozenFiles=%s;
const frozenConfig=%s;
const frozenReceipt=%s;
const decoded=new Map();
async function artifactFetch(path,options){
 if(path==='/config')return new Response(JSON.stringify(frozenConfig));
 if(path==='/render')return new Response(JSON.stringify(frozenReceipt));
 const name=path==='/previous-paired-song0.rgb16le'?'previous':path==='/rejected-paired-song0.rgb16le'?'rejected':path.replace('embedded/','');
 if(!frozenFiles[name])throw Error('Absent packaged asset: '+path);
 if(!decoded.has(name)){
  const bytes=Uint8Array.from(atob(frozenFiles[name]),c=>c.charCodeAt(0));
  decoded.set(name,new Response(new Blob([bytes]).stream().pipeThrough(new DecompressionStream('gzip'))).arrayBuffer());
 }
 return new Response(await decoded.get(name));
}
'''%(json.dumps(files),json.dumps(manifest),json.dumps(receipt))
    html=(ROOT/'tools/audio/paired_audition.html').read_text()
    html=html.replace('<script>','<script>'+loader,1).replace("fetch('/","artifactFetch('/").replace("fetch(b+","artifactFetch(b+")
    audio='data:audio/wav;base64,'+base64.b64encode((folder/'audio.wav').read_bytes()).decode()
    html=html.replace("$('audio').src=b+'audio.wav'","$('audio').src="+json.dumps(audio))
    html=html.replace("finally{$('render').disabled=false}","finally{$('render').disabled=true}")
    html=html.replace('label.append(input);','input.disabled=true;label.append(input);')
    html=html.replace('Render C++ [R]','Frozen C++ frames').replace('changes use the same C++ renderer','frozen settings for this export')
    html=html.replace('href="/"','href="http://127.0.0.1:8769/"')
    html=html.replace('C++ final frames · 320 LEDs','Offline package · embedded original PCM and C++ final frames · 320 LEDs')
    output.parent.mkdir(parents=True,exist_ok=True)
    if output.exists():raise FileExistsError(output)
    output.write_text(html)
    output.with_suffix('.json').write_text(json.dumps(dict(schema='paired-audition-package.v1',html_sha256=sha(output),
        export_identity=identity,previous_frames_sha256=sha(previous),rejected_frames_sha256=sha(rejected),browser_transfer='shared exposure6168 ->255; illustrative vertical blend only',
        frames_embedded=True,audio_embedded=True,backend_required=False,appearance_accepted=False),indent=2)+'\n')
    print(output)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--export',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    args=p.parse_args();package(args.export.resolve(),args.output.resolve())

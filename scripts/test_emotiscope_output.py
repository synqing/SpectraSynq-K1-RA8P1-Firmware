#!/usr/bin/env python3
"""Production Emotiscope geometry, continuous-DIN packing, and output-path checks."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--output",required=True,type=Path)
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[1]
    plat=root/"platform/ra8p1"
    out=args.output.resolve()
    out.mkdir(parents=True,exist_ok=False)
    records={"scope":"Host output conformance only; no target transmission or optical observation",
             "defines":["K1_EMOTISCOPE_PORT=1","K1_PALETTE_MORPH=1"],
             "cases":[]}
    files=[plat/n for n in ["palette_runtime.cpp","palette_runtime.h","emotiscope_port.cpp",
        "emotiscope_port.h","emotiscope_output.h","emotiscope_quantize.inc","hd_pixel16.h"]]
    files+=list((plat/"emotiscope_donor").rglob("*.h"))
    files+=list((plat/"emotiscope_shims").rglob("*"))
    files+=list((root/"src/k1").rglob("*.h"))+list((root/"src/k1").rglob("*.cpp"))
    files += [root/"tests/host/test_emotiscope_output.cpp",Path(__file__).resolve()]
    files=sorted(set(p for p in files if p.is_file()))
    before={str(p.relative_to(root)):sha(p) for p in files}
    records["source_sha256"]=before

    def run(name,command):
        p=subprocess.run([str(s) for s in command],capture_output=True,text=True)
        log=out/(name+".log")
        log.write_text(p.stdout+p.stderr)
        item={"name":name,"command":[str(s) for s in command],
              "exit":p.returncode,"log":str(log)}
        if p.returncode==0 and p.stdout.strip().startswith("{"):
            item["observed"]=json.loads(p.stdout)
        records["cases"].append(item)
        return p,item

    def build(platform,name):
        binary=out/name
        cmd=["/usr/bin/clang++","-std=c++17","-O1","-g","-Wno-narrowing",
             "-fsanitize=address,undefined","-fno-omit-frame-pointer",
             "-DK1_EMOTISCOPE_PORT=1","-DK1_PALETTE_MORPH=1",
             "-I"+str(root/"src/k1"),"-I"+str(platform),"-I"+str(plat),
             "-I"+str(platform/"emotiscope_donor"),"-I"+str(platform/"emotiscope_shims"),
             str(root/"tests/host/test_emotiscope_output.cpp"),
             str(platform/"palette_runtime.cpp"),str(platform/"emotiscope_port.cpp"),
             str(root/"src/k1/core/visual/channel_render_state.cpp"),
             str(root/"src/k1/core/visual/audio_focus.cpp"),"-o",str(binary)]
        result,_=run(name+"-compile",cmd)
        if result.returncode: raise RuntimeError("Compile failed for "+name)
        return binary

    success=True
    try:
        binary=build(plat,"output")
        marker,_=run("markers",[binary,"markers"])
        if marker.returncode: raise RuntimeError("Physical-index marker contract failed")
        direct=out/"direct-native128-rgb8.bin"
        actual=out/"runtime-continuous160-grb48.bin"
        a,_=run("direct",[binary,"direct",direct])
        b,_=run("runtime",[binary,"runtime",actual])
        if a.returncode or b.returncode: raise RuntimeError("Independent-process generation failed")
        raw=direct.read_bytes()
        packed=actual.read_bytes()
        if len(raw)!=1000*384 or len(packed)!=1000*960:
            raise RuntimeError("Unexpected frame count or output geometry")
        # Nearest native pixel by geometric distance, independent from adapter arithmetic.
        mapping=[min(range(128),key=lambda source:abs(source/127-destination/159))
                 for destination in range(160)]
        expected=bytearray()
        for frame in range(1000):
            for destination in range(160):
                index=frame*384+mapping[destination]*3
                for color in [1,0,2]:
                    code=raw[index+color]*65535*100//(255*255)
                    expected+=code.to_bytes(2,"big")
        expected_path=out/"independent-expected-continuous160-grb48.bin"
        expected_path.write_bytes(expected)
        same=packed==expected
        item={"name":"direct-donor-to-production-packer","exit":0 if same else 3,
              "independent_processes":True,"frames":1000,"wire_bytes_compared":len(expected),
              "old_product_controls_poisoned":True,"old_audio_features_poisoned":True,
              "direct_sha256":sha(direct),"actual_sha256":sha(actual),
              "expected_sha256":sha(expected_path),"all_wire_bytes_equal":same}
        if not same:
            index=next(i for i,(a,b) in enumerate(zip(packed,expected)) if a!=b)
            item["first_difference"]={"byte":index,"frame":index//960,
                                       "actual":packed[index],"expected":expected[index]}
        records["cases"].append(item)
        success &= same

        for name in ["wrong-lane-split","double-global-brightness","second-gamma"]:
            mutant=out/name
            mutant.mkdir()
            for filename in ["palette_runtime.cpp","palette_runtime.h","emotiscope_port.cpp",
                             "emotiscope_port.h","emotiscope_output.h","emotiscope_quantize.inc"]:
                shutil.copy2(plat/filename,mutant/filename)
            shutil.copytree(plat/"emotiscope_donor",mutant/"emotiscope_donor")
            shutil.copytree(plat/"emotiscope_shims",mutant/"emotiscope_shims")
            if name=="wrong-lane-split":
                p=mutant/"palette_runtime.cpp";text=p.read_text()
                old="source[lane * kPixelsPerHalf + i]"
                if text.count(old)!=1: raise RuntimeError("Cannot isolate lane mutation")
                p.write_text(text.replace(old,"source[i]"))
            elif name=="double-global-brightness":
                p=mutant/"palette_runtime.cpp";text=p.read_text()
                for color in ["red","green","blue"]:
                    old="scale16(pixel."+color+", config_.brightness)"
                    if text.count(old)!=1: raise RuntimeError("Cannot isolate gain mutation")
                    text=text.replace(old,"scale16("+old+", config_.brightness)")
                p.write_text(text)
            else:
                p=mutant/"emotiscope_output.h";text=p.read_text()
                for channel in ["p","p+1U","p+2U"]:
                    old="rgb["+channel+"] * 257U"
                    if text.count(old)!=1: raise RuntimeError("Cannot isolate gamma mutation")
                    text=text.replace(old,"unsigned(rgb["+channel+"]) * rgb["+channel+"] * 257U / 255U")
                p.write_text(text)
            exe=build(mutant,name+"-binary")
            result,item=run(name,[exe,"markers"])
            rejected=result.returncode!=0 and "OUTPUT_CONTRACT_FAILURE" in result.stderr
            item["expected_rejection_observed"]=rejected
            success &= rejected
        records["sources_unchanged_during_test"]=before=={
            str(p.relative_to(root)):sha(p) for p in files}
        success &= records["sources_unchanged_during_test"]
    except Exception as error:
        records["error"]=str(error)
        success=False
    records["host_output_checks_satisfied"]=success
    (out/"result.json").write_text(json.dumps(records,indent=2)+"\n")
    print(json.dumps({"host_output_checks_satisfied":success,"result":str(out/"result.json"),
        "cases":[{"name":x["name"],"exit":x["exit"]} for x in records["cases"]],
        "error":records.get("error")},indent=2))
    return 0 if success else 1
if __name__=="__main__":
    sys.exit(main())

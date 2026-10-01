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
        harness=platform/"test_emotiscope_output.cpp"
        if not harness.is_file(): harness=root/"tests/host/test_emotiscope_output.cpp"
        cmd=["/usr/bin/clang++","-std=c++17","-O1","-g","-Wno-narrowing",
             "-fsanitize=address,undefined","-fno-omit-frame-pointer",
             "-DK1_EMOTISCOPE_PORT=1","-DK1_PALETTE_MORPH=1","-DK1_EMOTISCOPE_TEST_SEAM=1",
             "-I"+str(root/"src/k1"),"-I"+str(platform),"-I"+str(plat),
             "-I"+str(platform/"emotiscope_donor"),"-I"+str(platform/"emotiscope_shims"),
             str(harness),
             str(platform/"palette_runtime.cpp"),str(platform/"emotiscope_port.cpp"),
             str(root/"src/k1/core/visual/channel_render_state.cpp"),
             str(root/"src/k1/core/visual/audio_focus.cpp"),
             str(root/"src/k1/core/visual/product_palette.cpp"),
             str(root/"src/k1/core/visual/product_palette_data.cpp"),"-o",str(binary)]
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

        # --- Output precision A/B -------------------------------------------------
        # (1) Reference untouched: precision 0 output equals the tagged reference build's.
        reference_tag = "titan-emotiscope-ref-r1"
        ref_src = out/"reference-source"
        ref_src.mkdir()
        for filename in ["palette_runtime.cpp","palette_runtime.h","emotiscope_port.cpp",
                         "emotiscope_port.h","emotiscope_output.h","emotiscope_quantize.inc"]:
            blob = subprocess.run(["git","-C",str(root),"show",f"{reference_tag}:platform/ra8p1/{filename}"],
                                  capture_output=True, check=True).stdout
            (ref_src/filename).write_bytes(blob)
        (ref_src/"test_emotiscope_output.cpp").write_bytes(subprocess.run(
            ["git","-C",str(root),"show",f"{reference_tag}:tests/host/test_emotiscope_output.cpp"],
            capture_output=True, check=True).stdout)
        shutil.copytree(plat/"emotiscope_donor",ref_src/"emotiscope_donor")
        shutil.copytree(plat/"emotiscope_shims",ref_src/"emotiscope_shims")
        ref_bin = build(ref_src,"reference-tag")
        ref_wire = out/"reference-tag-runtime-grb48.bin"
        r,_ = run("reference-tag-runtime",[ref_bin,"runtime",ref_wire])
        if r.returncode: raise RuntimeError("Reference-tag generation failed")
        identical = ref_wire.read_bytes()==packed
        records["cases"].append({"name":"precision0-equals-reference-tag","exit":0 if identical else 3,
            "reference_tag":reference_tag,"frames":1000,"wire_bytes_compared":len(packed),
            "reference_sha256":sha(ref_wire),"precision0_sha256":sha(actual),"identical":identical})
        success &= identical

        # (2) Candidate: every wire value equals the rounded, mapped, once-gained float.
        floats_path = out/"direct-post-gamma-f32.bin"
        cand_path = out/"runtime16-continuous160-grb48.bin"
        a,_ = run("direct16",[binary,"direct16",floats_path])
        b,_ = run("runtime16",[binary,"runtime16",cand_path])
        if a.returncode or b.returncode: raise RuntimeError("Candidate generation failed")
        import struct
        floats = struct.unpack("<%df" % (1000*384), floats_path.read_bytes())
        f32 = lambda x: struct.unpack("<f", struct.pack("<f", x))[0]
        def round16(v):
            # IEEE single: v*65535 rounded to float, +0.5 rounded to float, truncate.
            return int(f32(f32(v*65535.0)+0.5))
        def gain(code, brightness=100):
            return 0 if code==0 else (code*brightness)//255
        cand_expected = bytearray()
        for frame in range(1000):
            for destination in range(160):
                base = frame*384 + mapping[destination]*3
                for color in [1,0,2]:
                    cand_expected += gain(round16(floats[base+color])).to_bytes(2,"big")
        cand = cand_path.read_bytes()
        cand_same = cand==bytes(cand_expected)
        item={"name":"precision1-rounded-mapped-once-gained","exit":0 if cand_same else 3,
              "frames":1000,"wire_bytes_compared":len(cand_expected),"all_wire_bytes_equal":cand_same,
              "din_split_pixels":[80,80],"endpoints_and_centre_from_same_mapping":True}
        if not cand_same:
            index=next(i for i,(x,y) in enumerate(zip(cand,cand_expected)) if x!=y)
            item["first_difference"]={"byte":index,"frame":index//960,"actual":cand[index],"expected":cand_expected[index]}
        records["cases"].append(item)
        success &= cand_same

        # (3) Reported numbers, not pass/fail.
        ref_codes = struct.unpack(">%dH" % (len(packed)//2), packed)
        cand_codes = struct.unpack(">%dH" % (len(cand)//2), cand)
        diffs = [abs(x-y) for x,y in zip(ref_codes,cand_codes)]
        f,_ = run("fade",[binary,"fade"])
        if f.returncode: raise RuntimeError("Fade fixture failed")
        records["precision_numbers"]={
            "music_fixture":"2000 hops synthetic 110 Hz + 440/659 Hz tones, mode 1 Spectrum, output 100/255",
            "mean_abs_wire_difference_16bit":sum(diffs)/len(diffs),
            "max_abs_wire_difference_16bit":max(diffs),
            "fraction_values_differing":sum(1 for d in diffs if d)/len(diffs),
            "dim_fade":json.loads(f.stdout)}

        # (4) Palette hook reaches the wire: palette 25 output must differ from native.
        pal_path = out/"runtime-palette25-grb48.bin"
        pr,_ = run("runtimeP",[binary,"runtimeP",pal_path])
        if pr.returncode: raise RuntimeError("Palette generation failed")
        pal = pal_path.read_bytes()
        differing = sum(1 for x,y in zip(pal,packed) if x!=y)
        pal_ok = len(pal)==len(packed) and differing > len(packed)//20
        records["cases"].append({"name":"palette25-changes-wire","exit":0 if pal_ok else 3,
                                 "bytes_differing_from_native":differing,"wire_bytes":len(packed)})
        success &= pal_ok

        # (5) Palettes 1-44 draw as K1 did (independent oracle in the harness), and
        # three planted faults must each be rejected.
        kp,kitem = run("k1-palette-drawing",[binary,"k1palette"])
        success &= kp.returncode==0
        for name,old,new in [
            ("palette-keeps-gamma","if(!k1_palette_active())donor_apply_gamma_correction();","donor_apply_gamma_correction();"),
            ("palette-uses-colour-range",":base+progress;",":base+configuration.color_range*progress;"),
            ("palette-linear-level","const float level=0.65f*v*v+0.35f*v;","const float level=v;"),
            ("palette-colour-shifts","const float base=configuration.auto_color_cycle?configuration.color:0.0f;","const float base=configuration.color;"),
            ("background-native-hsv","if(!k1_palette_active()){donor_apply_background(background_level);return;}","{donor_apply_background(background_level);return;}"),
            ("tonemap-per-channel","if(!k1_palette_active()){donor_apply_tonemapping();return;}","{donor_apply_tonemapping();return;}"),
            ("screensaver-fixed-colours","if(!k1_palette_active()){donor_run_screensaver();return;}","{donor_run_screensaver();return;}"),
            ("hype-range-unpinned","configuration.color_range=1.0f;k1_draw_scope=true;","k1_draw_scope=true;")]:
            mutant=out/name
            mutant.mkdir()
            for filename in ["palette_runtime.cpp","palette_runtime.h","emotiscope_port.cpp",
                             "emotiscope_port.h","emotiscope_output.h","emotiscope_quantize.inc"]:
                shutil.copy2(plat/filename,mutant/filename)
            shutil.copytree(plat/"emotiscope_donor",mutant/"emotiscope_donor")
            shutil.copytree(plat/"emotiscope_shims",mutant/"emotiscope_shims")
            mp=mutant/"emotiscope_port.cpp";text=mp.read_text()
            if text.count(old)!=1: raise RuntimeError("Cannot isolate "+name)
            mp.write_text(text.replace(old,new))
            exe=build(mutant,name+"-binary")
            r,mitem=run(name,[exe,"k1palette"])
            rejected=r.returncode!=0 and "OUTPUT_CONTRACT_FAILURE" in r.stderr
            mitem["expected_rejection_observed"]=rejected
            success &= rejected

        for name in ["candidate-truncates","candidate-double-gain"]:
            mutant=out/name
            mutant.mkdir()
            for filename in ["palette_runtime.cpp","palette_runtime.h","emotiscope_port.cpp",
                             "emotiscope_port.h","emotiscope_output.h","emotiscope_quantize.inc"]:
                shutil.copy2(plat/filename,mutant/filename)
            shutil.copytree(plat/"emotiscope_donor",mutant/"emotiscope_donor")
            shutil.copytree(plat/"emotiscope_shims",mutant/"emotiscope_shims")
            if name=="candidate-truncates":
                p=mutant/"emotiscope_port.cpp";text=p.read_text();old="p[i]*65535.0f+0.5f"
                if text.count(old)!=1: raise RuntimeError("Cannot isolate rounding mutation")
                p.write_text(text.replace(old,"p[i]*65535.0f"))
            else:
                p=mutant/"emotiscope_output.h";text=p.read_text();old="return {rgb16[p], rgb16[p+1U], rgb16[p+2U]};"
                if text.count(old)!=1: raise RuntimeError("Cannot isolate candidate gain mutation")
                p.write_text(text.replace(old,"return {std::uint16_t(rgb16[p]*100U/255U), std::uint16_t(rgb16[p+1U]*100U/255U), std::uint16_t(rgb16[p+2U]*100U/255U)};"))
            exe=build(mutant,name+"-binary")
            mut_path=out/(name+".bin")
            r,mitem=run(name,[exe,"runtime16",mut_path])
            rejected = r.returncode!=0 or mut_path.read_bytes()!=bytes(cand_expected)
            mitem["expected_rejection_observed"]=rejected
            success &= rejected

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

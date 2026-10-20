#!/usr/bin/env python3
"""Production Emotiscope geometry, continuous-DIN packing, and output-path checks."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile

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
    files+=list(plat.glob("*.h"))+list(plat.glob("*.inc"))
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

    def build(platform,name,source_root=root):
        binary=out/name
        harness=platform/"test_emotiscope_output.cpp"
        if not harness.is_file(): harness=source_root/"tests/host/test_emotiscope_output.cpp"
        cmd=["/usr/bin/clang++","-std=c++17","-O1","-g","-Wno-narrowing",
             "-fsanitize=address,undefined","-fno-omit-frame-pointer",
             "-DK1_EMOTISCOPE_PORT=1","-DK1_PALETTE_MORPH=1","-DK1_EMOTISCOPE_TEST_SEAM=1",
             "-I"+str(source_root/"src/k1"),"-I"+str(platform),"-I"+str(source_root/"platform/ra8p1"),
             "-I"+str(platform/"emotiscope_donor"),"-I"+str(platform/"emotiscope_shims"),
             str(harness),
             str(platform/"palette_runtime.cpp"),str(platform/"emotiscope_port.cpp"),
             str(source_root/"src/k1/core/visual/channel_render_state.cpp"),
             str(source_root/"src/k1/core/visual/audio_focus.cpp"),
             str(source_root/"src/k1/core/visual/product_palette.cpp"),
             str(source_root/"src/k1/core/visual/product_palette_data.cpp"),"-o",str(binary)]
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
        # (1) Historical reference uses only its own tagged source and headers.
        # Its independent packing oracle remains strict. Current palette/AP
        # changes intentionally alter scene bytes, so the cross-image delta is
        # recorded separately from packing conformance.
        reference_tag = "titan-emotiscope-ref-r1"
        ref_src = out/"reference-source"
        ref_src.mkdir()
        archive=subprocess.run(["git","-C",str(root),"archive",reference_tag,
            "platform/ra8p1","src/k1","tests/host/test_emotiscope_output.cpp"],
            capture_output=True,check=True).stdout
        with tarfile.open(fileobj=io.BytesIO(archive)) as tree:
            for member in tree:
                path=Path(member.name)
                if path.is_absolute() or ".." in path.parts:
                    raise RuntimeError("Unsafe tagged source path")
                if not member.isfile(): continue
                target=ref_src/path
                target.parent.mkdir(parents=True,exist_ok=True)
                target.write_bytes(tree.extractfile(member).read())
        records["reference_source"]={"tag":reference_tag,"commit":subprocess.run(
            ["git","-C",str(root),"rev-parse",reference_tag+"^{commit}"],
            capture_output=True,text=True,check=True).stdout.strip(),
            "source_sha256":{str(p.relative_to(ref_src)):sha(p)
                for p in sorted(ref_src.rglob("*")) if p.is_file()}}
        ref_bin = build(ref_src/"platform/ra8p1","reference-tag",ref_src)
        ref_wire = out/"reference-tag-runtime-grb48.bin"
        ref_native = out/"reference-tag-direct-native128-rgb8.bin"
        d,_ = run("reference-tag-direct",[ref_bin,"direct",ref_native])
        r,_ = run("reference-tag-runtime",[ref_bin,"runtime",ref_wire])
        if d.returncode or r.returncode: raise RuntimeError("Reference-tag generation failed")
        ref_raw=ref_native.read_bytes()
        ref_packed=ref_wire.read_bytes()
        if len(ref_raw)!=1000*384 or len(ref_packed)!=1000*960:
            raise RuntimeError("Unexpected tagged reference geometry")
        ref_expected=bytearray()
        for frame in range(1000):
            for destination in range(160):
                index=frame*384+mapping[destination]*3
                for colour in [1,0,2]:
                    ref_expected+=(ref_raw[index+colour]*65535*100//(255*255)).to_bytes(2,"big")
        ref_expected_path=out/"reference-tag-independent-expected-grb48.bin"
        ref_expected_path.write_bytes(ref_expected)
        ref_same=ref_packed==ref_expected
        records["cases"].append({"name":"same-tag-direct-donor-to-packer","exit":0 if ref_same else 3,
            "independent_processes":True,"reference_tag":reference_tag,"frames":1000,
            "wire_bytes_compared":len(ref_packed),"reference_sha256":sha(ref_wire),
            "expected_sha256":sha(ref_expected_path),"all_wire_bytes_equal":ref_same})
        success &= ref_same
        identical = ref_wire.read_bytes()==packed
        records["historical_scene_comparison"]={"name":"current-versus-reference-tag",
            "reference_tag":reference_tag,"frames":1000,"wire_bytes_compared":len(packed),
            "reference_sha256":sha(ref_wire),"precision0_sha256":sha(actual),"identical":identical,
            "bytes_differing":sum(x!=y for x,y in zip(ref_packed,packed)),
            "purpose":"Historical raw comparison; current curated palette and AP repairs intentionally change scene output"}

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

        # (4) Palette hook reaches the wire: palette25 output must differ from boot SunsetReal1.
        pal_path = out/"runtime-palette25-grb48.bin"
        pr,_ = run("runtimeP",[binary,"runtimeP",pal_path])
        if pr.returncode: raise RuntimeError("Palette generation failed")
        pal = pal_path.read_bytes()
        differing = sum(1 for x,y in zip(pal,packed) if x!=y)
        pal_ok = len(pal)==len(packed) and differing > len(packed)//20
        records["cases"].append({"name":"palette25-changes-wire","exit":0 if pal_ok else 3,
                                 "bytes_differing_from_boot_palette":differing,"wire_bytes":len(packed)})
        success &= pal_ok

        # (5) Palettes 1-44 draw as K1 did (independent oracle in the harness), and
        # Current palette-only faults must each be rejected by the unchanged oracle.
        kp,kitem = run("k1-palette-drawing",[binary,"k1palette"])
        success &= kp.returncode==0
        for name,old,new,count in [
            ("palette-keeps-gamma","void apply_gamma_correction(){}","void apply_gamma_correction(){donor_apply_gamma_correction();}",1),
            ("palette-uses-colour-range",":base+progress;",":base+configuration.color_range*progress;",1),
            ("palette-linear-level","const float level=0.65f*v*v+0.35f*v;","const float level=v;",1),
            ("palette-colour-shifts","const float base=configuration.auto_color_cycle?configuration.color:0.0f;","const float base=configuration.color;",1),
            ("background-native-hsv","void apply_background(float background_level){","void apply_background(float background_level){donor_apply_background(background_level);return;",1),
            ("tonemap-per-channel","void apply_tonemapping(){","void apply_tonemapping(){donor_apply_tonemapping();return;",1),
            ("screensaver-fixed-colours","void run_screensaver(float elapsed_seconds){","void run_screensaver(float elapsed_seconds){donor_run_screensaver();return;",1),
            ("hype-range-unpinned","configuration.color_range=1.0f;k1_draw_scope=true;","k1_draw_scope=true;",1),
            ("palette-zero-admitted","if(id==0u||id>K1_EMOTISCOPE_PALETTES)","if(id>K1_EMOTISCOPE_PALETTES)",1)]:
            mutant=out/name
            mutant.mkdir()
            for filename in ["palette_runtime.cpp","palette_runtime.h","emotiscope_port.cpp",
                             "emotiscope_port.h","emotiscope_output.h","emotiscope_quantize.inc"]:
                shutil.copy2(plat/filename,mutant/filename)
            shutil.copytree(plat/"emotiscope_donor",mutant/"emotiscope_donor")
            shutil.copytree(plat/"emotiscope_shims",mutant/"emotiscope_shims")
            mp=mutant/"emotiscope_port.cpp";text=mp.read_text()
            if text.count(old)!=count: raise RuntimeError("Cannot isolate "+name)
            text=text.replace(old,new)
            changes=[{"old":old,"new":new,"replacement_count":count}]
            if name=="palette-linear-level":
                # Destination-aware contributions use the same curve with a
                # different parameter name; mutate the actual renderer too.
                old_level="const float level=0.65f*value*value+0.35f*value;"
                new_level="const float level=value;"
                if text.count(old_level)!=1: raise RuntimeError("Cannot isolate destination level mutation")
                text=text.replace(old_level,new_level)
                changes.append({"old":old_level,"new":new_level,"replacement_count":1})
            mp.write_text(text)
            exe=build(mutant,name+"-binary")
            r,mitem=run(name,[exe,"k1palette"])
            rejected=r.returncode!=0 and "OUTPUT_CONTRACT_FAILURE" in r.stderr
            mitem["expected_rejection_observed"]=rejected
            mitem["mutation"]={"source":"emotiscope_port.cpp","changes":changes}
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
            if r.returncode or not mut_path.is_file() or mut_path.stat().st_size!=len(cand_expected):
                raise RuntimeError("Precision mutant generation failed for "+name)
            rejected = mut_path.read_bytes()!=bytes(cand_expected)
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

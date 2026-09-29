#!/usr/bin/env python3
"""Observe the exact Emotiscope target image without changing its configuration.
Optional bounded playback uses an explicitly supplied trance/EDM WAV.
No reset, disarm, programming, brightness or system-volume write occurs here.
"""
import argparse, hashlib, json, struct, subprocess, sys, tempfile, time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/"tools/serial-studio"),str(ROOT/"scripts")]
import cdc_lock, live_lease, live_protocol
from titan_broker import find_titan, open_serial, close_serial
from titan_transport import Transport

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--output",type=Path,required=True)
    ap.add_argument("--expected-build",required=True)
    ap.add_argument("--audio",type=Path)
    ap.add_argument("--seconds",type=float,default=15)
    args=ap.parse_args()
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    result={"physical_appearance":"operator judgement pending","samples":[],"timing":[]}
    port=handle=lease=player=None
    scratch=tempfile.TemporaryDirectory(prefix="k1emoti-")
    lease_dir=Path(scratch.name)
    def save():
        (out/"result.json").write_text(json.dumps(result,indent=2)+"\n")
    try:
        usb=find_titan(inventory_lsof=False)
        handle=cdc_lock.acquire(usb["device"],"captain-emotiscope-observe")
        port=open_serial(usb["device"],handle)
        t=Transport(port,{1},campaign=True)
        info=t.info()["info"];result["info"]=info
        assert info["uid"]=="545433931bd25436593630352d068363","wrong target UID"
        assert info["build"]==args.expected_build,"wrong target build"
        assert info["contract"]=="sr12800.hop64.bins64.emotiscope","wrong active engine contract"
        t.allow({1,6,17,19,21,23,26})
        lease=live_lease.acquire(lease_dir,"captain-emotiscope-observe")
        def raw(op,payload=b""):
            q=t.transact(op,payload);assert q["ok"],(op,q["status"])
            return q["body"]
        def js(op):return json.loads(raw(op))
        result["before"]={"metrics":js(6),"palette":js(17),"mir":js(21)}
        assert result["before"]["palette"]["emit_enabled"],"emission not on"
        assert result["before"]["palette"]["brightness"]>=100,"unexpected default brightness"
        assert result["before"]["metrics"]["pdm_target"]["running"],"microphone not running"
        if args.audio:
            assert "song-0" not in str(args.audio).lower(),"banned soundtrack"
            result["playback"]={"path":str(args.audio.resolve()),
                "sha256":hashlib.sha256(args.audio.read_bytes()).hexdigest(),
                "system_volume_changed":False}
            player=subprocess.Popen(["/usr/bin/afplay",str(args.audio)],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
        start=time.monotonic();last_renew=start
        while time.monotonic()-start<args.seconds:
            if time.monotonic()-last_renew>4:
                live_lease.renew(lease_dir,lease["token"]);last_renew=time.monotonic()
            snap=live_protocol.unpack_snapshot(raw(23))
            wire=raw(19);assert len(wire)==1504,len(wire)
            name=f"wire-{len(result['samples']):03d}.bin";(out/name).write_bytes(wire)
            meta=struct.unpack("<16I",wire[:64]);rgb=wire[64:544];payload=wire[544:]
            expected=bytearray()
            for i in range(160):
                for ch in (1,0,2):
                    value=rgb[i*3+ch]*257*meta[3]//255
                    expected.extend(struct.pack(">H",value))
            assert payload==expected,"actual target wire differs from single-gain mapped RGB frame"
            vals=struct.unpack(">480H",payload)
            occupied=[sum(any(vals[(lane*80+i)*3:(lane*80+i+1)*3]) for i in range(80)) for lane in range(2)]
            result["samples"].append({"elapsed_s":time.monotonic()-start,"snapshot":snap,
                "wire_file":name,"wire_sha256":hashlib.sha256(wire).hexdigest(),
                "wire_generation":meta[1],"mode":meta[8],"brightness":meta[3],
                "max_channel":max(vals),"occupied_pixels_per_lane":occupied})
            save()
            time.sleep(0.75)
        if player:
            result["playback"]["returncode"]=player.wait(timeout=max(1,35-args.seconds))
            assert player.returncode==0,"playback failed"
        result["after"]={"metrics":js(6),"palette":js(17),"mir":js(21)}
        newest=result["samples"][-1]["snapshot"]["timing_write_seq"]
        cursor=max(0,newest-1000)
        for _ in range(18):
            h=live_protocol.unpack_history(raw(26,struct.pack("<Q",cursor)),
                live_protocol.TIMING_BYTES,live_protocol.unpack_timing)
            records=h.get("records",[])
            result["timing"].extend(records)
            if not records:break
            next_cursor=max(x["write_seq"] for x in records)
            if not next_cursor or next_cursor<=cursor:break
            cursor=next_cursor
            if cursor>=newest:break
        b=result["before"];a=result["after"]
        pb=b["metrics"]["pdm_target"];pa=a["metrics"]["pdm_target"]
        samples=result["samples"]
        result["observed"]={
            "audio_hops_delta":pa["ap_hops"]-pb["ap_hops"],
            "capture_epoch_delta":pa["stream_epoch"]-pb["stream_epoch"],
            "push_rejected_delta":pa["push_rejected"]-pb["push_rejected"],
            "asrc_discarded_delta":pa["asrc_discarded"]-pb["asrc_discarded"],
            "emitted_delta":a["palette"]["emitted"]-b["palette"]["emitted"],
            "pair_fault":a["palette"]["pair_fault"],
            "emit_errors":a["palette"]["emit_errors"],
            "distinct_sampled_wire_frames":len({x["wire_sha256"] for x in samples}),
            "distinct_sampled_rgb":len({(out/x["wire_file"]).read_bytes()[64:544] for x in samples}),
            "sampled_dark_frames":sum(x["max_channel"]==0 for x in samples),
            "sampled_stale":sum(bool(x["snapshot"]["flags"]&4) for x in samples),
            "stack_untouched_bytes":a["metrics"]["stack_untouched_bytes"],
            "heap_used_before":b["metrics"]["heap_used"],"heap_used_after":a["metrics"]["heap_used"],
            "ap_cycles_max":max(x["snapshot"]["ap_cycles"] for x in samples),
            "vp_cycles_max":max(x["snapshot"]["vp_cycles"] for x in samples),
            "emission_left_on":a["palette"]["emit_enabled"]}
        v=result["observed"]
        result["bounded_runtime_checks_satisfied"]=bool(v["audio_hops_delta"]>0 and
            v["emitted_delta"]>0 and v["pair_fault"]==0 and v["emit_errors"]==0 and
            v["push_rejected_delta"]==0 and v["capture_epoch_delta"]==0 and
            v["sampled_stale"]==0 and v["emission_left_on"])
    except Exception as error:
        result["error"]=repr(error);result["bounded_runtime_checks_satisfied"]=False
    finally:
        if player and player.poll() is None:player.terminate();player.wait(timeout=5)
        if lease:live_lease.release(lease_dir,lease["token"])
        if port:close_serial(port,handle)
        cdc_lock.release(handle);scratch.cleanup();save()
    print(json.dumps({k:v for k,v in result.items() if k in
        ("observed","bounded_runtime_checks_satisfied","error","playback")},indent=2))
    return 0 if result.get("bounded_runtime_checks_satisfied") else 1
if __name__=="__main__":sys.exit(main())

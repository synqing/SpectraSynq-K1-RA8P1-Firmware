"""Identity-bound cumulative health gate. No optical or per-cell inference."""
from __future__ import annotations

class HealthError(RuntimeError):
    def __init__(self, reason, verdict="INVALID"):
        super().__init__(reason)
        self.verdict = verdict

class WindowHealth:
    def __init__(self, uid, build, schema, stale_s=5.0):
        self.expected = dict(uid=uid, build=build, schema_sha256=schema)
        self.stale_s = stale_s
        self.identity = None
        self.baseline_errors = None
        self.last_hops = None
        self.progress_at = None
        self.last_comp = None
        self.comp_at = None

    def check(self, row, now, emitting=False):
        pal, pdm = row.get("palette") or {}, row.get("pdm") or {}
        # A positively observed fault remains FAIL even when other evidence is absent.
        for key in ("pair_fault", "lane_a0_fault", "lane_a1_fault", "emit_errors"):
            if type(pal.get(key)) is int and pal[key] != 0:
                raise HealthError(key, "FAIL")
        ident = row.get("identity") or {}
        if any(ident.get(k) != v for k,v in self.expected.items()) or not ident.get("accepted"):
            raise HealthError("identity mismatch or missing")
        stamp = (ident.get("boot_id"), row.get("epoch"), row.get("pid"))
        if any(v is None for v in stamp): raise HealthError("missing session identity")
        if self.identity is None: self.identity = stamp
        if self.identity != stamp: raise HealthError("boot, broker or epoch changed")
        if row.get("quiesced") is not False: raise HealthError("broker paused or unknown")
        for topic in ("identity", "pdm", "palette"):
            t = row.get("topic_mono", {}).get(topic)
            if not isinstance(t, (int,float)) or not 0 <= now-t <= self.stale_s:
                raise HealthError("stale or missing " + topic)
        for key in ("pair_fault", "lane_a0_fault", "lane_a1_fault", "emit_errors",
                    "pair_completions", "submitted_generation", "completed_generation"):
            if type(pal.get(key)) is not int: raise HealthError("missing " + key)
        if type(pal.get("brightness")) is not int or not 0 <= pal["brightness"] <= 255:
            raise HealthError("brightness invalid")
        if pal.get("mode_a") != 8: raise HealthError("mode drift", "FAIL")
        if type(pal.get("emit_enabled")) is not bool: raise HealthError("unknown emission state")
        if pdm.get("rate_locked") not in (1,True) or pdm.get("running") is not True:
            raise HealthError("audio lock/running missing or false")
        keys = ("asrc_starved", "pair_skew_drops", "pair_epoch_drops", "push_rejected", "stale_discards")
        errors = [pdm.get(k) for k in keys]
        for lane in pdm.get("lanes", []):
            errors.extend(lane.get(k) for k in ("error_callbacks", "overflow_events", "drop_events", "recovery_count"))
        if len(pdm.get("lanes", [])) != 2 or any(type(v) is not int for v in errors):
            raise HealthError("missing audio error counters")
        if self.baseline_errors is None: self.baseline_errors = errors
        if errors != self.baseline_errors: raise HealthError("audio errors changed", "FAIL")
        hops = pdm.get("ap_hops")
        if type(hops) is not int: raise HealthError("missing audio progress")
        if self.last_hops is None or hops > self.last_hops:
            self.progress_at = now
        elif hops < self.last_hops: raise HealthError("audio counter reset", "FAIL")
        if now-self.progress_at > self.stale_s: raise HealthError("audio stalled", "FAIL")
        self.last_hops = hops
        comp = pal["pair_completions"]
        if self.last_comp is not None and comp < self.last_comp: raise HealthError("completion counter reset")
        if not emitting or self.last_comp is None or comp > self.last_comp: self.comp_at = now
        if emitting and now-self.comp_at > self.stale_s: raise HealthError("LED completion stalled", "FAIL")
        self.last_comp = comp
        return pal, pdm

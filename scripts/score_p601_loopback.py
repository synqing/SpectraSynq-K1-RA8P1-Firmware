#!/usr/bin/env python3
"""Fail-closed sampled-trace scorer; never certifies downstream DIN or light."""
from __future__ import annotations
import argparse
import json
from pathlib import Path


def bits_from_pidr_samples(samples, pin_bit=1):
    return [(int(s) >> pin_bit) & 1 for s in samples]


def score_bits(bits, expected_cells):
    reasons = []
    if not bits or any(type(b) is not int or b not in (0, 1) for b in bits):
        reasons.append("missing_or_nonbinary_samples")
    if type(expected_cells) is not int or expected_cells <= 0:
        reasons.append("invalid_expected_cells")
    if bits and (bits[0] != 0 or bits[-1] != 0):
        reasons.append("truncated_boundary")
    rises = [i for i in range(1, len(bits)) if bits[i] == 1 and bits[i-1] == 0]
    falls = [i for i in range(1, len(bits)) if bits[i] == 0 and bits[i-1] == 1]
    if len(rises) != expected_cells or len(falls) != expected_cells:
        reasons.append("incomplete_cell_count")
    paired = len(rises) == len(falls) and all(r < f for r, f in zip(rises, falls))
    if not paired:
        reasons.append("unpaired_edges")
    return dict(expected_cells=expected_cells, samples=len(bits), rising=len(rises),
                falling=len(falls), first_rise=rises[0] if rises else None,
                last_fall=falls[-1] if falls else None,
                extra_rising=max(0, len(rises)-expected_cells) if type(expected_cells) is int else 0,
                high_after_last=sum(bits[falls[-1]+1:]) if falls else None,
                high_width_samples=[f-r for r,f in zip(rises,falls)] if paired else [],
                period_samples=[b-a for a,b in zip(rises,rises[1:])],
                mismatch=bool(reasons), reasons=reasons,
                verdict="INVALID" if reasons else "PULSE_STRUCTURE_MATCH",
                waveform="UNQUALIFIED", independent_din=False, optical="UNKNOWN")


def score_capture(c):
    bits = c.get("bits")
    if bits is None:
        bits = bits_from_pidr_samples(c.get("samples", []), c.get("pin_bit", 1))
    result = score_bits(bits, c.get("expected_cells"))
    invalid = list(result["reasons"])
    for k in ("generation", "requested_generation", "payload_hex", "sample_period_counts",
              "period_counts", "t0h_counts", "t1h_counts", "reset_counts", "build", "uid"):
        if k not in c or c[k] is None:
            invalid.append("missing_"+k)
    if c.get("generation") != c.get("requested_generation"):
        invalid.append("generation_mismatch")
    if c.get("truncated") is not False:
        invalid.append("capture_truncation_unknown_or_true")
    if c.get("uniform_sampling_qualified") is not True:
        invalid.append("sample_timing_unqualified")
    faults = []
    try:
        payload = bytes.fromhex(c["payload_hex"])
        expected = [(b >> bit) & 1 for b in payload for bit in range(7,-1,-1)]
        if len(expected) != c["expected_cells"]:
            invalid.append("payload_length_mismatch")
        sample, period = c["sample_period_counts"], c["period_counts"]
        if sample <= 0 or period <= 0 or c["reset_counts"] <= 0:
            raise ValueError("nonpositive timing")
        # Each sampled edge has <1 sample phase uncertainty, even with uniform sampling.
        # DMA-serviced samples require separate timing qualification before this applies.
        if not result["mismatch"]:
            for n, width in enumerate(result["high_width_samples"]):
                candidates = [v for v,key in enumerate(("t0h_counts", "t1h_counts"))
                              if abs(width*sample-c[key]) <= sample]
                if len(candidates) != 1:
                    invalid.append("ambiguous_width_"+str(n))
                elif n < len(expected) and candidates[0] != expected[n]:
                    faults.append("payload_bit_"+str(n))
            if any(abs(p*sample-period) > sample for p in result["period_samples"]):
                faults.append("cell_period")
            low_samples = len(bits)-1-result["last_fall"]
            if low_samples*sample < c["reset_counts"]:
                invalid.append("reset_not_fully_observed")
    except (KeyError, ValueError, TypeError):
        invalid.append("invalid_payload_or_timing")
    result.update(invalid_reasons=invalid, sampled_mismatches=faults,
                  verdict="INVALID" if invalid else "FAIL" if faults else "PASS",
                  scope="qualified sampled P601 trace only",
                  waveform="UNKNOWN" if invalid else "MISMATCH" if faults else "SAMPLED_TRACE_MATCH")
    return result


def synthesise(expected, extra_start=False, extra_end=False, oversample=8):
    return [0]*oversample + ([1]*2+[0]*(oversample-2))*(expected+extra_start+extra_end) + [0]*(oversample*4)


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--capture", type=Path, required=True)
    args=parser.parse_args()
    result=score_capture(json.loads(args.capture.read_text()))
    print(json.dumps(result,indent=2))
    return 0 if result["verdict"] == "PASS" else 2 if result["verdict"] == "INVALID" else 1

if __name__ == "__main__":
    raise SystemExit(main())

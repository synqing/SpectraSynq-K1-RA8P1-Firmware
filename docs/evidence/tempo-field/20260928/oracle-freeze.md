THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS.

Independent verifier freeze, 28 September 2026, before candidate phase scoring.
No hardware actions belong to this verification lane.

This document supplements the complete retained AP brief; it does not replace any
assignment, acceptance row, fixture requirement or execution plan.

Phase convention agreed with implementation owner: 512 chronological nonnegative
raw novelty values; newest sample at support_end, interval 1080 MEDIA_TIME_48K
frames. Rectangular additional window; existing tracker decay remains in the
input. No inferred max-pool or group-delay correction. Direct complex correlation
uses exp(-i*2*pi*f*t), where t=(index-511)*1080/48000 seconds. Its argument is
positive elapsed phase since an impulse. Support extent between first and last
sample is 511*1080 frames; analysed support includes 512 interval cells spanning
552960 frames (parent clarification; does not move phase reference). Last-observed
means the correlation support endpoint, not a detected beat instant.

Candidate arithmetic is double direct sinusoidal summation, not a recurrence.
The independent Python oracle uses complex exponentials and math.fsum, plus exact
rational impulse and derived-ratio references. No implementation code is copied.

Error budget frozen before scoring: u=2^-53, N=512,
gamma=(8*N*u)/(1-8*N*u), A=max absolute phase angle, L=sum(abs(input)),
B=L*(gamma+8*u*(1+A)). This accounts conservatively for multiply/add accumulation
and argument/transcendental error; the host libm assumption is <=8 ulp for the
tested finite angle range. Both implementations' vector errors are allowed, so
phase tolerance=asin(min(1,2*B/abs(correlation)))/(2*pi)+8*u cycles.
Conditioned scoring requires abs(correlation)>2*B. Add 0.5*2^-23 cycles only when
the public output is float; double output gets no float allowance. This is a
numerical arithmetic bound, not a musical beat-accuracy tolerance. Weak/undefined
correlation must not manufacture phase authority. Failure never enlarges this
bound: diagnose and fix, or retain the failing witness.

Frozen synthetic grid: all 512 impulse positions; amplitudes 2^-10,1,2^10;
BPM 60,90,120,155,127.43,89.5; deterministic fractional pulse trains; zero,
negative, NaN and infinity. Exact parity vectors: ratios 1/2,2/1,3/2 around wraps,
both beat parities, and long beat indices. All intended event coordinates use
rational arithmetic before deterministic rounding; full-chain PCM owned by main.

Lifecycle defaults agreed before scoring: at most four fresh analyses and eight
fixed track slots; relative peak separation 6%; correction <=0.08 cycles/update
retains generation, larger correction explicitly changes generation. Coast phase
valid at age <=48000 frames (1 second), invalid after; stale removal at age
>=96000 frames (2 seconds). Boundary tests use exact before/at/after media frames.
Copies/projections never refresh observation timestamps or field generation.

AP-AC05 musical measurement accuracy is NOT TESTED by this mathematical oracle.
AP-AC11 target deadline/capture admission is NOT TESTED by any host check here.

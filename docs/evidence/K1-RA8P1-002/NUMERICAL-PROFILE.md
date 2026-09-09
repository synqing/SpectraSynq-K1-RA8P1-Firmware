# Scalar platform-math profile — frozen before the next qualification replay

The original native HOST exact comparison remains intact and failed on M85 at
hop 337: onset strength 0.37769624590873718 versus 0.37769621610641479. The first
tempo-confidence difference occurs at hop 694. This was not an import difference:
all 52 K1 files remain byte-identical to the pinned source.

## Cause and correction to the reference environment

Apple HOST libm and the installed Arm newlib 4.4.0 do not round every Float32
logarithm identically. K1 calls logf for onset band flux and log2f/expf for tempo
priors. A library boundary was missing from the original cross-platform exact
comparison. Application -ffp-contract=off does not rebuild precompiled newlib:
the inspected scalar newlib routines use double-precision FMA internally.
Neither this scalar VFP code nor FMA implies MVE.

The new explicit profile is `arm_newlib_4_4_fma`. It still demands exact equality
of every observable field. It does not introduce a float tolerance, patch K1,
alter an existing golden or call a partial replay accepted. Native HOST receipts
and their differences remain separate evidence.

The HOST platform adapter uses the independently published Arm
[logf](https://github.com/ARM-software/optimized-routines/blob/v23.01/math/logf.c),
[log2f](https://github.com/ARM-software/optimized-routines/blob/v23.01/math/log2f.c)
and [expf](https://github.com/ARM-software/optimized-routines/blob/v23.01/math/expf.c)
algorithms and tables, with explicit FMA order checked against the installed
M85 disassembly. Before generating a reference, the script compares all three
tables byte-for-byte with the linked ELF, checks its hash and K1 source pin, and
records table, adapter and source identities. Thirty thousand deterministic
normal-domain function checks against a separate double-precision reference
meet the upstream single-function one-ULP bound. That bound is NOT an AP budget.
The diagnostic expf/log2f adapters deliberately abort outside their stated
domain rather than claim general libc equivalence.

Pinned donor sources are extracted independently with git show. Both donor and
candidate builds use the named HOST math platform. Their whole trajectories must
match exactly; the candidate never generates its own expected K1 outputs.
The compact transport is independently round-tripped for every HOST field,
preserves Float32 and unsigned 64-bit values, rejects non-finite values, and uses
the donor-emitted, hash-bound field/type schema. Full textual readouts remain
available; compact transfer changes neither the AP/VP work nor its sample clocks.

## Downstream sensitivity evidence, not a new tolerance

Across 14,000 HOST hops / 7,364,000 fields, changing only the three platform math
functions changes 1,640 strength/confidence fields. Maximum observed absolute
change is 4.76837158203125e-7. All pixels, discrete decisions, event IDs, flags,
media/Q32.32 coordinates, prediction timestamps and output ordering stay exact.
Cancellation in onset arithmetic can amplify ULP counts near zero; hence no
blanket ULP allowance is inferred. Full native-versus-Arm reports remain external.

The ongoing old-image capture is diagnostic, not a qualification under a
retroactively changed criterion. A fresh identified-image replay of all three
fixtures against this frozen explicit profile is required. Any remaining
difference fails; it must not be covered by another unexamined allowance.

## Resource and measurement boundary

The next scalar image retains M33/U55 parked, the actual AP/VP, and the memory
pixel sink. Main-stack untouched guard is 4 KiB and heap reserve guard 32 KiB,
chosen as conservative diagnostic headroom, not production maxima. Stack scan
uses RT-Thread's initial '#' fill. Heap current/maximum are allocator readouts.
DWT/tick consistency is checked over 100 ms within 2%; it is not an external
oscillator calibration. USB, serialisation and that clock check are outside
measured AP/VP work. Execution-cycle distributions separate tempo-updated frames.
No 7.5 ms deadline, physical latency, scheduled coexistence or F2 claim follows
from USB-paced measurements. Those require the separate frozen G4 campaign.

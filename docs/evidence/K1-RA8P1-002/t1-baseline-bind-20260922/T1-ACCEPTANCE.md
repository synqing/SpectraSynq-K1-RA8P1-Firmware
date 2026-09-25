# T1 acceptance rows

Bound before any live run. Scorer for a later campaign directory is `scripts/score_live_k1.py` (rows D00–D17 and G00–G06). Those rows are not this table. A green score over a missing row is a fail in that scorer. This table is the phase gate.

Submissions are not required to equal completions. Latest-pending replacement and in-flight work are allowed and must be counted separately from losses.

| Row | Required evidence | Result |
| --- | --- | --- |
| Useful debug session | Matching ELF, meaningful CPU and application state, deliberate halt/resume, clean detach | NOT TESTED on the board. A `-O2` image with `.debug_info` and `.debug_line` now exists (`e5d51385…`). No halt has been done. The image on the board is still the 21 September WS2812 show. |
| Correct physical route | Operator power-off confirmation of independent half inputs and the 80/81 break | BLOCKED. Latest wiring record still has DIN-B on P004. Photos would support identification; they do not replace that check. |
| Correct candidate | Hashes, write verification, independent UID and build, backend and profile | PARTIAL. Hashes, source snapshot (115/115), allocation and six host tests PASS. Write verification and application identity NOT TESTED. No programming. |
| Correct mapping and packing | Markers 0/79/80/159, half colours, centre-out and edges-in, generation-bound bytes, plus operator observation or video | NOT TESTED. Host pack tests PASS. That is submitted-data evidence only. |
| Useful music | Two-minute sequence, controls, progressing audio and pair counters | NOT TESTED |
| Stability | Ten-minute segment, no new loss, pair fault or generation mismatch | NOT TESTED |
| Recovery | WS2816-compatible image plus wiring and settings | NOT TESTED. The retained 21 September image is 24-bit WS2812 and is the wrong protocol for this stick. |
| Honest limits | Wire timing, TRUE16 source precision, second logical output, product qualification stay separate | PASS as a limit statement. `source_precision` on this candidate is `rgb8_to_grb48`. |

An image change gets a new identity. No PASS on this table is carried onto a different image.

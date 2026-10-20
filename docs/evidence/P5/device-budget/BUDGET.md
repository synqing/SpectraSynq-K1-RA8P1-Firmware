# Device-side latency budget — P5, 14 October 2026

THE TITAN MUST ALWAYS BE VISUALISING MUSIC.

This budget is built from the timing log of the same click-and-BOOM recording whose camera result could not be used. Image `f2f1983a53649f1c6f5e2d909d977d1029a704e9a51025e9839789293cc7d901`. Clock 1 GHz. Claims are marked [FACT], [INFERENCE] or [HYPOTHESIS]. Milliseconds in the tables are rounded to 0.001 ms. `budget.json` keeps the full figures.

## In plain English

From the sample the Titan stamps, to the moment analysis publishes that hop, takes **6.0 ms on a typical hop and 9.5 ms on a slow hop**. The click section and the BOOM section agree to about a tenth of a millisecond.

Most of those 6 ms is waiting: the microphone block, the resampler buffer and the queue. The arithmetic itself is about **1.0 ms**. Of that, the donor path is 0.28 ms and the in-house path is 0.69 ms on the hops that run it. One hop in three does almost no in-house work.

After publish, drawing the picture takes **0.20 ms**. Packing both halves of the strip takes **0.13 ms**. Shifting the bits down one 80-pixel half takes **5.10 ms**, and the two halves shift at the same time. The picture only starts on a 10 ms tick, so it then waits somewhere from 0 to 10 ms, typically 5 ms.

Put the measured pieces together and a typical hop reaches the end of the LED frame in **11.4 ms**. A slow hop takes **15.0 ms**. Adding the usual frame wait brings a typical total near **16.4 ms**. Stacking the slow analysis on the slow frame wait comes to about **25 ms**. That 25 ms adds two separate tails. It is not a single hop that was observed to take 25 ms.

The microphone slot is 7.15 ms long, and the resampler's filter is 1.14 ms. Both already sit inside the 6 ms. Leave them out of any total that already includes publish-minus-capture. Air travel, the microphone, the PDM filter and the LED's own glow are not stamped.

## The numbers

Sections use the host playback clock `_music_s`. [FACT] Clicks run from the first truth click at 4.390 s to the BOOM at 389.229 s (76,964 rows). BOOM runs from 389.229 s for 75.000 s, to 464.229 s (14,996 rows). The BOOM file is the 75 s excerpt named in the truth file.

| Stage | How it was got | In the total? | Clicks p50 | p99 | max | BOOM p50 | p99 | max |
|---|---|---|---|---|---|---|---|---|
| (a) PDM slot | Derived. 296 samples ÷ 41,408 Hz | No | 7.148 | 7.148 | 7.148 | 7.148 | 7.148 | 7.148 |
| (a) Air, microphone, PDM filter | Unmeasured | No | — | — | — | — | — | — |
| (b) Content stamp → start of analysis | Inferred split of (c) | No | 4.946 | 8.425 | 9.008 | 4.957 | 8.433 | 8.948 |
| (c) Analysis and publish | Measured. publish − capture | Yes | 5.955 | 9.546 | 10.269 | 6.027 | 9.563 | 10.215 |
| (d) Render | Measured. `vp_cycles` at 1 GHz, one value per new render | Yes | 0.197 | 0.277 | 0.282 | 0.197 | 0.277 | 0.288 |
| (e) LED pack, CPU | Measured. `emit_cycles_last`, one value per new pack | Yes | 0.128 | 0.131 | 0.134 | 0.128 | 0.131 | 0.133 |
| (e) LED wire | Derived. Formula below | Yes | 5.100 | 5.100 | 5.100 | 5.100 | 5.100 | 5.100 |
| (f) Wait for the 100 fps tick | Inferred. Uniform model, 0 to 10 ms | Yes | 5.000 | 9.900 | 10.000 | 5.000 | 9.900 | 10.000 |
| Measured path to the end of the wire | Percentile of the per-row sum (c)+(d)+(e) | This is the sum | 11.384 | 14.976 | 15.695 | 11.459 | 14.992 | 15.622 |
| Stack of the stage figures, including (f) | Sum of each column above | A stack of percentiles | 16.380 | 24.954 | 25.786 | 16.452 | 24.972 | 25.737 |

[FACT] Every one of the 94,996 rows has a usable capture stamp, and publish is always after capture. The medians above are over those rows, split by section.

[FACT] The time series has two bands, and they are the same in both sections. On the clicks, 30.1% of hops are published within 1 ms of the previous hop: their publish−capture median is 3.803 ms (p99 5.126, max 5.588). The other 69.9% have a gap of at least 5 ms: median 7.035 ms (p99 9.612, max 10.269). BOOM is 3.846 ms and 7.007 ms on the same split. The single 6.0 ms median sits between the two bands. [INFERENCE] The short rows are a second hop finished in the same burst. The content stamp still advances by 5.000 ms every row (p99 of that step is 5.003 ms, max 5.009 ms), and the average gap between publishes is 5.000 ms, so the device keeps level with the audio.

[FACT] Inside (c), the processor time (`ap_cycles`) has median 0.978 ms on the clicks and 0.984 ms on the BOOM (p99 1.276 and 1.280, max 1.290 and 1.292). Donor median 0.285 ms in both sections. In-house median 0.693 ms on the clicks and 0.700 ms on the BOOM. On 33.3% of hops in each section the in-house counter is under 10,000 cycles. [INFERENCE] That one-in-three idle share is the same pattern P4 reported for hops with no 24 kHz descriptor. It is a separate split from the 30% short hops above.

## What each stamp means

[FACT] Both clocks are the DWT cycle counter, converted at 1 GHz. `info.json` for this capture reports `clock_hz` 1000000000. `hal_entry.c:258` passes `SystemCoreClock` into `k1_fixture_initialise`, and `fixture_app.cpp:2306` passes that rate into the live clock (`k1_live_runtime.cpp:77`). Capture microseconds divide the same counter by `SystemCoreClock` (`pdm_target.c:280-282`). Live microseconds come from `nowUs()` (`k1_live_runtime.cpp:107-117`).

[FACT] The DMA completion ISR is what times a microphone block. `k1_pdm_target_dmac_isr` calls `k1_pdm_stream_context_on_data` with `k1_pdm_target_now_us()` (`pdm_target.c:443-445`). The slot stores that as `capture_end_us` (`pdm_capture.c:254`) and starts the next slot at the same instant (`pdm_capture.c:268`). The PDM data callback returns without stamping (`pdm_target.c:415-418`).

[FACT] When the block is pushed into the resampler, `receipt_us` copies that ISR time and `receipt_cycles` is sampled at the push (`pdm_target.c:670-671`). The push can be later than the ISR by the poll delay. The content estimate uses `receipt_us`.

[FACT] The published `capture_time_us` is the donor hop's newest sample, back-projected from that receipt. `pdm_target.c:1227-1243` subtracts the samples still ahead of the newest output sample, plus 47 filter samples (`K1_ASRC24_FILTER_DELAY_SAMPLES`, `k1_asrc_24k.h:20`). The comment at `pdm_target.c:1230-1231` says this is an estimate of causal filter latency, not a measured acoustic time. `physical_timestamp_valid` is set to 0 (`pdm_target.c:1218`). Provenance is `K1_AUDIO_TS_DMA_ISR_BACKPROJECTION`.

[FACT] `finalisePublication` (`k1_live_runtime.cpp:443-480`) binds the capture clock and the live clock at one DWT instant, then maps the content estimate onto the live clock. The mapping rejects a stamp that would fall after publish (`k1_live_runtime.cpp:479-480`). On this image `consume()` calls that function twice. The early call (`k1_live_runtime.cpp:237`) only binds the clocks. The timing record stores the later call, after in-house analysis (`k1_live_runtime.cpp:325-338` and `408-411`). So stage (c) runs from the donor content instant to the end of donor-plus-in-house publication.

[FACT] `vp_cycles` is the last completed render (`fixture_app.cpp:570-587`: "Render cost, excluding asynchronous LED transmission"). It is copied onto later hops until the next render (`k1_live_runtime.cpp:413`). The stage (d) figures count one value each time the field changes: 38,483 renders in the clicks, 7,499 in the BOOM.

[FACT] `emit_cycles_last` is the pack-and-submit of both DINs (`fixture_app.cpp:633-728`), stored by `k1_pdm_target_note_emit_cycles` (`pdm_target.c:296-297`). `take_hop_cost` clears the poll and resampler counters and leaves the emit cost in place (`pdm_target.c:287-293`), so it repeats until the next frame. Stage (e) CPU counts one value per change.

[FACT] `hop_dt_us` is the gap between successive publish instants (`k1_live_runtime.cpp:329-331`). `_music_s` and `_host_read_s` are host clocks added when the row was read (`scripts/titan_cost_capture.py:135-138`). They are not device time. One USB read stamps every row in that read. The playback stamps are 0.027 s apart at the median.

## Stage notes

**(a) Acoustic path and the PDM block.** [FACT] The slot is 296 samples (`pdm_target.h:16`). The nominal length is 7.400 ms at 40 kHz (`pdm_target.h:20`). Every scored row reports `measured_hz` 41408, so the block is 296/41408 s = 7.148 ms. The 47-sample filter at that rate is 1.135 ms (`k1_asrc_24k.h:20`, and the same 47 in `k1_poly24.h:73` for the 24 kHz path). [FACT] Both are already removed from the DMA receipt when the content stamp is built, so they are inside (b) and (c). [FACT] Nothing in the log times the air path, the microphone or the sinc-decimation group delay. Sincdec is 49 and SINCRNG is 10 (`pdm_target.h:21-24`). No millisecond is invented for them.

**(b) Capture to descriptor.** [FACT] `descriptor_ready_us` is written when the donor hop is pulled (`pdm_target.c:1197`) and is not in the timing log. Stage (b) is publish−capture minus `ap_cycles`. On every row that equals the wait from the content stamp to the start of the measured analysis. It includes the filter, the rest of the DMA block, the queue and the descriptor pull. [FACT] The resampler queue at consume has median 355 samples (p99 500), which is 8.57 ms of source audio at 41,408 Hz. [INFERENCE] That depth is the buffer the back-projection already walks through. It is not another stage on top of (b).

**(c) Analysis and publish.** [FACT] This is the measured capture-to-publish interval. P4's earlier BOOM window, a different recording, was 5.998–6.046 ms at the median, 9.561–9.581 ms at p99, and 9.763–10.322 ms at the max (`docs/evidence` review F2, file `_scratch/waveform-migration-001/runs/P4-20261014T085434Z/permitted/REVIEW.md`). This run's BOOM median is 6.027 ms, p99 9.563 ms, max 10.215 ms. [INFERENCE] This image sits on that same P4 band. The P2 final image on that older window was faster: median 5.465 ms.

**(d) Render.** [FACT] Measured on the device, and it excludes the LED wire. Median 0.197 ms. The all-row median, which repeats each render across the hops that share it, matches to 0.001 ms.

**(e) Pack and wire.** [FACT] The CPU part is the measured pack-and-submit, median 0.128 ms. The wire part is derived. Profile 3 (`ws281x_diag.h:41-42`: period 1250 ns, reset 300 µs, 6 bytes per pixel) is the profile submitted for both DINs (`fixture_app.cpp:646` and `fixture_app.cpp:675-676`). The cost function is `k1_ws281x_gpt_cost_lanes` (`ws281x_waveform.c:165-167`):

```
bits     = 80 pixels × 6 bytes × 8 = 3840
data_ns  = 3840 × 1250 = 4,800,000
reset_ns = 300 × 1000 = 300,000
emit_ns  = 5,100,000  = 5.100 ms
```

[FACT] The strip is one 160-pixel line, split 80 and 80 (`fixture_app.cpp:639-641`). The two DINs run together, so the wall time is one lane, 5.100 ms. Profile 4 is a different timing and is not the path this image submits.

**(f) Frame tick.** [FACT] Under the Emotiscope build the palette period is 10,000 µs (`palette_runtime.h:40`), and this capture's identity reports `render_hz` 100. A frame is due when `now >= next` (`palette_runtime.h:144`). A late frame jumps ahead by whole periods (`palette_runtime.cpp:359-361`). [FACT] During this capture the device emitted 47,505 frames and skipped none (`report.json` deltas) over a 474.99 s publish span, which is 100.01 frames per second. [INFERENCE] The wait from a publish to the next tick was not stamped per hop. With frames holding 100 fps and hops arriving on their own 5 ms grid, the phase mixes, so the wait is modelled as uniform from 0 to 10 ms: median 5 ms, p99 9.9 ms, max 10 ms. [FACT] The zero skip count means this recording did not pay an extra skipped frame.

[FACT] The field `a24_cycles_div16` is 0 on every row. The capture report says analysis24 was not exported by this image. The in-house cycle counter above is a different field and is not zero. [INFERENCE] The zero spare field means the polyphase cycle total was not recorded. It does not mean the in-house work was absent.

[FACT] The since-boot poll high-water mark is 8,672,049 cycles, 8.672 ms (`report.json`). The per-hop poll maximum inside this recording has median 0.162 ms and max 0.171 ms. [INFERENCE] The 8.7 ms mark is a one-time peak outside these hops. It is not added to the stage total. P4 described a filter-design stall of that size; this log does not identify the cause of the high-water mark by itself.

## What remains unmeasured

**Air, the microphone and the PDM filter.** [FACT] The stamp begins at a back-projected DMA time, and the firmware marks the physical time invalid. No millisecond is assigned.

**Where inside a 5 ms hop the click sits.** [FACT] The published stamp is the newest sample of a 64-sample donor hop, which is 5.000 ms of audio at 12.8 kHz. [INFERENCE] A click that is not the newest sample is older than the stamp by up to one hop. That uncertainty is inside the hop the analysis already consumed. It is not added again after publish.

**LED electrical frame to photon.** [FACT] `emit_cycles` ends at submit. The 5.100 ms is the shift-register time from the profile constants. Phosphor, LED driver and the first photon are not in the log.

**Whether the waveform drew a flash.** [FACT] The camera pass on this recording claimed no latency. Hit fractions were 0.50 and 0.38, and the null control fired (`permitted/analysis-v2/report.json`). The assignment states the cause: the waveform does not flash on every click. A correlation peak on that picture is not a latency.

### PROPOSAL — NOT APPROVED

Two small tests would close the two holes. Neither is authorised by this budget. Either one has to leave the show running: Titan boots into music and stays there.

1. **Emission to photon.** Add one logged, full-scale edge on top of the running show, at a DWT time the existing timing record already keeps, and film only that edge with a camera faster than the product 30 fps. Score the camera frame against the logged submit. That isolates the glow from the music mapping. It changes what the strip emits, so it needs Captain's OK before anyone builds it. A diagnostic that replaces the show with a click-flash is the wrong shape.

2. **Did the effect draw the click?** Keep the packed 160-pixel frame the firmware already builds (`palette_wire_snapshot` in `fixture_app.cpp`) on each 10 ms tick, and score those bytes against the click times. This timing log does not contain those frames. The score answers whether the effect changed the buffer. A camera comes after that, and only to time an edge the buffer says was sent.

## Integrity

[FACT] All of the following were counted from `timing.jsonl` (sha256 `ca3345699993b6ab2b1fe49b1cd295a30573bb79d991bf8ffd04bdd8268a70f1`, the same file the camera report names).

| Check | Result |
|---|---|
| Rows | 94,996 |
| `write_seq` | 47,272 through 142,267, every step +1. Missing 0, duplicates 0, backwards 0. The log starts mid-uptime. |
| `hop_sequence` | Equal to `write_seq` on every row |
| `capture_time_us == 0` | 0 |
| publish before capture | 0 |
| Capture time going backwards, zeros excluded | 0 |
| Successive content stamps | Median step 5.000 ms, max 5.009 ms |
| Before playback (`_music_s` < 0) | 599 rows |
| Lead-in, including the opening sync | 882 rows |
| Click section | 76,964 rows |
| BOOM section | 14,996 rows |
| After the 75 s BOOM | 1,555 rows |
| Flags nonzero, resampler starved | 0 and 0 |

[INFERENCE] Section edges can be off by about one USB read, whose stamps are 0.027 s apart at the median. The row counts times 5.000 ms match the 384.8 s click window and the 75.0 s BOOM window, so that smear does not move the percentiles.

## Files

- `BUDGET.md` — this note
- `budget.json` — every percentile, the integrity counts, the wire formula, the two hop bands
- `budget.png` — stacked stage bars at p50 and p99, the split of the publish delay, and publish−capture across the recording
- `compute_budget.py` — the host script that wrote the json and the png

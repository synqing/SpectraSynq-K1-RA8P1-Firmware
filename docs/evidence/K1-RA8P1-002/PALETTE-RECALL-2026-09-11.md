# WS2812 demo recall and WS2816 mismatch — 11 September 2026

## Verdict

Captain reports that the WS2816 appearance is nothing like the earlier WS2812
demo. The earlier assistant claim "restored" is withdrawn as a claim of visual
equivalence. Command acceptance and advancing palette IDs do not establish that.
No firmware, LED selection, brightness, GPIO configuration or flash was changed
during this investigation. Only INFO and STATUS were read from the live board.

Two distinct historical WS2812 runs have been recovered from retained build
sources and target receipts. The later run matches the stated five-to-six-hour
window; the earlier run is the one that `docs/PALETTES.md` associates with
Captain's positive appearance feedback. The original feedback/launch transcript
has not been recovered. Captain subsequently clarified: "I believe it would be
the later version". Use the midnight centre-effects showcase as the working
reference; do not restart the earlier-versus-later search. This is Captain's
selection, not a newly recovered historical transcript.

All artifact paths below are relative to:
`/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/`.

| | Earlier palette morph | Later centre-effects showcase | Current WS2816 |
|---|---|---|---|
| Perth time | 10 Sep 21:29–21:34 | 11 Sep 00:00 | 11 Sep 05:44 configuration; 05:57 readback |
| Build prefix | `d354879b` | `3decd154` | `11b95cdf` |
| Evidence | `palette-morph-live-01`, `palette-morph-transition-live-02` | `centre-effects-live-01` | `ws2816-palette-showcase-01`, `ws2816-palette-recall-01` |
| Rendering | Mode 0, full-strip palette gradient moving along strip | Modes 100–103, ribbons/aurora/embers/pulse | Modes 100–103 |
| Palette selection | Bases 33/43; all 44 cycling | Bases 33/43; all 44 cycling | Same bases and cycle |
| Palette fades | 1500 ms configured | 1500 ms configured | 1500 ms configured |
| Geometry | Linear full-strip mapping | Centre-symmetric radial mapping | Centre-symmetric radial mapping |
| Animation clock | RTOS millisecond accumulation | Extended DWT cycle count | Extended DWT cycle count |
| Nominal scheduler | 8333 us | 8333 us | 16667 us |
| Physical output | 128 WS2812 on P601 | 128 WS2812 on P601 | 80 WS2816 on P601 plus 80 on P004 |
| Output gain | 24/255 | 24/255 | 128/255 |

The old gradient's linear motion differs from the later explicit centre-origin
instruction. Do not silently restore that geometry or call a centre-remapped
version byte-identical to it. Source at
`palette-morph-build-01/stage/src/palette_runtime.cpp`, function `preview`;
compare `centre-effects-build-01/stage/src/palette_runtime.cpp`.

## Memory lookup and missing history

Used the requested claude-mem-router → mem-search workflow and context-stack.
The context-engineering router routed context recovery, not a product rewrite.
Queries `palettes`, `palette`, `WS2812`, `Titan`, `morph` were inspected, with
recent-date searches followed by unfiltered sanity checks. Memory preflight
reported NOMINAL; this is service health, not complete capture coverage.

Search/timeline/get_observations for observation **101719**, session
`26220287-0289-4df6-ac45-ab9f1ee81b1d`, refers to the separate original K1
Chromatic Score execution brief, not a Titan palette demonstration. No recent
Titan palette implementation observation was returned. The server-beta search
surface refused because the configured runtime is worker; no configuration was
changed to work around that refusal.

Raw Codex transcripts inspected:

- `01a088aa-8058-7961-a1dc-7e60076bbb74`: this Titan/LED thread; no original
  palette-demo exchange between the 13:32 UTC console discussion and 16:07 UTC
  WS2816 request.
- `01a0870e-04db-7b40-bb12-67a86ea205d4`: Titan AP/PDM/line-in work, not the
  missing palette session.
- `01a08b43-b042-7e13-a510-2b8555ca42ff`: original K1 Chromatic Score work.
  Its positive colour feedback at 21:01 UTC is not evidence about Titan.

The installed Crispy recall CLI returned no September matches and its recent
session list stopped in April. Do not infer that the demonstration never happened
from incomplete memory indexes. Do not ask Captain to re-prove his observation.

## Decisive source/frame comparison

Both archived palette tables have SHA-256:
`f4df31d112f587c39a4d23bb34799b7cff937ae54207a1ec7dc984c85d609060`.

Built independent HOST executables from the complete staged source trees of
`centre-effects-build-01` and `ws2816-palette-build-01`. No production source was
edited. Identical configuration: palette bases 33/43, effect bases 100/101,
flags 23, 1500 ms fades, 4000 ms travel. Both executables were sampled at the
same 3841 timestamps, 50 ms apart, over 192 seconds of simulated time.

Result: **3,687,360 native RGB8 bytes per executable; zero mismatched bytes**.
Both output hashes:
`aafe205b61ad6dcbdab12f787c8757d4744df618f473d0ee0e8269023df7eb38`.

This establishes equality of these sampled logical frames, not equal physical
light, actual frame rate, clock accuracy, or the identity of Captain's intended
demo. It does not compare the earlier linear-gradient build with the showcase.

Reproduce:

```sh
/Users/spectrasynq/SpectraSynq-EdgeAI-Lab/.venv/bin/python \
  /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/ws2816-palette-recall-01/compare.py
```

`comparison.json` contains compiler commands, source hashes and the results;
`comparison-stderr.txt` is empty; exit code was 0.
`current-readback.txt` binds current UID/build and reports cycle/showcase active,
profile 4, 160 pixels and zero software emission errors. Its observer closed CDC.

## What remains different and unproved

Old wire values: `floor(Pixel8 * 24 / 255)` into GRB24, with native 160 positions
resampled to 128. Current: `floor(Pixel8 * 257 * 128 / 255)` into GRB48, with
native positions 0–79 and 80–159 sent to separate pins. These are different code
values and output paths; no photometric equivalence or LED transfer function was
measured. The doubled nominal period and sequential DIN emission also differ.
No evidence yet isolates any one of them as the optical root cause.

Next bounded discriminator, owned by the executing Titan agent: preserve the
archived reference, select a fixed palette/time frame, compare complete intended
wire bytes on both paths, then check actual emitted signal/LED response. The
selected reference is now the later showcase, modes 100–103. Do not run another arbitrary
showcase and call it restored. Do not raise brightness or modify timing merely
because software CRCs pass. Board state and unrelated dirty work are preserved.

## Follow-up: WS2816 conversion inspection

Read-only source inspection after Captain's selection found no omitted software
gamma operation in the inspected FastLED WS2816 conversion. This does not
establish the fitted LED's optical transfer function or full controller parity.

- Pinned donor `adedfc40e73fb80f8e930318781036d8fe1dbd9f`, retained at
  `/tmp/ra8p1-fastled-research.x8zMEg/FastLED`: function
  `ScaledPixelIteratorRGB16::advance` in
  `src/fl/chipsets/encoders/pixel_iterator.h:483` obtains corrected RGB8 values,
  expands them to RGB16 and applies separate brightness in HD mode. It does not
  insert a gamma transform in that adapter. Non-HD mode scales before expansion.
- The separately installed WS2816-Testbed FastLED's
  `src/pixel_controller.h:533`, `loadAndScale_WS2816_HD`, likewise expands and
  scales without software gamma. Its comment about internal chipset gamma is
  not independent evidence of this stick's fitted part or response.
- Titan `src/k1/core/visual/product_palette.cpp:132`, `quantiseLinearRgb`,
  quantises components; `platform/ra8p1/palette_runtime.cpp:178`,
  `packBenchGrb48Lane`, expands RGB8 and applies the configured gain. The earlier
  raw-packer oracle did not test FastLED's full scaling/correction pipeline.

Consequently, adding an assumed gamma curve is not a demonstrated fix. Preserve
the selected renderer and investigate the output transfer using paired fixed
frames. This follow-up changed documentation only: no firmware, flash, pin,
timing, brightness or live serial operation.

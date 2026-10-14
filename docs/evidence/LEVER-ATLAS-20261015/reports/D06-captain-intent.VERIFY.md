---
abstract: "Adversarial verification for D06-captain-intent (15 Oct 2026). The D06 report itself could not be checked: it was never written to disk and its text was not passed to this verifier. As a substitute, 14 Captain-intent facts from SWARM-BRIEF.md that D06 must rely on were checked against source: 10 confirmed, 2 partly wrong, 2 have no source on disk."
---

# D06 Captain-intent verification

## What happened, in plain English

I could not check the D06 report. It was never saved: the D06 folder is empty (`ls -la D06-captain-intent/` showed only `.` and `..` before this file). Its text was also not included in what I was given. The orchestrator's note says "the full content is in this result", but that result never reached me. So **I checked none of D06's own claims**. Treat D06 as **UNVERIFIED** until its text is saved or passed to a verifier.

Instead, I checked the Captain-intent facts in `SWARM-BRIEF.md` that any D06 quote ledger has to rest on. 10 of 14 hold up. Two are partly wrong. Two have no source on disk. They exist only in the chat.

## Checks against source (Titan worktree HEAD 1ef5395, branch lane/titan-lagfree-20261014)

| # | Brief claim | Verdict | Evidence |
|---|---|---|---|
| 1 | At 81ac031, inhouse_waveform.cpp ~209–231 used the 16-slot palette and blended RGB channels into one colour per frame | CONFIRMED | `git show 81ac031:src/k1/core/visual/inhouse_waveform.cpp` lines 200–235: it loops `compiled_fastled16[i]` for i<16, blends `chroma_colour`/`fallback` channel by channel (`mul(q(..red),b)+mul(..,ib)`), and runs one `dot_` EMA (0.080) per frame |
| 2 | Captain: v9 "overall superior to v8" | CONFIRMED | `_scratch/waveform-migration-001/runs/STATUS.md:99` 'Do not revert, this current build is overall superior to v8'; Kx-Titan `docs/migration/MUSIC-VISUAL-VALUE-TAKEOVER-2026-10-08.md:1904` |
| 3 | Captain: the drop still does not hit harder than the build | CONFIRMED (verbatim) | `STATUS.md:107`: "That's still a no, BUT, there was a noticible difference (slight improvement) in the 'tension release'..." |
| 4 | Captain: fade-to-black was an improvement | CONFIRMED (agent paraphrase, not verbatim) | `STATUS.md:99` "lingering trails/fade-to-black improved, dark at end mostly" |
| 5 | Captain's "explosive force / thrust ... edges towards the centre" quote | NO SOURCE ON DISK | grep for "explosive force" in the Titan docs, the runs folder (excluding LEVER-SWARM) and Kx-Titan docs found nothing. It exists only in the brief and the chat. A D06 record that marks it [FACT] must cite the chat, not a file |
| 6 | Captain: palette "not being accurately mapped to each of the 80 bins", "over-averaging" | NO SOURCE ON DISK; meaning disputed | `docs/evidence/P6/palette-forensics-2/mem-2025.md:27,149`: "over-averaging" has 0 hits in claude-mem in any year. The [INFERENCE] there is that "80 bins" means 80 per-half pixel positions (`LOGICAL_LEDS = 80`), not 80 frequency bins |
| 7 | "It was fixed in K1 firmware", commit c2738b88 (per-bin HD sampling, `palette_manual_colour(pal, freq_prog, bin)`) | PARTLY WRONG | The line exists: `git show c2738b88` diff line 123 `final_color = palette_manual_colour(pal, freq_prog, bin);` replacing a `CRGBPalette16`/`uint8 paletteIndex` path. But the commit (22 Aug 2026) is titled "paint C1/C2 colour in 16-bit after device PASS". It swaps seven `ColorFromPalette` sites for 16-bit paint precision. It does not describe an averaging fix. `palette-forensics-2/grok-lightwave-git.md:11` says no commit in the searched histories shows the Captain's before/after. The 1:1 bin→palette map came earlier, in SB `6109a1e` |
| 8 | SensoryBridge 6109a1e (Spectrum River, hue=k/79) and d408433 (no 16-entry flattening) | CONFIRMED; brief paths are wrong | Both objects exist in `/Users/spectrasynq/SensoryBridge-golddig` (and 7 other `~/SensoryBridge-*` clones). They do NOT exist at the brief's paths: `/Users/spectrasynq/SensoryBridge-main` is not a git repository, and `Workspace_Management/Software/SensoryBridge{,-1,-2}` lack both commits. Content: `grok-lightwave-git.md:64,111` |
| 9 | K1 ae5d90a: chroma position never freezes | CONFIRMED (subject) | `git log -1 ae5d90a3`: "feat(vp): K1_PALETTE_VIBRANCY_V1 — fix palette-coordinate collapse on dense/loud music" |
| 10 | Brightness: 24/255 here vs 100/255 in Kx-Titan AGENTS.md | CONFIRMED CONFLICT | Kx-Titan `AGENTS.md:12` "default brightness100/255 supersedes historical24"; the global CLAUDE.md Titan mandate preserves 24/255. Not resolved here, as the brief requires |
| 11 | Tempo tracker locks to half time via an 88 BPM prior | CONFIRMED | `docs/evidence/P7/RECEIPT.md:19,105` (log2-Gaussian on 88 BPM, σ 0.75 octave; 69 and 92 BPM locks) |
| 12 | PL14-B14: the product default did not select the fix | CONFIRMED | Kx-Titan `docs/migration/TITAN-PRODUCT-LANE-LESSONS-2026-10-14.md:148` |
| 13 | LED wire 3840 bits × 1.25 µs per line | CONFIRMED | `docs/evidence/P5/device-budget/BUDGET.md:73-74` |
| 14 | "Three detectors failed to fire within +50 ms of annotated drops"; the drop energy is below 110 Hz | PARTLY WRONG | `runs/P6-DROP-DETECTOR/DETECTOR.md:11`: the chosen detector DID hit the BOOM drop (on a click) inside 50 ms, but no other required drop. So "failed" means it failed the gate, not that it never fired. `:21` confirms the energy is mostly below 110 Hz |

## How the "hit harder" requirement can be tested

`docs/evidence/P6/REVIEW-ITERATION-1.md:34,54` [FACT]: the frozen C1 metric (a median of build vs drop) passes while the Captain's eye says no, and it is marked NOT VALID AS A PROXY. Any D06 "testable requirement" for "hit harder" that reuses a build/drop median repeats a known-invalid proxy. The next question for the Captain is still open (`STATUS.md:119`): does "hit harder" mean peak brightness, spatial spread, or contrast against the build?

## Lever origins asserted without evidence

The brief frames the 16-slot palette compression as an ESP32-S3 budget trade-off. On disk the evidence points to **inherited / library default** instead. `CRGBPalette16` + `ColorFromPalette` is FastLED's standard API. SB `d408433` replaced it as an accuracy fix, and its message cites lost resolution, not CPU headroom (`grok-lightwave-git.md:111`). Any D06 lever labelled origin = "S3 budget" for the 16-slot palette, the uint8 coordinate or the per-frame single sample is **[UNSUPPORTED]** unless it cites a measured S3 cycle cost. I could not check D06's other origin labels because the report is missing.

## What remains

1. The orchestrator saves D06's returned text to `D06-captain-intent/REPORT.md` (parent-owned write), or passes it to a re-run of this verifier.
2. The re-run checks at least 12 of D06's own [FACT] claims against the table above.
3. Only then can D06 claims enter synthesis as verified.

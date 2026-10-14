---
abstract: "P7 step 2(b) real-corpus proposal for the tempo half-time / 3:2 gate. PROPOSAL — NOT APPROVED. Lists local candidate tracks per genre slot, a proposed annotated beat level and BPM with reasoning, and a tuning / held-out split. Nothing here is frozen."
---

# P7 real corpus — PROPOSAL — NOT APPROVED

THE TITAN MUST ALWAYS BE VISUALISING MUSIC.

**Status: PROPOSAL — NOT APPROVED. Not frozen. The Captain supplies or approves every track and confirms every annotation.**

## In plain English

Seven of the twelve slots have no suitable track on this machine. They say **Captain to supply**:

- DnB;
- hardstyle;
- reggaeton/dembow;
- half-time dubstep;
- a ballad under 80 BPM;
- a held-out track for hip-hop;
- a held-out track for house.

The three required songs (BOOM, Sonic Samba, Dopex) are on disk. So are one more trance track and a local research dataset (MUSDB18) with labelled rap, reggae and electronic songs.

I cannot listen. The BPMs below come from three independent automatic estimates, set beside what our own tracker reports with its prior removed. Where those disagree, I say so. **Every BPM is a proposal for the Captain to confirm by ear.**

## How the proposed BPMs were obtained

`src/p7_corpus_scan.py` → `corpus/corpus-scan.json`. It decodes 75 s per track; MUSDB tracks start at 30 s. Three estimates per track:

- **bt:** librosa 0.11 `beat_track`.
- **tg:** the librosa tempogram's three strongest tempi.
- **kick:** an autocorrelation of the rectified log-energy rise below 150 Hz, the kick band.

**[INFERENCE]** These estimates do not use our tracker. They are known to make octave errors themselves, and the tempogram bins are coarse (about ±3 BPM near 140).

**Annotated level (Captain default, P7):** the kick/backbeat level.

- Four-on-the-floor reports the kick.
- Hip-hop reports the backbeat quarter (snare on 2 and 4), not the hats.
- DnB reports 174, not 87.

## Proposed tracks

| Slot | Local track (path) | bt / tg / kick estimates | Proposed level and BPM | Reasoning | Split |
|---|---|---|---|---|---|
| BOOM (required) | Tiësto & Sevenn — BOOM, `_scratch/waveform-migration-001/audio-boom-001/boom-0-75s-pcm16.wav` (0–75 s; sha256 `09c9a02f…`) | 136.0 / 136.0, 69.8, 143.6 / 139.5, 69.0 | kick level, **138** | All three estimates fall within 136–140. P3 measured a 138 BPM beat line and a 3+3+2 accent at 0.652 s. Our tracker with a flat prior reports 138 in the drop. Exact value (138 against 139/140) to be confirmed by ear or by a beat-grid fit. | tuning |
| Sonic (required) | Armin van Buuren — Sonic Samba (Rising Star Remix), `titan-music-intelligence-001/tests/fixtures/tempo_field/v1/trance/sonic-samba.wav` (0–75 s, 24 kHz) | 143.6 / 143.6, 136.0, 71.8 / 129.0, 112.5 | kick level, **about 142–144** | bt and tg agree. Our tracker with a flat prior reports 142–144. The kick-band estimate (129) disagrees, so **uncertain**. | tuning |
| Dopex (required) | Gareth Emery — Dopex, `…/trance/dopex.wav` (0–75 s, 24 kHz) | 76.0 / 152.0, 76.0, 99.4 / 99.5, 129.0, 156.5 | kick level, **about 150–152, uncertain** | tg's top tempo (152), double bt (76), and our tracker with a flat prior (150–151) agree. The kick band gives 99.5. **Needs the Captain's ear.** | **held-out** (the existing lineage policy already holds the whole Dopex lineage out of tuning) |
| extra trance | Veracocha — Carte Blanche (Mixed), `…/trance/carte-blanche.wav` | 143.6 / 69.8, 143.6, 92.3 / 139.5, 70.0 | kick level, **about 140** | The kick band and tg's second tempo agree near 140. Optional. | tuning |
| DnB 170–176 | none found | — | **Captain to supply** (2 tracks) | No local DnB. Celestial Shore scores 172 but is labelled singer/songwriter. | 1 tuning + 1 held-out |
| hardstyle about 150 | none found | — | **Captain to supply** (2) | — | 1 + 1 |
| house 120–128 | MUSDB **PR — Happy Daze** (`SpectraSynq-EdgeAI-Lab/datasets/musdb18/…/PR - Happy Daze.stem.mp4`, genre Electronic) | 123.0 / 123.0, 117.5, 80.7 / 120.5, 64.0 | kick level, **about 122** | All three estimates fall within 120–123. MUSDB labels it "Electronic", not house. The Captain confirms it as house. | tuning |
| house 120–128 (second) | MUSDB **Girls Under Glass — We Feel Alright** or **Chris Durban — Celebrate** (Electronic) | 123.0 / 63.0, 123.0 / 62.5, 126.0 — and 129.2 / 129.2 / 64.0, 123.0 | about 124–126, or 123–129 | The estimates are octave-split. **Uncertain**; otherwise **Captain to supply**. | held-out |
| hip-hop 85–100 | MUSDB **Grants — PunchDrunk** (Rap, CC BY-NC-SA) | 99.4 / 99.4, 198.8, 64.6 / 97.5, 191.5 | backbeat level, **about 98–99** | All three put the quarter near 98–99. | tuning |
| hip-hop 85–100 (second) | none with agreeing estimates. The other MUSDB Rap tracks (ANiMAL ×3, Lushlife, M.E.R.C., Side Effects Project, Little Chicago's Finest) split 63–199. | — | **Captain to supply**, or confirm one by ear | — | held-out |
| reggaeton/dembow | none found | — | **Captain to supply** (2) | MUSDB reggae (Arise — Run Run Run, about 86, all three agree; Sunshine Garcia Band) is one-drop reggae, not dembow. It could be offered as an extra slot if the Captain wants one. | 1 + 1 |
| half-time dubstep 140 | none found | — | **Captain to supply** (2) | — | 1 + 1 |
| ballad under 80 | none confirmed | — | **Captain to supply** | The MUSDB singer/songwriter tracks score 83–172. Alexander Ross — Velvet Curtain agrees at about 83 (above 80). Music Delta — Beatles has tg/kick about 73–74 against bt 112. Neither is confirmed. | 1 + 1 |

## Proposed split (frozen together, before any tuning)

- **Tuning:**
  - BOOM;
  - Sonic Samba;
  - Carte Blanche (optional);
  - PR — Happy Daze;
  - Grants — PunchDrunk;
  - one Captain-supplied track each for DnB, hardstyle, dembow, half-time dubstep and ballad.
- **Held-out:**
  - Dopex;
  - a second house track;
  - a second hip-hop track;
  - one Captain-supplied track each for DnB, hardstyle, dembow, half-time dubstep and ballad.
- Held-out tracks are never run before the tuned tracker is frozen. Dopex is already a lineage holdout. The recording `dopex-admission-fix-holdout-001/observation.json` carries `audio_window.lineage_policy`: "whole Dopex source lineage held out from tuning; first exposure after candidate freeze".
- **Disclosure.** This P7 host work has already replayed Dopex through the *current* tracker. That was a diagnosis only (step 1); nothing was tuned on it. The Captain may decide whether this counts as exposure.

## What running the corpus needs (not done here)

- **Room recordings.** Each track must run as the file and as a room recording, at device-calibrated level ±12 dB.
  - Only BOOM, Sonic (the 6–30 s slice only) and Dopex have room recordings today, in `canonical-audio-v01/`.
  - Every other track needs a new room capture on the bench. That is a hardware and room task, outside this host dispatch.
  - The 15-minute same-song loop limit (EdgeAI datasets README) applies to room playback.
- **Exact annotations.** The gate needs a beat grid, not one BPM per track.
  - Proposal: the Captain confirms the level by ear.
  - An agent then fits the beat grid by aligning the kick (or backbeat) onsets, and freezes the grid with its residuals.
- **Licences.** MUSDB tracks are research-only: "Restricted" (DSD) or CC BY-NC-SA. They are fine for an internal gate, not for demos.

## RULING NEEDED

1. Supply the missing slots: DnB ×2, hardstyle ×2, dembow ×2, half-time dubstep ×2, ballad ×2, a hip-hop held-out and a house held-out.
2. Confirm or correct the proposed level and BPM for BOOM (138), Sonic Samba (about 142–144), Dopex (about 150–152, uncertain), Carte Blanche (about 140), PR — Happy Daze (about 122) and Grants — PunchDrunk (about 98–99).
3. Approve the tuning / held-out split above.
4. Approve new room recordings for every track that lacks one, and the ±12 dB level sweep.

## Changelog

| Date | Author | Change |
|---|---|---|
| 2026-10-14 | agent:claude (P7 host worker) | Created. Proposal only; nothing frozen. |

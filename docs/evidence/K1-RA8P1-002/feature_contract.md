---
abstract: "H1 strength/shape/surprise adapter contract for admitted 24 kHz / 180 / 80-bin GDFT. Musical promotion needs an authorised holdout."
---

# H1 feature contract

- Source: existing AP `AudioFeaturesV1.spectrum[80]` plus `spectral_energy`.
- Rate/profile: `sr24000.hop180.bins80.xover40` only. Do not convert 12.8 kHz/96.
- Channel fusion: mono AP frame already fused.
- Units: linear band energy, L2-normalised shape, L2 surprise against the
  previous frame's shape (past-only). Silence/`kEventSilence` is invalid.
- State owner: `k1::core::audio::StrengthShapeAdapter`. Reset advances epoch.
- Future-prefix: a later frame cannot change an already emitted observation.
- Promotion: not claimed. Needs a labelled holdout corpus.

**Document Changelog**
| Date | Author | Change |
|------|--------|--------|
| 2026-09-13 | agent:grok | Created with the host adapter. |

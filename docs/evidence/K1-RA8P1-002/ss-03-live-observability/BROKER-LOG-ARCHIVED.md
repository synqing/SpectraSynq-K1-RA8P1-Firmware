# SS-03 broker log archive — superseded, non-authoritative

Stopped `titan_broker.py` pid 74965 on 2026-09-19. CDC lock files removed.
The live JSONL was **moved, not truncated**. Misleading `LIVE OBSERVATION` /
`identity_ok` rows for build `d13cd520…` are provenance of a stale observer,
not evidence of resident image `a3f37e8a…`.

| Field | Value |
| --- | --- |
| SHA-256 | `ba47ecba0bf61a6b3aae8218e9cfac0e637bd3d143efb60b8b75d4fd752d478d` |
| Bytes | 1,953,267,716 |
| Archive | `/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/ss-03-broker-archive-20260919/broker.jsonl` |
| Authority | **superseded / non-authoritative** |
| Last sampled build | `d13cd520dafbc3e7d22955a5a4b71cbe8a61f8009f02414873fc756d2c83c8d3` |
| Last sampled UID | `545433931bd25436593630352d068363` |
| Resident at archive time | `a3f37e8a…` (not this log) |

Do not treat this log as current board identity.

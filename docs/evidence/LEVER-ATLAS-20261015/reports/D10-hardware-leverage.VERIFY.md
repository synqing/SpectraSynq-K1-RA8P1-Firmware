# D10 hardware-leverage: adversarial verification (15 Oct 2026)

## In plain English

The D10 report was never written to disk, because the harness blocked the write. I recovered its full structured result from the workflow journal (`wf_2a73b8fd-675/journal.jsonl`, line 26, agent `a760f58e37385b297`) and checked it against source.

Most of the important claims hold up:

- **The microphone's bass-cut filter.** The coefficients are exactly the chip's reset values. The manual's note on them is "cut-off = 100 Hz". The manual also says the filters after the sinc stage were designed for 32 kHz and that their behaviour changes at other rates. The product runs them at about 82.8 kHz. The claimed cut-off of about 259 Hz is therefore well supported as an inference, but it has not been measured on the device.
- **Other confirmed findings:**
  - the 16-bit PCM width;
  - the ×16 gain and its loud-guard threshold of 1,875;
  - the second microphone being thrown away;
  - no sound-detection set-up;
  - no WS2816 degamma on the Titan;
  - Emotiscope modes defaulting to 8-bit output;
  - the 100 Hz frame period.

Two claims are wrong:

- **C-7 has the DMA build flag backwards.** The `led-first` map is the one that gives the microphones DMA 1/2. The default `pdm-first` map gives them DMA 0/1.
- **C-8's "evidence of drift" is an overreach.** It rests on 41,408 Hz against 41,417 Hz, two numbers taken from different captures.

Three "origin" labels have no supporting evidence: HW-09, HW-16 and, partly, HW-07.

The headline figure of "about 28% CPU" is an estimate built from per-stage cycle counts. Nothing measured the idle time.

## Source identity
- Titan worktree `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-emotiscope-port-001`, branch `lane/titan-lagfree-20261014`, HEAD `1ef53952`. The producer cited HEAD 81ac031; the platform files I read are the current ones.
- I read `hal_data.c` from the staged BSP copy `_scratch/waveform-migration-001/arm-p6-v8-postchange-002/stage/ra_gen/hal_data.c`.
- I read `r_pdm_api.h` from `docs/emotiscope-port-001/evidence/arm-palette-002/stage/ra/fsp/inc/api/r_pdm_api.h`.
- RA8P1 manual text: `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-PRISM/Kx-Titan/_scratch/pdm-profile/hardware-manual.txt` (R01UH1064EJ0130 Rev.1.30).

## Claim checks

| # | D10 claim | Verdict | Evidence |
|---|---|---|---|
| 1 | HPF coefficients at hal_data.c:617-619 are s0 0x3F61, k1 0x3EC1, h {0x4000,0xC000} | CONFIRMED [FACT] | The staged hal_data.c has them at lines 617-619. The manual reset values match: 0x00003F61 and 0x00003EC1 (manual lines 210184 and 210188). |
| 2 | The manual note says the reset value means "cut-off frequency = 100 Hz" | CONFIRMED [FACT] | Manual §50.2.21, line 211471: "Note 1. High-pass filter cut-off frequency = 100 Hz". |
| 3 | The HPF runs at 2·Fout, so fc is about 259 Hz at the product rate | CONFIRMED as [INFERENCE], strengthened | §50.3.1.1 (line 212795): `Fout = Fclk/(M×2)`. §50.3.3 says to set SINCDEC so that "the sampling frequency after the decimation can be 32 kHz", and Note 1 says the characteristics of the filters after the sinc stage "might change" at other rates. A fixed-coefficient filter's cut-off scales with its sample rate: 100 × 82,816/32,000 = 258.8 Hz. The dB figures assume a first-order response (110/√(110²+259²) gives −8.2 dB). A higher-order filter would cut more. The cut-off has not been measured on the device. |
| 4 | pdm_target.c:764-781 copies p_extend and overrides only sincdec/sincrng | CONFIRMED, with a small omission | It also overrides `interrupt_threshold` (8). None of the HPF, compensation or LPF fields is touched. The pdm_target.h comment "BSP sincdec 124 was 16 kHz at 2 MHz" is inconsistent with the manual formula: 2 MHz/250 = 8 kHz. The source comment is wrong, not D10. |
| 5 | SINCDEC/SINCRNG at pdm_target.h:21-24 | CONFIRMED | Line 21 is `SINCDEC_40K 49u`; line 24 is `SINCRNG_40K 10u`. |
| 6 | 16-bit width at hal_data.c:643; FSP offers 20-bit at r_pdm_api.h:43 | CONFIRMED | Line 643 is `PDM_PCM_WIDTH_16_BITS_0_14`; line 43 is `PDM_PCM_WIDTH_20_BITS_0_18`. |
| 7 | Only the programme lane is copied into capture_pcm (pdm_target.c:608-610) | CONFIRMED | Line 609, `capture_pcm[index] = view.sample;`, sits inside `if (lane == K1_PDM_TARGET_PROGRAMME_LANE)`. |
| 8 | ×16 gain (Q8 4096); the guard trips when the raw peak exceeds 1,875; `capture_last_hop_gain_q8` at :626 | CONFIRMED | `k1_pdm_sensitivity.h` sets SENSITIVITY_Q8 to 4096 and CEILING to 30000. The trip point is 30000·256/4096 = 1,875. The header comment reads "Restore the DualMCU ×16 analogue". The assignment is at pdm_target.c:626. |
| 9 | Sound detection is not configured; the FSP API exists | CONFIRMED | There are 0 `sound_detection` hits in pdm_target.c. r_pdm_api.h:66 defines `PDM_EVENT_SOUND_DETECTION`. |
| 10 | No WS2816 degamma on Titan; the K1 S3 has `k1_ws2816_degamma.h` (commit 38428a82) | CONFIRMED, with a caveat | There are 0 "degamma" hits in platform/ or src/. In the K1 repo the file exists, was added in 38428a82, and is used by `k1_look.h`. **Caveat for HW-06:** the Emotiscope donor already applies its own gamma (`emotiscope_port.cpp:134` `apply_gamma_correction`, `post_gamma_rgb`). Adding WS2816 degamma to those modes stacks two curves. D10 does not mention this interaction. |
| 11 | Emotiscope output_precision defaults to 0 → RGB8 path (palette_runtime.cpp:411-419; emotiscope_control.h:299,352,396) | CONFIRMED | Lines 299, 352 and 396 set `output_precision = 0`. palette_runtime.cpp selects `candidate?emotiscopeWirePixel16:emotiscopeWirePixel`. |
| 12 | kPalettePeriodUs is 10000 under K1_EMOTISCOPE_PORT (palette_runtime.h:39-40); measured 100.01 fps | CONFIRMED | palette_runtime.h:39-40. `docs/evidence/P5/device-budget/BUDGET.md:81` records 47,505 frames over 474.99 s and 0 skipped. |
| 13 | hal_entry.c:258 "No SecondaryCoreStart"; R_USB_Open at :262 | CONFIRMED, lines off by 1-2 | The comment is at line 259 and R_USB_Open at line 260. |
| 14 | Slot is 296 samples at pdm_target.h:16; 7.15 ms at 41.4 kHz | CONFIRMED | pdm_target.h:16. BUDGET.md:25 gives 7.148 ms. |
| 15 | G6 NO_QUALIFYING_CANDIDATE at STATUS.md:262 | CONFIRMED | `docs/evidence/K1-RA8P1-002/STATUS.md:262`. |
| 16 | The top output uses GPT7/12, DMA6/7 and ELC E-H, and is default-disabled (ws281x_gpt_dma_hw_top.c:1-45) | CONFIRMED | Lines 2-3 and 40/44. P714 is GPT12B at line 231. |
| 17 | C-7: "overridden to 1/2 only under dmac-lane-map pdm-first (build_scalar.py:850-852); led-first uses 0/1 (:863-865)" | **REFUTED** | The labels are reversed. Lines 846-853 are the `led-first` branch: LED DMA0, PDM RISE 1, PDM FALL 2. That matches the mandate. Lines 862-866 are the `else` (pdm-first) branch: PDM RISE 0, FALL 1. The default is `pdm-first` (line 575), the same as the pdm_target.h:44-48 defaults of 0/1. The real contradiction: the default map breaks the mic DMA1/2 mandate unless the product build passes `--dmac-lane-map led-first`. |
| 18 | PL14-A4 (ITCM 2e550b62 did not boot), A6 (41-47 s dark), B5 (0 q-register vfma), B12 (MOCO, 41,417 Hz; DTCM −4.5% render, −2.4% analysis) | CONFIRMED | Kx-Titan `docs/migration/TITAN-PRODUCT-LANE-LESSONS-2026-10-14.md` lines 64, 73, 116-117 and 144-147. Also `docs/evidence/P4/RECEIPT-P4-iteration1.md`. |
| 19 | In-house GDFT is about 0.70 M cycles per hop | CONFIRMED (assumes a 1 GHz clock) | BUDGET.md:40 gives an in-house median of 0.700 ms on BOOM. |
| 20 | C-8: 41,408 against 41,417 Hz is "evidence of drift" | **REFUTED as stated** | Both numbers exist: BUDGET.md:25 and PL14-B12. They come from different captures and different estimators. A 9 Hz (0.02%) difference between sessions does not show drift within a run. D10's own HW-04 experiment is the right way to test drift. |

### Not checked or unverifiable
- "ITCM 0 bytes, SDRAM 0 bytes, DTCM 69 KB (P4-corrections rtthread.map)": there is no `.map` file under `docs/evidence/P4`. UNVERIFIABLE here.
- "M85 about 28% busy, 6.5-7 ms free": this is an INFERENCE assembled from stage costs (AP about 0.98 ms per 5 ms hop, plus pack and render). No idle-loop counter exists, and D10's own open questions admit this. The summary presents it as a headline fact. It should be labelled an estimate.
- Schematic facts (LSM6DS3TR-C on I2C1 0x6A with INT1 on P412; ES8156 → LPA2103 → J4; U13/U14 mics) and PLATFORM_MEMORY: I did not find the schematic or the PLATFORM_MEMORY file in this pass. UNVERIFIED.
- WS2816C datasheet p.1 "internal 4-bit gamma" and p.4 timing: the datasheet was not opened. The K1 header comment `HARDWARE_NATIVE_4BIT` is indirect support only.

## Origins asserted without evidence
- **HW-09** ("one sample per frame", origin "S3 render budget (FastLED era)"): no S3 source, commit or budget figure is cited. The renderer may be single-sample for aesthetic or simplicity reasons. Status: unknown.
- **HW-16** ("short state only", origin "S3 RAM budget"): no S3 evidence that song-scale memory was rejected for RAM reasons. It may never have been designed. Status: unknown.
- **HW-07** ("S3 budget: 8-bit LEDs and transport"): this is better described as inherited donor hardware (Emotiscope on 8-bit WS2812) than a CPU budget. The origin category is mislabelled, though the substance is plausible.
- Supported origins:
  - HW-01 (accident): only SINCDEC/SINCRNG were retuned (pdm_target.c:764-781).
  - HW-02 (inherited): the header says "Restore the DualMCU ×16 analogue".
  - HW-15: the hal_entry comment.
  - HW-17: PL14-A4.

## Effect on decisions
- **HW-01 survives and is the top lever.** The manual itself warns that the post-sinc filters change away from the 32 kHz design rate. Next step: a host model of the Table 50.12 HPF, then the paired-lane sweep D10 proposes.
- **HW-06 needs an extra arm.** The Emotiscope modes already carry donor gamma, so test degamma on the in-house (TRUE16) modes separately from the Emotiscope modes.
- **C-7 must be corrected in REPORT.md before anyone acts on it.** The safe mapping is `led-first`.

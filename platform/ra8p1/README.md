# RA8P1 Platform Boundary

Only hardware/RTOS/toolchain integration belongs here. Shared K1 source under `src/k1/` must remain free of Renesas, FSP, RT-Thread, J-Link and board-specific headers.

The first backend must eventually provide:

1. boot and monotonic timer;
2. canonical `MEDIA_TIME_48K` observations at the audio capture boundary;
3. PDM and/or SSIE/TDM capture through bounded DMA;
4. an explicit `(media_frame, monotonic_time, uncertainty)` observation;
5. render-cost measurement;
6. physical LED output/latch cost;
7. fault/overrun and epoch-break reporting.

Do not copy RT1062 timing constants into this backend. Measure or analytically derive RA8P1-specific values.

For K1-RA8P1-001, execute on the M85 only. Do not split work onto M33 and do not use Ethos-U55.

#pragma once

/* Compile-time DMAC ownership. PDM rise/fall and LED pair A/B must be four
   distinct channels with matching IRQs. Include after channel macros exist. */

#if defined(K1_PALETTE_WS2816_GPT_PAIR) && defined(K1_PDM_TARGET)

#if (K1_WS281X_GPT_DMA_CHANNEL == K1_PDM_TARGET_RISE_DMA_CHANNEL) \
    || (K1_WS281X_GPT_DMA_CHANNEL == K1_PDM_TARGET_FALL_DMA_CHANNEL) \
    || (K1_WS281X_GPT_DMA_CHANNEL_A1 == K1_PDM_TARGET_RISE_DMA_CHANNEL) \
    || (K1_WS281X_GPT_DMA_CHANNEL_A1 == K1_PDM_TARGET_FALL_DMA_CHANNEL) \
    || (K1_WS281X_GPT_DMA_CHANNEL == K1_WS281X_GPT_DMA_CHANNEL_A1) \
    || (K1_PDM_TARGET_RISE_DMA_CHANNEL == K1_PDM_TARGET_FALL_DMA_CHANNEL)
#error "PDM and LED pair share a DMAC channel"
#endif

#endif

#if defined(K1_P601_LOOPBACK)
#if (4u == K1_WS281X_GPT_DMA_CHANNEL) || (4u == K1_WS281X_GPT_DMA_CHANNEL_A1) \
    || (defined(K1_PDM_TARGET_RISE_DMA_CHANNEL) && 4u == K1_PDM_TARGET_RISE_DMA_CHANNEL) \
    || (defined(K1_PDM_TARGET_FALL_DMA_CHANNEL) && 4u == K1_PDM_TARGET_FALL_DMA_CHANNEL)
#error "P601 loopback DMAC4 collides with LED or PDM"
#endif
#endif

#if defined(K1_PALETTE_WS2816_GPT_PAIR)
#ifndef VECTOR_NUMBER_DMAC2_INT
#error "DMAC2 vector is required for LED lane A"
#endif
#endif

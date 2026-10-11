#pragma once
#include "emotiscope_port.h"
#ifdef __cplusplus
extern "C" {
#endif
bool k1_secondary_emotiscope_init(uint32_t seed);
bool k1_secondary_emotiscope_set_config(const K1EmotiscopeConfig *config);
void k1_secondary_emotiscope_get_config(K1EmotiscopeConfig *config);
bool k1_secondary_emotiscope_patch(const K1EmotiscopePatch *patch,uint32_t *changed,uint32_t *mode_transition);
bool k1_secondary_emotiscope_set_palette(uint32_t id);
uint32_t k1_secondary_emotiscope_palette(void);
float k1_secondary_emotiscope_effective_hue(void);
bool k1_secondary_emotiscope_set_response_gain(float gain);
float k1_secondary_emotiscope_response_gain(void);
bool k1_secondary_emotiscope_set_edge_response(uint32_t mode,float depth);
uint32_t k1_secondary_emotiscope_edge_response(void);
float k1_secondary_emotiscope_edge_depth(void);
bool k1_secondary_emotiscope_accept_audio(const K1EmotiscopeAudioPublication *publication);
bool k1_secondary_emotiscope_render(uint32_t now_us,uint8_t rgb[384]);
void k1_secondary_emotiscope_last_rgb16(uint16_t rgb[384]);
void k1_secondary_emotiscope_snapshot(K1EmotiscopeSnapshot *snapshot);
#ifdef __cplusplus
}
#endif

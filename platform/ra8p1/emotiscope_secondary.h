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
bool k1_secondary_emotiscope_prepare_frame(uint32_t now_us);
bool k1_secondary_emotiscope_set_edge_colour(const K1EmotiscopeEdgeColourConfig *config);
void k1_secondary_emotiscope_get_edge_colour(K1EmotiscopeEdgeColourConfig *out);
bool k1_secondary_emotiscope_set_edge_pair(const K1EmotiscopeEdgePairConfig *config);
void k1_secondary_emotiscope_get_edge_pair(K1EmotiscopeEdgePairConfig *out);
uint32_t k1_secondary_emotiscope_effective_edge_pair(void);
bool k1_secondary_emotiscope_set_musical_config(const K1EmotiscopeMusicalConfig *config);
void k1_secondary_emotiscope_get_musical_config(K1EmotiscopeMusicalConfig *out);
bool k1_secondary_emotiscope_set_ml_config(const K1EmotiscopeMlConfig *config);
void k1_secondary_emotiscope_get_ml_config(K1EmotiscopeMlConfig *out);
bool k1_secondary_emotiscope_set_scene_config(const K1ComposedSceneConfig *config);
void k1_secondary_emotiscope_get_scene_config(K1ComposedSceneConfig *out);
void k1_secondary_emotiscope_scene_status(K1ComposedSceneStatus *out);
void k1_secondary_emotiscope_scene_prepare(uint32_t now_us);
void k1_secondary_emotiscope_get_ml_result(K1EmotiscopeMlResult *out);
void k1_secondary_emotiscope_accept_ml(const K1EmotiscopeMlResult *result);
void k1_secondary_emotiscope_musical_state(uint32_t *cursor,float *value);
bool k1_secondary_emotiscope_accept_audio(const K1EmotiscopeAudioPublication *publication);
bool k1_secondary_emotiscope_render(uint32_t now_us,uint8_t rgb[384]);
void k1_secondary_emotiscope_last_rgb16(uint16_t rgb[384]);
void k1_secondary_emotiscope_snapshot(K1EmotiscopeSnapshot *snapshot);
#ifdef __cplusplus
}
#endif

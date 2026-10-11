#pragma once
#include <stdint.h>

/* Scene authoring is independent of legacy mode, Edge colour linking and
 * hardware gain. Values are requested state; motion values latch at a gesture
 * boundary. The two boards retain separate copies. */
typedef struct {
  uint32_t scene;                 /* 0 legacy, 1 ribbons, 2 call/answer, 3 bloom */
  float outgoing_width;           /* fraction of centre-to-edge radius */
  float outgoing_ms;
  float separation_ms;
  float quiet_ms;
  float return_width;
  float return_ms;
  float return_strength;          /* fraction of captured call strength */
  float trail_ms;
  uint32_t trips;                 /* 0, 1 or 2 complete returns */
  float outgoing_feather;
  float return_feather;
  float recovery_ms;
  uint32_t choreography;          /* 0 independent, 1 call/answer, 2 counter */
  uint32_t initiator;             /* 0 bottom, 1 top; pair-owned */
  float answer_delay_ms;
  uint32_t phrase;                /* 0 off, 1 auto, 2 hold, 3 release, 4 rest */
  uint32_t ml_target;             /* 0 off, 1 width, 2 palette, 3 answer detail */
  uint32_t quiet_scope;           /* 0 material, 1 board, 2 pair, 3 wash exempt */
} K1ComposedSceneConfig;
typedef struct {
  uint32_t active_scene, phase, pending, gesture_id, phrase_state, quiet;
} K1ComposedSceneStatus;

#ifdef __cplusplus
#include <cstring>
inline K1ComposedSceneConfig k1_composed_scene_defaults() {
  return {0u,.14f,1400.0f,350.0f,450.0f,.10f,2200.0f,.45f,120.0f,
          1u,.3f,.5f,500.0f,0u,0u,180.0f,0u,0u,1u};
}
inline bool k1_composed_scene_range(float x,float low,float high) {
  uint32_t bits;std::memcpy(&bits,&x,sizeof(bits));
  return (bits&0x7f800000u)!=0x7f800000u && x>=low && x<=high;
}
inline bool k1_composed_scene_valid(const K1ComposedSceneConfig &c) {
  return c.scene<=3u && c.trips<=2u && c.choreography<=2u &&
    c.initiator<=1u && c.phrase<=4u && c.ml_target<=3u && c.quiet_scope<=3u &&
    k1_composed_scene_range(c.outgoing_width,.02f,.8f) &&
    k1_composed_scene_range(c.return_width,.02f,.8f) &&
    k1_composed_scene_range(c.outgoing_ms,200.0f,8000.0f) &&
    k1_composed_scene_range(c.return_ms,200.0f,8000.0f) &&
    k1_composed_scene_range(c.separation_ms,0.0f,5000.0f) &&
    k1_composed_scene_range(c.quiet_ms,0.0f,8000.0f) &&
    k1_composed_scene_range(c.return_strength,0.0f,.95f) &&
    k1_composed_scene_range(c.trail_ms,0.0f,2000.0f) &&
    k1_composed_scene_range(c.outgoing_feather,0.0f,1.0f) &&
    k1_composed_scene_range(c.return_feather,0.0f,1.0f) &&
    k1_composed_scene_range(c.recovery_ms,0.0f,8000.0f) &&
    k1_composed_scene_range(c.answer_delay_ms,0.0f,8000.0f);
}
#endif

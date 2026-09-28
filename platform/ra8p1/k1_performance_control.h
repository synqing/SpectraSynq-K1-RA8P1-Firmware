#pragma once
// THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS.
// Finite immutable targets, applied exclusively by the foreground RT owner.
#include "k1_live_protocol.h"
#include <cmath>

namespace k1::titan {
inline constexpr std::uint32_t kLiveCfgPerformance = 9U;
inline constexpr std::uint32_t kLiveCfgPerformanceScene = 10U;
inline constexpr std::uint32_t kLiveCfgPerformanceAudition = 11U;
inline constexpr std::uint32_t kLiveCfgPerformanceSelection = 12U;
inline bool performanceMode(std::uint32_t mode) noexcept {
  constexpr std::uint32_t modes[]{8,7,11,3,9,12,13,14,15,21,24,26,22,23};
  for (auto value : modes) if (mode == value) return true;
  return false;
}
struct PerformanceAudition final {
  bool active=false; float original=1.F; std::uint64_t expires_us=0;
  void renew(float previous, std::uint64_t now) noexcept {
    if (!active) original=previous;
    active=true; expires_us=now+750000U;
  }
};
inline constexpr std::uint16_t kPerfLevel = 0x101U, kPerfTonal = 0x107U;
inline constexpr std::uint16_t kPerfWarmth = 0x201U, kPerfBulb = 0x202U;
inline constexpr std::uint16_t kPerfNovelty=0x110U,kPerfBass=0x111U,kPerfKick=0x112U,
    kPerfSnare=0x113U,kPerfHihat=0x114U,kPerfTransient=0x115U,kPerfMood=0x116U,kPerfLow=0x117U;
inline constexpr std::uint16_t kPerformanceFields[16] = {
    kPerfLevel, kPerfTonal, kPatchVisChroma, kPatchVisFallbackBrightness,
    kPatchVisShiftRate, kPatchVisIdleFade, kPerfWarmth, kPerfBulb,
    kPerfNovelty,kPerfBass,kPerfKick,kPerfSnare,kPerfHihat,kPerfTransient,kPerfMood,kPerfLow};
inline float* performanceField(LiveConfigBlob& cfg, std::uint16_t field) noexcept {
  switch (field) {
    case kPerfLevel: return &cfg.focus_a[0];
    case kPerfTonal: return &cfg.focus_a[5];
    case kPatchVisChroma: return &cfg.visual_a.chroma;
    case kPatchVisFallbackBrightness: return &cfg.visual_a.vp_waveform_fallback_brightness;
    case kPatchVisShiftRate: return &cfg.visual_a.vp_waveform_shift_rate;
    case kPatchVisIdleFade: return &cfg.visual_a.vp_waveform_idle_fade;
    case kPerfWarmth: return &cfg.visual_a.incandescent_filter;
    case kPerfBulb: return &cfg.visual_a.bulb_opacity;
    case kPerfNovelty:return &cfg.focus_a[1];case kPerfBass:return &cfg.focus_a[7];
    case kPerfKick:return &cfg.focus_a[10];case kPerfSnare:return &cfg.focus_a[11];
    case kPerfHihat:return &cfg.focus_a[12];case kPerfTransient:return &cfg.focus_a[9];
    case kPerfMood:return &cfg.visual_a.mood;case kPerfLow:return &cfg.focus_a[2];
    default: return nullptr;
  }
}
inline unsigned performanceIndex(std::uint16_t field) noexcept {
  for(unsigned i=0;i<16;++i)if(kPerformanceFields[i]==field)return i;
  return 16;
}
inline bool performanceFocus(std::uint16_t id) noexcept {
 return id==kPerfLevel||id==kPerfTonal||id==kPerfNovelty||id==kPerfBass||id==kPerfKick||id==kPerfSnare||id==kPerfHihat||id==kPerfTransient||id==kPerfLow;
}
inline bool performanceAuditionField(std::uint16_t id) noexcept {
 return id==kPerfLevel||id==kPerfNovelty||id==kPerfKick||id==kPerfSnare||id==kPerfHihat||id==kPerfTransient;
}
inline bool performanceConsumes(std::uint32_t mode,std::uint16_t field) noexcept {
  // Executed production-renderer buffer sensitivity: test_performance_consumers.
  unsigned mask=0;
  switch(mode) {
    case 3U:mask=16582U;break;
    case 7U:mask=255U;break;
    case 8U:mask=255U;break;
    case 9U:mask=16582U;break;
    case 11U:mask=223U;break;
    case 12U:mask=16582U;break;
    case 13U:mask=17088U;break;
    case 16U:mask=195U;break;
    case 18U:mask=199U;break;
    case 21U:mask=25027U;break;
    case 22U:mask=1475U;break;
    case 23U:mask=33987U;break;
    case 24U:mask=25027U;break;
    case 25U:mask=195U;break;
    case 26U:mask=7360U;break;
    case 32U:mask=199U;break;
    default:return false;
  }
  const auto index=performanceIndex(field);return index<16 && (mask&(1U<<index));
}
inline bool performancePalette(std::uint32_t id) noexcept {
  return id==33U || id==36U || id==37U || id==38U || id==40U || id==41U || id==42U || id==43U;
}
inline LivePatchStatus liveApplyPerformance(LiveConfigBlob* cfg, std::uint32_t revision,
    std::uint32_t boot, std::uint16_t channel, const LivePatchEntry* entries,
    std::uint16_t count) noexcept {
  if (!cfg || !entries || !count || count>kLivePatchMaxEntries) return LivePatchStatus::Invalid;
  if (!boot || boot!=cfg->boot_id) return LivePatchStatus::WrongBoot;
  if (revision!=cfg->revision) return LivePatchStatus::Stale;
  if (channel || !performanceMode(cfg->mode_a) || cfg->mode_b!=cfg->mode_a || cfg->output_channel!=0U)
    return LivePatchStatus::Forbidden;
  for (unsigned i=0; i<count; ++i) {
    const auto id=entries[i].field_id; const float value=entries[i].value;
    if (!performanceField(*cfg,id) || !performanceConsumes(cfg->mode_a,id)) return LivePatchStatus::Forbidden;
    if (id==kPerfWarmth && (cfg->visual_a.flags & (kVisFixSecondary|kVisIncandescent)))
      return LivePatchStatus::Forbidden;
    const float lo=id==kPatchVisIdleFade ? .90F : 0.F;
    const float hi=id==kPatchVisShiftRate ? 240.F : id==kPatchVisIdleFade ? .999F :
                   performanceFocus(id) ? 2.F : 1.F;
    if (entries[i].kind!=kLivePatchKindF32 || !std::isfinite(value) || value<lo || value>hi)
      return LivePatchStatus::Invalid;
    for (unsigned j=0;j<i;++j) if (entries[j].field_id==id) return LivePatchStatus::Invalid;
  }
  for (unsigned i=0;i<count;++i) *performanceField(*cfg,entries[i].field_id)=entries[i].value;
  ++cfg->revision;
  return LivePatchStatus::Ok;
}
struct PerformanceRamp final {
  bool active=false;
  std::uint64_t last_us=0;
  void start(std::uint64_t now) noexcept { active=true; last_us=now; }
  void cancel() noexcept { active=false; }
  // cfg carries acknowledged TARGETS; rendered carries current consumer values.
  // A rendered receipt means the target's ramp was consumed, not yet settled.
  bool step(LiveConfigBlob& rendered, LiveConfigBlob& target, std::uint64_t now) noexcept {
    if (!active || now<=last_us || now-last_us<1000U) return false;
    const float dt=static_cast<float>(now-last_us)*1e-6F; last_us=now;
    const float fast=1.F-std::exp(-dt/.08F), focus=1.F-std::exp(-dt/.10F), slow=1.F-std::exp(-dt/.12F);
    bool remaining=false;
    for (auto id:kPerformanceFields) {
      float& value=*performanceField(rendered,id); const float wanted=*performanceField(target,id);
      const float alpha=id==kPatchVisShiftRate ? fast : (performanceFocus(id) || id==kPatchVisFallbackBrightness) ? focus : slow;
      value += (wanted-value)*alpha;
      const float epsilon=id==kPatchVisShiftRate ? .001F : 1e-6F;
      if (std::fabs(wanted-value)<=epsilon) value=wanted; else remaining=true;
    }
    active=remaining;
    return true;
  }
};
} // namespace k1::titan

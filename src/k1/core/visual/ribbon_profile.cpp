#include "core/visual/ribbon_profile.h"
#include <algorithm>
#include <cmath>

namespace k1::core::visual {
namespace {
RibbonSettings effective(const RibbonSettings& base, float liveiness) noexcept {
  auto result=base;
  // Neutral at 0.5, half/twice base at the endpoints. Saturate only transport
  // at the material's declared physical range; never rewrite the base value.
  result.velocity_px_s=std::clamp(base.velocity_px_s*std::exp2(2*liveiness-1),4.F,240.F);
  return result;
}
}

bool RibbonProfile::configure(const RibbonSettings& base, std::uint64_t time) noexcept {
  if(!validRibbonSettings(base) || !material_.configure(effective(base,liveiness_),time)) return false;
  base_=base;
  return true;
}

bool RibbonProfile::setLiveiness(float value, std::uint64_t time) noexcept {
  if(!std::isfinite(value) || value<0 || value>1 ||
      !material_.configure(effective(base_,value),time)) return false;
  liveiness_=value;
  return true;
}

RibbonNativeFrame RibbonProfile::render(std::uint64_t time, std::uint8_t brightness) noexcept {
  const auto& pixels=material_.render(time,std::min(brightness,std::uint8_t{24})/255.F);
  return RibbonNativeFrame(pixels,material_.telemetry().frame_valid);
}

bool RibbonNativeFrame::stage(std::uint8_t* a, std::size_t na,
                              std::uint8_t* b, std::size_t nb) const noexcept {
  if(!valid_ || !a || !b || na!=kPackedBytesPerLane || nb!=kPackedBytesPerLane) return false;
  // Reject overlapping destinations before writing either half. uintptr_t
  // differences avoid relational comparisons of unrelated object pointers.
  const auto pa=reinterpret_cast<std::uintptr_t>(a), pb=reinterpret_cast<std::uintptr_t>(b);
  if(pa<=pb ? pb-pa<na : pa-pb<nb) return false;
  return splitChannel160(pixels_->data(),pixels_->size(),a,b);
}
} // namespace k1::core::visual

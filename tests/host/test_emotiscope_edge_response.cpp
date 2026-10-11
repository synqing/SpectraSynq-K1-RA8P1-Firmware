#include "emotiscope_edge_response.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

using Frame = std::array<float, 384>;
static void require(bool value, const char* message) {
  if (!value) { std::fprintf(stderr, "EDGE_RESPONSE_FAILURE %s\n", message); std::exit(2); }
}
static Frame colour() {
  Frame frame{};
  for (unsigned i = 0; i < 128; ++i) {
    frame[3*i] = 0.8f; frame[3*i+1] = 0.3f; frame[3*i+2] = 0.1f;
  }
  return frame;
}
static void same(const Frame& a, const Frame& b, const char* message) {
  require(std::memcmp(a.data(), b.data(), sizeof(a)) == 0, message);
}
static void near(float a, double b, const char* message) {
  require(std::fabs(static_cast<double>(a)-b) < 2e-6, message);
}
int main() {
  EmotiscopeEdgeResponse a, b;
  K1EmotiscopeAudioPublication p{}; p.valid = 1; p.vu_level = 0.8f;
  p.novelty_normalised = 0.7f;
  const Frame source = colour(); Frame frame = source;
  require(a.apply(frame.data(), p, 0.01f), "default rejected");
  same(frame, source, "default is not identity");
  require(a.configure(1, 0), "zero configure");
  a.apply(frame.data(), p, 0.01f); same(frame, source, "depth0 changed pixels");
  require(a.configure(1, 0.65f), "novelty configure");
  // Independent closed-form response for constant input, not the recurrence.
  for (unsigned step = 1; step <= 20; ++step) {
    frame = source; require(a.apply(frame.data(), p, 0.01f), "novelty apply");
    const double envelope = double(p.novelty_normalised) * (1.0-std::exp(-double(step)*0.01/0.040));
    const double scale = 1.0-double(a.depth())*double(p.vu_level)*(1.0-envelope);
    for (unsigned i = 0; i < 384; ++i) near(frame[i], double(source[i])*scale, "closed-form response");
  }
  // Equal elapsed time split into different frame schedules.
  require(b.configure(1, 0.65f), "second configure");
  Frame other = source;
  b.apply(other.data(), p, 0.2f);
  for (unsigned i = 0; i < 384; ++i) near(frame[i], other[i], "dt dependence");
  // Invalid configuration is atomic, including history.
  EmotiscopeEdgeResponse saved = a;
  for (float bad : {std::numeric_limits<float>::quiet_NaN(),
                    std::numeric_limits<float>::infinity(), -0.1f, 1.1f})
    require(!a.configure(2, bad), "bad depth accepted");
  require(!a.configure(3, 0.5f), "bad mode accepted");
  frame=source;other=source;a.apply(frame.data(),p,0.01f);saved.apply(other.data(),p,0.01f);
  same(frame,other,"invalid configure changed state");
  // Spectrum reference is an independent coordinate equation and closed form.
  require(a.configure(2,1.0f),"spectrum configure");
  for(unsigned i=0;i<64;++i)p.spectrum_smooth[i]=float(i)/63.0f;
  frame=source; a.apply(frame.data(),p,0.04f);
  for(unsigned pixel=0;pixel<128;++pixel){
    const double distance=std::fabs(double(pixel)-63.5)/63.5;
    const unsigned bin=static_cast<unsigned>(std::floor(63.0*std::pow(distance,0.7)));
    const double envelope=double(p.spectrum_smooth[bin])*(1.0-std::exp(-1.0));
    const double scale=1.0-double(p.vu_level)*(1.0-envelope);
    for(unsigned c=0;c<3;++c)near(frame[3*pixel+c],double(source[3*pixel+c])*scale,"spectral map/colour ratio");
    near(frame[3*pixel],frame[3*(127-pixel)],"centre mirror");
  }
  require(frame[0]>frame[3*63],"spectral contour absent");
  // One bad late bin cannot partially update the frame. Invalid observation
  // resets history: next result must match a clean instance.
  p.spectrum_smooth[63]=std::numeric_limits<float>::quiet_NaN();frame=source;
  require(!a.apply(frame.data(),p,0.01f),"nonfinite audio accepted");same(frame,source,"partial invalid frame");
  p.spectrum_smooth[63]=1.0f;EmotiscopeEdgeResponse clean;clean.configure(2,1.0f);
  frame=source;other=source;a.apply(frame.data(),p,0.01f);clean.apply(other.data(),p,0.01f);
  same(frame,other,"invalid audio did not reset history");
  p.valid=0;frame=source;require(!a.apply(frame.data(),p,0.01f),"invalid publication accepted");same(frame,source,"gap not pass-through");
  p.valid=1;frame=source;require(!a.apply(frame.data(),p,-1.0f),"negative dt accepted");same(frame,source,"bad dt changed frame");
  require(!a.apply(frame.data(),p,std::numeric_limits<float>::quiet_NaN()),"NaN dt accepted");
  // Render order and A-only changes cannot influence B, with unlike histories.
  EmotiscopeEdgeResponse a1,b1,a2,b2;a1.configure(1,.65f);a2.configure(1,.65f);b1.configure(2,.9f);b2.configure(2,.9f);
  for(unsigned i=0;i<12;++i){
    Frame fa=source,fb=source,ra=source,rb=source;
    p.novelty_normalised=float(i%3)*0.3f;
    a1.apply(fa.data(),p,.01f);b1.apply(fb.data(),p,.01f);
    b2.apply(rb.data(),p,.01f);a2.apply(ra.data(),p,.01f);
    same(fa,ra,"A depends on render order");same(fb,rb,"B depends on render order");
    if(i==4){a1.configure(2,.2f);a2.configure(2,.2f);}
  }
  // Range clipping never amplifies; black has no colour floor.
  a.configure(1,1);p.vu_level=4;p.novelty_normalised=-4;frame={};
  a.apply(frame.data(),p,0.0f);same(frame,Frame{},"black became light");
  std::printf("Edge response reference, invalid input, geometry and state isolation passed; sizeof=%zu\n",sizeof(a));
}

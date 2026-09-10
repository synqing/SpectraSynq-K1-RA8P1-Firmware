#include "core/visual/channel_render_state.h"
#include "core/visual/product_output_treatment.h"
#include <cassert>
#include <cstdio>
using namespace k1::core;
using namespace k1::core::visual;
int main() {
  for (unsigned cell=0; cell<3; ++cell) {
    Pixel8 pixels[160]{};
    ChannelVisualControls controls;
    controls.prism_count=cell==0 ? 0.F : 1.F;
    controls.vp_fix_prism_off=cell==2;
    ProductOutputTreatmentState state;
    applyProductOutputTreatment(PixelSpan{pixels,160},controls,state);
    unsigned lit=0;
    for (unsigned i=0; i<160; ++i) {
      const auto p=pixels[i];
      if (p.red || p.green || p.blue) {
        ++lit;
        std::printf("cell=%u index=%u rgb=(%u,%u,%u)\n",cell,i,p.red,p.green,p.blue);
        assert(cell==1);
        assert((i==77 && p.red==48 && p.green==0 && p.blue==24) ||
               (i==82 && p.red==0 && p.green==48 && p.blue==24));
      }
    }
    assert(lit==(cell==1 ? 2U : 0U));
  }
  std::puts("PASS: pinned PRISM count=1 injects two fixed coloured points into black; count=0 and fix-prism-off preserve black.");
}

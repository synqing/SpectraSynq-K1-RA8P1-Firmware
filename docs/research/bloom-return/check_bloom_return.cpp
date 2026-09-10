#include "bloom_return_reference.h"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace bloom_reference;
static bool near(float a, float b) { return std::fabs(a-b) < 0.00001F; }

int main() {
  // One isolated seed. At radius 1/2 the four legs pass at .5T, 1.5T,
  // 2.5T and 3.5T. T=7200 ticks makes every probe land on the input grid.
  for (unsigned returns=0; returns<=2; ++returns) {
    BloomReturn engine;
    BloomReturn::Parameters p;
    p.travel_ticks=7200; p.returns=returns; p.edge_gain=.5F;
    p.centre_gain=.5F; p.half_life_ticks=7200.F;
    assert(engine.configure(p));
    for (std::int64_t now=0; now<=36000; now+=360) {
      assert(engine.push(0, now, now==0 ? Rgb{1,0,0} : Rgb{}) == BloomReturn::Push::accepted);
      Rgb a, b;
      assert(engine.render(now, .5F, a));
      // Intervening render calls must not change the image at the same time.
      assert(engine.render(now, .21F, b));
      assert(engine.render(now, .5F, b));
      assert(a.r==b.r && a.g==b.g && a.b==b.b);
      const float sum=returns==0 ? 1.F : returns==1 ? 1.5F : 1.875F;
      float expected=0;
      if (now==3600) expected=std::exp2(-.5F)/sum;
      if (now==10800 && returns>=1) expected=.5F*std::exp2(-1.5F)/sum;
      if (now==18000 && returns>=2) expected=.25F*std::exp2(-2.5F)/sum;
      if (now==25200 && returns>=2) expected=.125F*std::exp2(-3.5F)/sum;
      if (now%3600==0) assert(near(a.r,expected));
      assert(a.g==0 && a.b==0);
      // Endpoints: source appears at the edge first at T, never earlier.
      Rgb edge; assert(engine.render(now, 1.F, edge));
      if (now<7200) assert(edge.r==0);
      if (now==7200) assert(edge.r>0);
      if (now>28800) {
        for (unsigned r=0; r<80; ++r) {
          Rgb tail; assert(engine.render(now, r/79.F, tail));
          assert(tail.r==0 && tail.g==0 && tail.b==0);
        }
      }
    }
  }
  BloomReturn engine;
  BloomReturn::Parameters p; p.travel_ticks=96000; p.returns=2;
  assert(engine.configure(p));
  for (int i=0; i<2500; ++i) {
    assert(engine.push(0, i*360LL, {1,1,1}) == BloomReturn::Push::accepted);
    Rgb v; assert(engine.render(i*360LL, .5F, v));
    assert(v.r>=0 && v.r<=1.000001F && v.g==v.r && v.b==v.r);
  }
  assert(engine.sample_count()==1070);
  Rgb out;
  assert(!engine.render(2500*360LL, .5F, out)); // future input unavailable
  assert(engine.push(0,2499*360LL,{})==BloomReturn::Push::stale);
  assert(engine.push(0,2501*360LL,{})==BloomReturn::Push::gap_reset);
  assert(engine.sample_count()==1);
  engine.reset(7);
  assert(engine.sample_count()==0);
  assert(engine.push(0,0,{})==BloomReturn::Push::invalid);
  assert(engine.push(7,0,{0,1,0})==BloomReturn::Push::accepted);
  assert(!engine.render(0,std::numeric_limits<float>::quiet_NaN(),out));
  p.returns=3; assert(!engine.configure(p));
  // A later seed cannot appear before its own event timestamp. This catches
  // ordinary linear interpolation that would anticipate the event by one hop.
  BloomReturn causal;
  assert(causal.push(0,0,{})==BloomReturn::Push::accepted);
  assert(causal.push(0,360,{})==BloomReturn::Push::accepted);
  assert(causal.push(0,720,{1,0,0})==BloomReturn::Push::accepted);
  assert(causal.render(719,0.F,out) && out.r==0);
  assert(causal.render(720,0.F,out) && out.r>0);
  // Radial mapping is exact RGB symmetry for both bench and native geometry.
  for (unsigned n : {128U,160U}) {
    Rgb pixels[160]{};
    for (unsigned r=0; r<n/2; ++r) {
      assert(causal.render(720,static_cast<float>(r)/(n/2-1),out));
      pixels[n/2-1-r]=pixels[n/2+r]=out;
    }
    for (unsigned i=0; i<n; ++i) {
      const auto a=pixels[i], b=pixels[n-1-i];
      assert(a.r==b.r && a.g==b.g && a.b==b.b);
    }
  }
  std::printf("PASS: 0/1/2 returns, boundary timing, read-only cadence, palette channels, bounded history, silence tail, gap/epoch recovery. sizeof(engine)=%zu; seed payload=%zu bytes/channel\n", sizeof(engine), BloomReturn::kCapacity*sizeof(Rgb));
}

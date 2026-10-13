#include "palette_runtime.h"
#include "emotiscope_port.h"
#include "emotiscope_secondary.h"
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace k1::titan;
using namespace k1::core::visual;
int main(){
  assert(k1_emotiscope_init(1));assert(k1_secondary_emotiscope_init(2));
  static PaletteRuntime r;PaletteConfig c;c.mode_a=c.mode_b=1;c.palette_a=c.palette_b=0;c.flags=5;c.brightness=255;
  assert(r.configure(c,0));Pixel16 frame[160];
  for(unsigned i=0;i<160;++i)frame[i]={std::uint16_t(257*i+1),std::uint16_t(65500-211*i),std::uint16_t(97*i+3)};
  assert(r.stageWideFrame(0,frame,160));assert(r.stageWideFrame(1,frame,160));
  unsigned char before[2][2][480];
  for(unsigned board=0;board<2;++board)for(unsigned lane=0;lane<2;++lane){
    auto* out=before[board][lane];assert(r.packBoardGrb48Lane(out,480,board,lane)==480);
    for(unsigned i=0;i<80;++i){
      // Physical evidence: top DIN0 serves right, DIN1 left; order inside each half is retained.
      const auto p=frame[(board?(1-lane):lane)*80+i];unsigned expected[3]={p.green,p.red,p.blue};
      for(unsigned ch=0;ch<3;++ch)assert((unsigned(out[i*6+ch*2])<<8|out[i*6+ch*2+1])==expected[ch]);
    }
  }
  assert(!std::memcmp(before[0][0],before[1][1],480));assert(!std::memcmp(before[0][1],before[1][0],480));
  assert(r.setBoardIntensity(1,.25f));
  for(unsigned board=0;board<2;++board)for(unsigned lane=0;lane<2;++lane){unsigned char out[480];assert(r.packBoardGrb48Lane(out,480,board,lane)==480);
    for(unsigned i=0;i<240;++i){unsigned prev=unsigned(before[board][lane][i*2])<<8|before[board][lane][i*2+1];unsigned got=unsigned(out[i*2])<<8|out[i*2+1];assert(got==(board?(prev*16384u+32767u)/65535u:prev));}
  }
  unsigned char short_out[480];std::memset(short_out,0xa5,480);assert(!r.packBoardGrb48Lane(short_out,479,1,0));for(auto v:short_out)assert(v==0xa5);
  puts("physical top DIN half assignment and unchanged lower/gain/precision passed");
}

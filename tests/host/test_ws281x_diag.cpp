#include "ws281x_diag.h"
#include <cassert>
#include <cstring>
#include <cstdio>

int main() {
    uint8_t wire[K1_WS281X_DIAG_MAX_BYTES];
    k1_ws281x_diag_timing_t timing{};
    k1_ws281x_diag_request_t request{1,1,0,128,8,0x12,0x34,0x56};
    assert(k1_ws281x_diag_pack(&request,wire,sizeof(wire),&timing)==384);
    const uint8_t grb[]{0x34,0x12,0x56};
    assert(std::memcmp(wire,grb,3)==0 && timing.t1h_ns==875);
    for(unsigned i=24;i<384;++i) assert(wire[i]==0);
    request.profile=3; request.red=0x12ab; request.green=0x34cd; request.blue=0x56ef;
    assert(k1_ws281x_diag_pack(&request,wire,sizeof(wire),&timing)==768);
    const uint8_t grb16[]{0x34,0xcd,0x12,0xab,0x56,0xef};
    assert(std::memcmp(wire,grb16,6)==0 && timing.t1h_ns==650);
    assert(wire[3]!=wire[2]); // Low-byte loss must fail.
    request.lit_pixels=0;
    assert(k1_ws281x_diag_pack(&request,wire,sizeof(wire),&timing)==768);
    for(auto value:wire) assert(value==0);
    request.profile=1; request.lit_pixels=128; request.red=0x20; request.green=0; request.blue=0;
    assert(k1_ws281x_diag_pack(&request,wire,sizeof(wire),&timing)==384);
    for(unsigned i=0;i<384;i+=3) assert(wire[i]==0 && wire[i+1]==0x20 && wire[i+2]==0);
    const auto valid=request;
    for(unsigned mutation=0;mutation<9;++mutation) {
        request=valid;
        if(mutation==0) request.version=0;
        if(mutation==1) request.profile=99;
        if(mutation==2) request.pin=2;
        if(mutation==3) request.pixels=0;
        if(mutation==4) request.pixels=129;
        if(mutation==5) request.lit_pixels=129;
        if(mutation==6) request.red=65536;
        if(mutation==7) { request.profile=1; request.red=256; } // Reject 16-bit values, never truncate.
        if(mutation==8) { request.pixels=1; request.lit_pixels=2; }
        std::memset(wire,0xa5,sizeof(wire));
        assert(k1_ws281x_diag_pack(&request,wire,sizeof(wire),&timing)==0);
        for(auto value:wire) assert(value==0xa5);
    }
    request=valid;
    assert(k1_ws281x_diag_pack(&request,wire,383,&timing)==0);
    assert(k1_ws281x_diag_pack(nullptr,wire,sizeof(wire),&timing)==0);
    assert(k1_ws281x_diag_profile(2,&timing) && timing.t1h_ns==580 && timing.period_ns==1225);
    std::puts("K1_WS281X_DIAG=PASS grb24=PASS grb48=PASS rejected_mutations=9");
}

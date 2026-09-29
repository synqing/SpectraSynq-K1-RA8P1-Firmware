// Exercises the production 128->160 adapter and PaletteRuntime GRB48 packer.
#include "emotiscope_port.h"
#include "emotiscope_output.h"
#include "palette_runtime.h"
#include "hd_pixel16.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
using k1::titan::PaletteConfig;
using k1::titan::PaletteRuntime;
using k1::core::visual::Pixel16;
static void require(bool ok, const char* message) {
    if (!ok) { std::cerr<<"OUTPUT_CONTRACT_FAILURE "<<message<<"\n"; std::exit(2); }
}
static PaletteConfig config(unsigned brightness=100) {
    PaletteConfig c;
    c.palette_a=c.palette_b=0;
    c.mode_a=c.mode_b=1;
    c.flags=5;
    c.brightness=brightness;
    return c;
}
// Independent geometric nearest-neighbour specification, not the adapter formula.
static unsigned nearest(unsigned destination) {
    const long double x=static_cast<long double>(destination)/159.0L;
    long double best=2.0L;
    unsigned selected=128;
    for (unsigned source=0; source<128; ++source) {
        const long double distance=std::fabs(static_cast<long double>(source)/127.0L-x);
        if (distance<best) { best=distance; selected=source; }
    }
    return selected;
}
static uint16_t expected_wire(uint8_t donor, unsigned brightness) {
    // Fraction of full wire scale after one global gain, truncating to a code.
    return uint16_t((uint64_t(donor)*65535u*brightness)/(255u*255u));
}
static uint16_t channel(const uint8_t* wire, unsigned offset) {
    return uint16_t(unsigned(wire[offset])*256u+wire[offset+1]);
}
static void marker_case() {
    PaletteRuntime runtime;
    auto c=config();
    require(k1::titan::kDefaultPaletteBrightness>=100u,"boot brightness below mandate");
    require(runtime.configure(c,0),"mode 1 / brightness100 configure rejected");
    K1EmotiscopeConfig donor{};
    k1_emotiscope_get_config(&donor);
    require(donor.current_mode==1,"mode 1 was not handed to donor");
    auto invalid=c;
    invalid.brightness=99; require(runtime.configure(invalid,0),"explicit dimming99 rejected");
    invalid.brightness=0; require(runtime.configure(invalid,0),"explicit dimming0 rejected");
    require(runtime.configure(c,0),"restore100 after explicit dimming failed");
    invalid=c; invalid.brightness=256; require(!runtime.configure(invalid,0),"brightness256 accepted");
    invalid=c; invalid.mode_a=invalid.mode_b=8; require(!runtime.configure(invalid,0),"legacy mode8 accepted");
    invalid=c; invalid.output_channel=1; require(!runtime.configure(invalid,0),"second independent scene accepted");
    invalid=c; invalid.palette_b=1; require(!runtime.configure(invalid,0),"old palette selector accepted");
    invalid=c; invalid.flags=3; require(!runtime.configure(invalid,0),"old automatic palette cycle accepted");
    require(runtime.config().brightness==100 && runtime.config().mode_a==1,
            "rejected configuration altered active configuration");

    std::array<uint8_t,384> rgb{};
    for (unsigned p=0;p<128;++p) {
        rgb[p*3]=uint8_t(p*2);
        rgb[p*3+1]=uint8_t((p*53+19)%256);
        rgb[p*3+2]=uint8_t(255-p*2);
    }
    const unsigned special[]{0,63,64,127};
    const uint8_t colors[][3]{{255,1,128},{17,231,69},{213,32,7},{3,255,123}};
    for(unsigned i=0;i<4;++i) std::memcpy(rgb.data()+special[i]*3,colors[i],3);
    std::array<Pixel16,160> expanded{}, poison{};
    std::array<unsigned,128> coverage{};
    for (unsigned d=0;d<160;++d) {
        const unsigned s=nearest(d);
        require(k1::titan::emotiscopeSourceIndex(d)==s,"nearest geometry differs");
        ++coverage[s];
        expanded[d]=k1::titan::emotiscopeWirePixel(rgb.data(),d);
        poison[d]={65535,65535,65535};
    }
    // Candidate adapter: same nearest source, 16-bit value passed through without x257.
    std::array<uint16_t,384> rgb16{};
    for (unsigned i=0;i<384;++i) rgb16[i]=uint16_t(i*167u+3u);
    for (unsigned d=0;d<160;++d) {
        const Pixel16 px=k1::titan::emotiscopeWirePixel16(rgb16.data(),d);
        const unsigned s=nearest(d)*3u;
        require(px.red==rgb16[s] && px.green==rgb16[s+1] && px.blue==rgb16[s+2],"candidate mapping differs");
    }
    require(nearest(0)==0 && nearest(159)==127,"endpoints missing");
    require(nearest(79)==63 && nearest(80)==64,"centre pair wrong");
    for (unsigned n:coverage) require(n>=1,"native donor pixel lost");
    for (unsigned brightness:{0u,99u,100u,255u}) {
        c=config(brightness);
        require(runtime.configure(c,0),"legal brightness configure rejected");
        require(runtime.stageWideFrame(0,expanded.data(),160),"complete scene staging failed");
        require(runtime.stageWideFrame(1,poison.data(),160),"poison scene staging failed");
        for(unsigned lane=0;lane<2;++lane) {
            std::array<uint8_t,482> guarded;
            guarded.fill(0xa7);
            require(runtime.packBenchGrb48Lane(guarded.data()+1,480,lane)==480,
                    "DIN is not exactly80pixels480bytes");
            require(guarded[0]==0xa7 && guarded[481]==0xa7,"packer wrote outside lane");
            for(unsigned pixel=0;pixel<80;++pixel) {
                const unsigned source=nearest(lane*80+pixel);
                const uint8_t* wire=guarded.data()+1+pixel*6;
                require(channel(wire,0)==expected_wire(rgb[source*3+1],brightness),"GRB green/mapping/gain");
                require(channel(wire,2)==expected_wire(rgb[source*3],brightness),"GRB red/mapping/gain");
                require(channel(wire,4)==expected_wire(rgb[source*3+2],brightness),"GRB blue/mapping/gain");
            }
        }
    }
    std::array<uint8_t,480> lane{};
    require(runtime.packBenchGrb48Lane(lane.data(),479,0)==0,"short capacity accepted");
    require(runtime.packBenchGrb48Lane(lane.data(),480,2)==0,"invalid third lane accepted");
    std::cout << R"json({"source_pixels_covered":128,"logical_pixels":160,"pixels_per_din":80,"endpoints":true,"centre_63_64_to_79_80":true,"single_brightness_gain":true,"mode1_configured":true,"explicit_dimming_preserved":true,"rejected_legacy_config":true,"grb48_order":true})json" << "\n";
}
static void generate(const std::string& kind,const char* path) {
    const bool candidate=kind=="runtime16" || kind=="direct16";
    const bool integrated=kind=="runtime" || kind=="runtime16";
    require(integrated || kind=="direct" || kind=="direct16","unknown output generator");
    std::ofstream file(path,std::ios::binary);
    require(bool(file),"cannot open evidence buffer output");
    PaletteRuntime runtime;
    if (integrated) {
        require(runtime.configure(config(),0),"runtime configure failed");
        require(runtime.outputPrecision()==0u,"boot output precision is not the reference");
        if (candidate) runtime.setOutputPrecision(1u);
        // These old product controls must have no effect on the donor scene.
        for(unsigned ch=0;ch<2;++ch) {
            auto& c=runtime.channel(ch).controls();
            c.brightness=0; c.photons_id=0; c.saturation=0;
            c.incandescent_filter=1; c.square_iterations=3;
            c.reverse_order=true; c.mirror_enabled=false;
            c.base_coat=true; c.base_coat_intensity=1;
        }
    } else {
        K1EmotiscopeConfig c{};
        k1_emotiscope_default_config(&c);
        c.current_mode=1;
        require(k1_emotiscope_set_config(&c),"direct donor configure failed");
    }
    k1::contract::AudioFeaturesV1 legacy{};
    legacy.validity_flags=0xffffffffu;
    legacy.event_flags=0xffffffffu;
    legacy.peak_scaled=legacy.vu_level=1;
    for(auto& value:legacy.spectrum) value=1;
    k1::core::audio::TempoTrackerEvent legacy_tempo{};
    k1::core::visual::VisualWaveformHistory legacy_wave{};
    const k1::core::visual::VisualAudioFrameView old_view{
        legacy,legacy_tempo,legacy_wave,123456u};
    uint64_t nonzero_bytes=0;
    for(unsigned hop=1;hop<=2000;++hop) {
        float pcm[64];
        for(unsigned i=0;i<64;++i) {
            const double t=double((hop-1)*64+i)/12800.0;
            const double envelope=0.02+0.06*std::pow(std::max(0.0,std::sin(2*M_PI*2*t)),4);
            pcm[i]=float(envelope*std::sin(2*M_PI*110*t)+
                0.013*std::sin(2*M_PI*(t<5?440:659.2551)*t));
        }
        require(k1_emotiscope_push_pcm64(pcm,hop*5000u),"PCM rejected");
        if(hop%2) continue;
        if(integrated) {
            require(runtime.step(hop*5000u,&old_view),"runtime render not released");
            std::array<uint8_t,960> wire{};
            require(runtime.packBenchGrb48Lane(wire.data(),480,0)==480,"DIN-A pack failed");
            require(runtime.packBenchGrb48Lane(wire.data()+480,480,1)==480,"DIN-B pack failed");
            for(auto value:wire) nonzero_bytes+=value!=0;
            file.write(reinterpret_cast<const char*>(wire.data()),wire.size());
        } else {
            std::array<uint8_t,384> rgb{};
            require(k1_emotiscope_render(hop*5000u,rgb.data()),"direct render failed");
            if (candidate) {
                // The validated post-gamma floats themselves; the oracle rounds them.
                static K1EmotiscopeSnapshot snapshot;
                k1_emotiscope_snapshot(&snapshot);
                for(float value:snapshot.post_gamma_rgb) nonzero_bytes+=value!=0.0f;
                file.write(reinterpret_cast<const char*>(snapshot.post_gamma_rgb),
                           sizeof(snapshot.post_gamma_rgb));
            } else {
                for(auto value:rgb) nonzero_bytes+=value!=0;
                file.write(reinterpret_cast<const char*>(rgb.data()),rgb.size());
            }
        }
    }
    require(nonzero_bytes>0,"vacuous all-black comparison");
    require(bool(file),"buffer evidence write failed");
    std::cout<<"{\"frames\":1000,\"nonzero_bytes\":"<<nonzero_bytes
             <<",\"integrated\":"<<(integrated?"true":"false")<<"}\n";
}
#ifdef K1_EMOTISCOPE_TEST_SEAM
// Dim fade through the bottom of the 8-bit range, pushed through the production
// output stage frame by frame. Counts distinct non-zero wire levels per path.
static void fade_case() {
    constexpr unsigned kFrames=600;
    std::cout<<"{\"frames\":"<<kFrames<<",\"start_level\":0.03,\"end_level\":0.0,\"cases\":[";
    bool first=true;
    for (unsigned brightness:{100u,24u}) {
      for (bool dither:{true,false}) {
        K1EmotiscopeConfig c{};
        k1_emotiscope_default_config(&c);
        c.temporal_dithering=dither;
        require(k1_emotiscope_set_config(&c),"fade configure failed");
        std::array<bool,65536> ref_seen{}, cand_seen{};
        unsigned ref_levels=0, cand_levels=0;
        std::array<float,384> frame{};
        std::array<uint8_t,384> rgb8{};
        std::array<uint16_t,384> rgb16{};
        for (unsigned f=0; f<kFrames; ++f) {
            const float v=0.03f*float(kFrames-1u-f)/float(kFrames-1u);
            frame.fill(v);
            require(k1_emotiscope_test_transmit(frame.data(),rgb8.data(),rgb16.data()),"fade frame rejected");
            // Pixel 0 green, wire code after the one production output gain.
            const uint16_t ref=k1::titan::scale16(uint16_t(rgb8[1]*257u),brightness);
            const uint16_t cand=k1::titan::scale16(rgb16[1],brightness);
            if (ref && !ref_seen[ref]) { ref_seen[ref]=true; ++ref_levels; }
            if (cand && !cand_seen[cand]) { cand_seen[cand]=true; ++cand_levels; }
        }
        std::cout<<(first?"":",")<<"{\"output_brightness\":"<<brightness<<",\"temporal_dithering\":"
                 <<(dither?"true":"false")<<",\"reference_distinct_nonzero_levels\":"<<ref_levels
                 <<",\"candidate_distinct_nonzero_levels\":"<<cand_levels<<"}";
        first=false;
      }
    }
    std::cout<<"]}\n";
}
#endif
int main(int argc,char** argv) {
    require(argc>=2,"usage: markers | fade | direct|direct16|runtime|runtime16 OUTPUT");
    require(k1_emotiscope_init(0x45e071u),"boot-once donor init failed");
    const std::string kind=argv[1];
    if(kind=="markers") marker_case();
#ifdef K1_EMOTISCOPE_TEST_SEAM
    else if(kind=="fade") fade_case();
#endif
    else { require(argc==3,"output file required"); generate(kind,argv[2]); }
    return 0;
}

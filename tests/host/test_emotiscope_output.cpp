// Exercises the production 128->160 adapter and PaletteRuntime GRB48 packer.
#include "emotiscope_port.h"
#include "emotiscope_output.h"
#include "palette_runtime.h"
#include "hd_pixel16.h"
#include "core/visual/product_palette.h"
#include <algorithm>
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
    const bool paletted=kind=="runtimeP";
    const bool integrated=kind=="runtime" || kind=="runtime16" || paletted;
    require(integrated || kind=="direct" || kind=="direct16","unknown output generator");
    std::ofstream file(path,std::ios::binary);
    require(bool(file),"cannot open evidence buffer output");
    PaletteRuntime runtime;
    if (integrated) {
        require(runtime.configure(config(),0),"runtime configure failed");
        require(runtime.outputPrecision()==0u,"boot output precision is not the reference");
        if (candidate) runtime.setOutputPrecision(1u);
        if (paletted) require(k1_emotiscope_set_palette(25u) && k1_emotiscope_palette()==25u,"palette select failed");
        require(!k1_emotiscope_set_palette(45u),"out-of-range palette accepted");
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

// Independent oracle: K1 palette_manual_colour (lightshow_modes.h, HD path) over the
// raw catalogue stops, and K1 GDFT palette mode: position = colour + strip progress,
// level = 0.65v^2+0.35v (SQUARE_ITER 1), no saturation, no output gamma.
static void k1_oracle(unsigned id,float h,float level,float out[3]) {
    const auto& pal=k1::core::visual::productPalette(static_cast<std::uint16_t>(id));
    h-=std::floor(h); if(h<0.0f)h+=1.0f;
    unsigned i=0;
    while(i+1<pal.stop_count && pal.stops[i+1].position/255.0f<h) ++i;
    float c[3];
    auto rgb=[&](unsigned k,float* o){o[0]=pal.stops[k].colour.red/255.0f;o[1]=pal.stops[k].colour.green/255.0f;o[2]=pal.stops[k].colour.blue/255.0f;};
    if(i+1>=pal.stop_count || h<=pal.stops[0].position/255.0f) rgb(h<=pal.stops[0].position/255.0f?0:pal.stop_count-1,c);
    else {
        float a[3],b[3]; rgb(i,a); rgb(i+1,b);
        const float span=(pal.stops[i+1].position-pal.stops[i].position)/255.0f;
        float t=span>1e-6f?(h-pal.stops[i].position/255.0f)/span:0.0f; t=t<0?0:(t>1?1:t);
        for(int j=0;j<3;++j) c[j]=a[j]+(b[j]-a[j])*t;
    }
    for(int j=0;j<3;++j) out[j]=c[j]*level;
}
static void k1palette_case() {
    // Same sampler the runtime registers (palette_runtime.cpp); its wiring to the
    // wire is covered by the runtimeP case. The oracle above does not use it.
    k1_emotiscope_set_palette_sampler([](std::uint32_t id,float phase,float rgb[3]){
        const auto c=k1::core::visual::sampleProductPaletteHd(static_cast<std::uint16_t>(id),phase,1.0F);
        rgb[0]=c.red;rgb[1]=c.green;rgb[2]=c.blue;});
    K1EmotiscopeConfig c{};
    k1_emotiscope_default_config(&c);
    c.current_mode=1;
    unsigned checked=0; double worst=0.0;
    for(bool autoc:{false,true}) for(bool reverse:{false,true}) {
        c.reverse_color_range=reverse;
        c.auto_color_cycle=autoc;
        const float base=autoc?c.color:0.0f;  // Colour moves the palette only via auto colour
        require(k1_emotiscope_set_config(&c),"config write failed");
        for(unsigned id=1u;id<=44u;++id) {
            require(k1_emotiscope_set_palette(id),"palette select failed");
            for(int s=0;s<=20;++s) for(float v:{0.0f,0.25f,0.5f,1.0f}) {
                const float progress=s/20.0f;
                float pre[3],post[3],want[3];
                k1_emotiscope_test_colour(progress,v,pre,post);
                k1_oracle(id-1u,reverse?base+1.0f-progress:base+progress,0.65f*v*v+0.35f*v,want);
                for(int j=0;j<3;++j){
                    const double e=std::fabs(double(post[j])-want[j]); if(e>worst)worst=e;
                    require(e<1e-5,"palette colour differs from K1 drawing");
                    require(pre[j]==post[j],"palette colour was gamma-squared");
                }
                ++checked;
            }
        }
        // Zero must not switch a musical scene back to native HSV.
        require(!k1_emotiscope_set_palette(0u) && k1_emotiscope_palette()==44u,"native zero accepted or changed palette");

    }
    // Background, peaks, screensaver and Hype with a palette on.
    c.auto_color_cycle=false; c.reverse_color_range=false; c.mirror_mode=false;
    require(k1_emotiscope_set_config(&c),"config write failed");
    require(k1_emotiscope_set_palette(31u),"palette select failed");
    std::array<float,384> f{};
    k1_emotiscope_test_stage(0,0.8f,f.data());
    for(unsigned i=0;i<128;++i){
        float want[3]; k1_oracle(30u,i/128.0f,0.04f,want);  // (0.25*0.8)^2
        for(int j=0;j<3;++j) require(std::fabs(f[3*i+j]-want[j])<1e-5f,"background is not the palette at the same positions");
    }
    for(unsigned i=0;i<128;++i){f[3*i]=1.2f;f[3*i+1]=0.6f;f[3*i+2]=0.3f;}
    k1_emotiscope_test_stage(1,0.0f,f.data());
    require(f[0]<1.0f && std::fabs(f[1]/f[0]-0.5f)<1e-5f && std::fabs(f[2]/f[0]-0.25f)<1e-5f,"peak soft-clip changed the palette hue");
    k1_emotiscope_test_stage(2,0.0f,f.data());
    for(unsigned d=0;d<4;++d){
        float want[3]; k1_oracle(30u,0.125f+0.25f*d,1.0f,want);
        for(int j=0;j<3;++j) require(std::fabs(f[3*d+j]-want[j])<1e-5f,"screensaver colour is not from the palette");
    }
    k1_emotiscope_test_stage(3,0.0f,f.data());
    require(f[0]==1.0f && std::fabs(f[1]-c.color_range)<1e-6f,"Hype colour-range pin not scoped to the draw");
    require(!k1_emotiscope_set_palette(0u) && k1_emotiscope_palette()==31u,"native zero changed selected palette");
    k1_emotiscope_test_stage(3,0.0f,f.data());
    require(f[0]==1.0f && std::fabs(f[1]-c.color_range)<1e-6f,"palette draw did not restore range");
    // Silence has no injected background floor; Analog retains geometry with paired hues.
    k1_emotiscope_test_stage(0,0.0f,f.data());
    for(float v:f)require(v==0.0f,"zero audio injected background light");
    c.current_mode=0;c.mirror_mode=true;
    require(k1_emotiscope_set_config(&c),"Analog config rejected");
    require(k1_emotiscope_set_palette(1u),"SunsetReal Analog palette rejected");
    float analog_min_hue_difference=1.0f;
    unsigned analog_paired_pixels=0;
    for(float level:{0.25f,0.75f}){
        k1_emotiscope_test_stage(4,level,f.data());
        k1_emotiscope_test_stage(4,level,f.data());
        float expected_left[3],expected_right[3];
        k1_oracle(0u,level+0.5f,1.0f,expected_left);
        k1_oracle(0u,level,1.0f,expected_right);
        const float left_sum=expected_left[0]+expected_left[1]+expected_left[2];
        const float right_sum=expected_right[0]+expected_right[1]+expected_right[2];
        require(left_sum>0.0f&&right_sum>0.0f,"Analog oracle palette phase is black");
        unsigned paired_lit=0;
        float measured_hue_difference=0.0f;
        for(unsigned i=0;i<64u;++i){
            const unsigned a=3u*i,b=3u*(127u-i);
            const float sa=f[a]+f[a+1]+f[a+2],sb=f[b]+f[b+1]+f[b+2];
            if(sa>1e-6f&&sb>1e-6f){
                ++paired_lit;
                for(unsigned j=0;j<3u;++j){
                    const float left=f[a+j]/sa,right=f[b+j]/sb;
                    if(std::fabs(left-expected_left[j]/left_sum)>=1e-4f || std::fabs(right-expected_right[j]/right_sum)>=1e-4f)
                        std::cerr<<"ANALOG_HUE_SAMPLE level="<<level<<" pixel="<<i<<" component="<<j
                                 <<" left="<<left<<" expected_left="<<expected_left[j]/left_sum
                                 <<" right="<<right<<" expected_right="<<expected_right[j]/right_sum<<"\n";
                    require(std::fabs(left-expected_left[j]/left_sum)<1e-4f,"Analog left hue differs from its paired palette phase");
                    require(std::fabs(right-expected_right[j]/right_sum)<1e-4f,"Analog right hue differs from its palette phase");
                    measured_hue_difference=std::max(measured_hue_difference,std::fabs(left-right));
                }
            }
        }
        require(paired_lit>0u,"Analog lost its actual audio-driven pixels");
        require(measured_hue_difference>1e-3f,"Analog paired dots lost their distinct catalogue hues");
        analog_min_hue_difference=std::min(analog_min_hue_difference,measured_hue_difference);
        analog_paired_pixels+=paired_lit;
    }
    std::cout<<"{\"k1_palette_pixels_checked\":"<<checked<<",\"worst_abs_error\":"<<worst
             <<",\"analog_paired_pixels\":"<<analog_paired_pixels
             <<",\"analog_min_normalised_hue_difference\":"<<analog_min_hue_difference<<"}\n";
}
int main(int argc,char** argv) {
    require(argc>=2,"usage: markers | fade | direct|direct16|runtime|runtime16 OUTPUT");
    require(k1_emotiscope_init(0x45e071u),"boot-once donor init failed");
    const std::string kind=argv[1];
    if(kind=="markers") marker_case();
#ifdef K1_EMOTISCOPE_TEST_SEAM
    else if(kind=="fade") fade_case();
    else if(kind=="k1palette") k1palette_case();
#endif
    else { require(argc==3,"output file required"); generate(kind,argv[2]); }
    return 0;
}

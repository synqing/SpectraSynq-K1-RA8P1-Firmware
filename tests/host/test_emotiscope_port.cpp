// AP conformance against separately compiled, pinned original donor headers.
#include "emotiscope_port.h"
#include "emotiscope_ap_reference.h"
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace ref = emotiscope_reference;
static double max_error = 0.0;
static uint64_t compared_values = 0;
static void require(bool ok, const char* message) {
    if (!ok) { std::cerr << message << "\n"; std::exit(2); }
}
static void equal_value(float actual, float expected, const char* field,
                        uint32_t hop, uint32_t index = 0) {
    const double error = std::fabs(double(actual)-double(expected));
    const double tolerance = 2e-6*(1.0+std::fabs(double(expected)));
    ++compared_values;
    max_error = std::max(max_error, error);
    if (!std::isfinite(actual) || !std::isfinite(expected) || error > tolerance) {
        std::cerr << "AP_PARITY_MISMATCH field=" << field << " index=" << index
                  << " hop=" << hop << " actual=" << actual
                  << " expected=" << expected << " error=" << error << "\n";
        std::exit(3);
    }
}
static void compare(const K1EmotiscopeSnapshot& p, uint32_t hop) {
    for (unsigned i=0; i<64; ++i) {
        equal_value(p.spectrogram[i], ref::spectrogram[i], "spectrogram", hop, i);
        equal_value(p.spectrogram_smooth[i], ref::spectrogram_smooth[i], "smooth", hop, i);
    }
    for (unsigned i=0; i<12; ++i)
        equal_value(p.chromagram[i], ref::chromagram[i], "chromagram", hop, i);
    equal_value(p.vu_level_raw, ref::vu_level_raw, "vu_raw", hop);
    equal_value(p.vu_level, ref::vu_level, "vu", hop);
    equal_value(p.vu_max, ref::vu_max, "vu_max", hop);
    equal_value(p.vu_floor, ref::vu_floor, "vu_floor", hop);
    equal_value(p.tempo_confidence, ref::tempo_confidence, "confidence", hop);
    equal_value(p.tempi_power_sum, ref::tempi_power_sum, "power_sum", hop);
    for (unsigned i=0; i<1024; ++i) {
        equal_value(p.novelty[i], ref::novelty_curve[i], "novelty", hop, i);
        equal_value(p.novelty_normalized[i], ref::novelty_curve_normalized[i], "novelty_norm", hop, i);
        equal_value(p.vu_curve[i], ref::vu_curve[i], "vu_curve", hop, i);
    }
    for (unsigned i=0; i<96; ++i) {
        equal_value(p.tempi_magnitude[i], ref::tempi[i].magnitude, "tempo_mag", hop, i);
        equal_value(p.tempi_full_scale[i], ref::tempi[i].magnitude_full_scale, "tempo_full", hop, i);
        equal_value(p.tempi_phase[i], ref::tempi[i].phase, "tempo_phase", hop, i);
        equal_value(p.tempi_beat[i], ref::tempi[i].beat, "tempo_beat", hop, i);
        equal_value(p.tempi_smooth[i], ref::tempi_smooth[i], "tempo_smooth", hop, i);
        equal_value(p.tempi_target_hz[i], ref::tempi[i].target_tempo_hz, "tempo_hz", hop, i);
        require(p.tempi_block_size[i] == ref::tempi[i].block_size, "tempo window differs");
    }
}
static std::vector<float> load_pcm(const char* path) {
    std::ifstream input(path, std::ios::binary);
    require(bool(input), "cannot open explicit s16le/12800 mono fixture");
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(input)), {});
    require(!bytes.empty() && bytes.size()%128 == 0, "fixture must have complete 64-sample s16le chunks");
    std::vector<float> pcm(bytes.size()/2);
    for (size_t i=0; i<pcm.size(); ++i) {
        const uint16_t raw = uint16_t(bytes[2*i]) | uint16_t(bytes[2*i+1])<<8;
        const int32_t signed_value = raw < 32768 ? int32_t(raw) : int32_t(raw)-65536;
        pcm[i] = float(signed_value)/32768.0f;
    }
    return pcm;
}
static std::vector<float> synthetic() {
    std::vector<float> pcm(12800*20, 0.0f);
    for (size_t i=0; i<pcm.size(); ++i) {
        const double t = double(i)/12800.0;
        // Silence, single note, changing level, two-note mixture, and sparse attacks.
        if (t>=1.0 && t<5.0) pcm[i]=float(0.015*std::sin(2.0*M_PI*440.0*t));
        if (t>=5.0 && t<9.0) pcm[i]=float((t<7.0?0.08:0.002)*std::sin(2.0*M_PI*329.6276*t));
        if (t>=9.0 && t<13.0) pcm[i]=float(0.04*std::sin(2.0*M_PI*110.0*t)+0.02*std::sin(2.0*M_PI*880.0*t));
        if (t>=13.0 && t<17.0 && i%6400<64) pcm[i]=float(0.3*std::exp(-double(i%6400)/16.0));
    }
    return pcm;
}
int main(int argc, char** argv) {
    require(argc>=2, "usage: test_emotiscope_port synthetic | pcm FILE");
    std::vector<float> pcm;
    if (std::string(argv[1])=="synthetic") pcm=synthetic();
    else { require(argc==3 && std::string(argv[1])=="pcm", "explicit fixture arguments required"); pcm=load_pcm(argv[2]); }
    require(k1_emotiscope_init(0x45e071u), "port init failed");
    ref::init();
    K1EmotiscopeConfig config;
    k1_emotiscope_default_config(&config);
    config.current_mode=1; // Full donor GPU runs; AP oracle owns the matching novelty/phase clock.
    require(k1_emotiscope_set_config(&config), "donor config rejected");
    std::array<uint8_t,384> previous{}, rgb{};
    uint64_t frame_hash=1469598103934665603ULL;
    uint32_t nonblack_frames=0, changed_frames=0, renders=0;
    K1EmotiscopeSnapshot snapshot{};
    const uint32_t hops=uint32_t(pcm.size()/64);
    for (uint32_t hop=1; hop<=hops; ++hop) {
        const uint32_t now_us=hop*5000u;
        const float* block=pcm.data()+size_t(hop-1)*64;
        require(k1_emotiscope_push_pcm64(block,now_us), "valid PCM rejected");
        ref::push_pcm64(block,now_us);
        k1_emotiscope_snapshot(&snapshot);
        compare(snapshot,hop);
        if (hop%2==0) {
            require(k1_emotiscope_render(now_us,rgb.data()), "render failed");
            ref::render_step(now_us);
            k1_emotiscope_snapshot(&snapshot);
            compare(snapshot,hop);
            bool nonblack=false;
            for (uint8_t value:rgb) { nonblack|=value!=0; frame_hash=(frame_hash^value)*1099511628211ULL; }
            nonblack_frames += nonblack;
            changed_frames += rgb!=previous;
            previous=rgb; ++renders;
        }
    }
    require(snapshot.audio_hops==hops && snapshot.render_frames==renders, "port lifecycle count differs");
    require(snapshot.nonfinite_output_frames==0, "port sanitized a nonfinite image");
    float bad[64]={}; bad[11]=std::numeric_limits<float>::quiet_NaN();
    require(!k1_emotiscope_push_pcm64(bad,(hops+1)*5000u), "NaN chunk was accepted");
    k1_emotiscope_snapshot(&snapshot);
    require(snapshot.audio_hops==hops && snapshot.rejected_audio_chunks==1, "invalid chunk changed AP history/count");
    compare(snapshot,hops);
    std::cout << "{\"audio_hops\":" << hops << ",\"render_frames\":" << renders
              << ",\"compared_values\":" << compared_values << ",\"max_ap_error\":" << max_error
              << ",\"nonblack_frames\":" << nonblack_frames << ",\"changed_frames\":" << changed_frames
              << ",\"rgb8_fnv1a\":" << frame_hash << ",\"invalid_chunk_rejected\":true}\n";
    return 0;
}

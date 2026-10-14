// Conditional reproduction of Sonic's first-pair 805/753-sample queue losses.
// Production capture/ASRC code supplies PCM, chronology and fault accounting.
// Only the extracted owner loop uses simulated DSP/render/clock/pair seams.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include "pdm_target.h"

extern "C" {
void pressure_reset(void);
void pressure_time(std::uint64_t);
void pressure_raw(unsigned, std::uint64_t);
void pressure_poll(void);
int pressure_pull(k1_audio_hop_t*);
unsigned pressure_filled(void);
unsigned pressure_rejected(void);
unsigned pressure_discarded(void);
unsigned pressure_consumed(void);
unsigned pressure_raw_faults(void);
k1_pdm_capture_service_t pressure_service(void);
}

namespace k1 { namespace titan {
enum class LiveHopStatus { Ok, NotReady, Discontinuity, Fault };
}}
using Status = k1::titan::LiveHopStatus;
static std::vector<k1_audio_hop_t> received;
static std::uint64_t now_us, frame_due_us, pending_receipt;
static unsigned pending_sequence, calls, pair_polls, renders, resets;
static bool inject_pending;

static void advance(std::uint64_t delta) {
    now_us += delta;
    if (inject_pending && pending_receipt <= now_us) {
        pressure_raw(pending_sequence, pending_receipt);
        inject_pending = false;
    }
    pressure_time(now_us);
}
static void palette_pair_poll() { ++pair_polls; }
static struct Runtime {
    Status serviceOneHop() {
        ++calls;
        k1_audio_hop_t hop{};
        const int result = pressure_pull(&hop);
        if (result == K1_AUDIO_READ_NOT_READY) return Status::NotReady;
        if (result == K1_AUDIO_READ_DISCONTINUITY) return Status::Discontinuity;
        if (result != K1_AUDIO_READ_OK) return Status::Fault;
        received.push_back(hop);
        advance(2500u); // Explicit cost seam, not host or target measurement.
        return Status::Ok;
    }
    std::uint64_t nowUs() { return now_us; }
} live_runtime;
static struct Palettes {
    void resetAudioState() { ++resets; }
    bool frameDue(std::uint64_t time) { return time >= frame_due_us; }
    void renderDue() {
        if (!frameDue(now_us)) return;
        assert(now_us - frame_due_us < 10000u); // No skipped 10ms release.
        ++renders;
        frame_due_us += 10000u;
        advance(1500u);
    }
} palettes;
// Production calls its existing frame/transport owner. Only its cost and due
// clock are seams here; target replay must qualify the real render/emit path.
static void palette_step(bool emit) {
    assert(emit);
    palettes.renderDue();
}

static void current_owner() {
    // INSERT_CURRENT_OWNER_LOOP
    palettes.renderDue();
}
static void interleaved_owner() {
    // INSERT_INTERLEAVED_OWNER_LOOP
    palettes.renderDue();
}

static std::uint64_t receipt(unsigned sequence) {
    return 1000000u + (std::uint64_t(sequence) * 296000000u) / 41417u;
}
static void collect_ready() {
    k1_audio_hop_t hop{};
    while (pressure_pull(&hop) == K1_AUDIO_READ_OK) received.push_back(hop);
}
static std::vector<k1_audio_hop_t> serial_oracle(unsigned blocks) {
    pressure_reset();
    received.clear();
    for (unsigned sequence = 1; sequence <= blocks; ++sequence) {
        pressure_raw(sequence, receipt(sequence));
        pressure_poll();
        collect_ready();
    }
    assert(pressure_rejected() == 0u && pressure_raw_faults() == 0u);
    return received;
}

static void scenario(unsigned blocks, unsigned hops, unsigned witness,
                     bool interleaved, bool require_lossless) {
    const auto oracle = serial_oracle(blocks + 1u);
    pressure_reset();
    received.clear();
    // Construct a valid stream through actual raw ownership and pulls. This
    // prefix does not assert the unrecorded physical schedule that built it.
    for (unsigned sequence = 1; sequence <= blocks - 2u; ++sequence) {
        pressure_raw(sequence, receipt(sequence));
        pressure_poll();
        while (received.size() < hops - 1u) {
            k1_audio_hop_t hop{};
            const int result = pressure_pull(&hop);
            if (result == K1_AUDIO_READ_NOT_READY) break;
            assert(result == K1_AUDIO_READ_OK);
            received.push_back(hop);
        }
    }
    assert(received.size() == hops - 1u);
    assert(pressure_rejected() == 0u);
    const unsigned entry = pressure_filled();
    assert(entry == (witness == 805u ? 420u : 368u));
    // Two periodic completions are ready at this poll; a third will arrive
    // ~7.047ms after entry. Poll finishes at4.468ms, first hop at6.968ms,
    // due render at8.468ms: the third completion then precedes the next poll.
    pressure_raw(blocks - 1u, receipt(blocks - 1u));
    pressure_raw(blocks, receipt(blocks));
    now_us = receipt(blocks) + 100u;
    pressure_time(now_us);
    pressure_poll();
    assert(pressure_filled() == (witness == 805u ? 1012u : 960u));
    calls = pair_polls = renders = resets = 0u;
    frame_due_us = now_us + 6000u;
    pending_sequence = blocks + 1u;
    pending_receipt = receipt(pending_sequence);
    inject_pending = true;
    advance(4468u); // Measured-range two-pair cost; real wall time is mocked.
    if (interleaved) interleaved_owner(); else current_owner();
    assert(!inject_pending);
    assert(renders == 1u && resets == 0u && pair_polls == calls);
    assert(calls == (interleaved ? 3u : 1u));
    if (!interleaved) assert(pressure_filled() == witness);
    const unsigned before_push = pressure_filled();
    pressure_poll();
    if (require_lossless) assert(pressure_rejected() == 0u);
    if (!interleaved) {
        const auto fault = pressure_service();
        assert(pressure_rejected() == 1u);
        assert(fault.reject_poll_entry_filled == witness);
        assert(fault.reject_filled == witness && fault.reject_poll_pair_index == 1u);
        assert(pressure_discarded() == witness + 296u);
        k1_audio_hop_t gap{};
        assert(pressure_pull(&gap) == K1_AUDIO_READ_DISCONTINUITY);
        assert(gap.discontinuity_reason == 2u);
    } else {
        assert(pressure_rejected() == 0u && pressure_discarded() == 0u);
        collect_ready();
        assert(received.size() == oracle.size());
        for (std::size_t i = 0; i < oracle.size(); ++i) {
            const auto& got = received[i];
            const auto& expected = oracle[i];
            assert(std::memcmp(got.pcm, expected.pcm, sizeof(got.pcm)) == 0);
            assert(got.stream_epoch == 1u && got.hop_sequence == i + 1u);
            assert(got.source_first_index == expected.source_first_index);
            assert(got.source_last_index == expected.source_last_index);
            assert(got.source_first_frac_q16 == expected.source_first_frac_q16);
        }
        assert(pressure_consumed() + pressure_filled() == (blocks + 1u) * 296u);
    }
    assert(pressure_raw_faults() == 0u);
    std::printf("witness=%u path=%s entry=%u before_next_push=%u rejected=%u "
                "discarded=%u pulls=%u renders=%u raw_faults=%u pcm_hops=%zu\n",
                witness, interleaved ? "repaired-production-loop" : "restored-old-production-loop",
                entry, before_push, pressure_rejected(), pressure_discarded(),
                calls, renders, pressure_raw_faults(), received.size());
}

int main(int argc, char** argv) {
    assert(argc == 2);
    const bool interleaved = std::strcmp(argv[1], "interleaved") == 0;
    const bool require_lossless = std::strcmp(argv[1], "require-lossless-current") == 0;
    assert(interleaved || require_lossless || std::strcmp(argv[1], "observed") == 0);
    scenario(37u, 49u, 805u, interleaved, require_lossless);
    scenario(76u, 105u, 753u, interleaved, require_lossless);
}

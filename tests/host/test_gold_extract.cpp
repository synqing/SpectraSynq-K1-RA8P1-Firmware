#include "core/visual/ws2816_pack.h"
#include "k1_exact_stream.h"
#include "k1_shared_snapshot.h"
#include "titan_ram_diag.h"
#include "ws281x_waveform.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using k1::core::visual::Pixel16;
using k1::core::visual::kPackedBytesPerLane;
using k1::core::visual::kPixelsPerChannel;
using k1::core::visual::packPixel;
using k1::core::visual::splitChannel160;

static void fill_hop_pcm(int16_t *pcm, uint32_t seq)
{
    for (uint32_t i = 0; i < K1_EXACT_HOP_SAMPLES * K1_EXACT_CHANNELS; ++i) {
        pcm[i] = (int16_t)((int32_t)(seq * 17u + i * 3u) - 4000);
    }
    pcm[0] = 32767;
    pcm[1] = (int16_t)-32768;
    pcm[2] = (int16_t)-1;
}

int main()
{
    k1_ram_diag_t diag{};
    k1_ram_diag_reset(&diag);

    int16_t pcm[K1_EXACT_HOP_SAMPLES * K1_EXACT_CHANNELS];
    k1_exact_stream_t stream{};
    k1_exact_stream_reset(&stream);
    std::vector<int16_t> dumped;
    uint32_t preserved = 0;
    for (uint32_t hop_i = 0; hop_i < 8u; ++hop_i) {
        k1_exact_hop_t hop{};
        fill_hop_pcm(pcm, hop_i);
        int16_t original[K1_EXACT_HOP_SAMPLES * K1_EXACT_CHANNELS];
        std::memcpy(original, pcm, sizeof(original));
        uint64_t start = (uint64_t)hop_i * K1_EXACT_HOP_US;
        uint64_t end = start + K1_EXACT_HOP_US;
        assert(k1_exact_hop_pack(&hop, hop_i, 1u, start, end, 0u, pcm) ==
               K1_EXACT_OK);
        k1_exact_stats_t stats{};
        k1_exact_stats(hop.pcm, K1_EXACT_HOP_SAMPLES * K1_EXACT_CHANNELS,
                       &stats);
        assert(stats.clip_count >= 2u);
        assert(stats.peak == 32768);
        assert(std::memcmp(hop.pcm, original, sizeof(original)) == 0);
        assert(k1_exact_stream_ingest(&stream, &diag, &hop) == K1_EXACT_OK);
        assert(std::memcmp(hop.pcm, original, sizeof(original)) == 0);
        dumped.insert(dumped.end(), hop.pcm,
                      hop.pcm + K1_EXACT_HOP_SAMPLES * K1_EXACT_CHANNELS);
        preserved += 1u;
        if (hop_i == 3u) {
            Pixel16 ch[kPixelsPerChannel]{};
            ch[0] = {0x12AB, 0x8000, 0};
            uint8_t lane_a[kPackedBytesPerLane]{};
            uint8_t lane_b[kPackedBytesPerLane]{};
            assert(splitChannel160(ch, kPixelsPerChannel, lane_a, lane_b));
            uint32_t duty[K1_WS281X_GPT_DUTY_CAP];
            k1_ws281x_duty_meta_t meta{};
            assert(k1_ws281x_build_gpt_duty(lane_a, kPackedBytesPerLane, 3u,
                                            100000000u, duty,
                                            K1_WS281X_GPT_DUTY_CAP, &meta));
            assert(meta.bits == 80u * 48u);
            assert(duty[0] == meta.t1h_counts - 1u);
        }
    }
    assert(stream.hops_accepted == 8u);
    assert(diag.sample_discontinuities == 0u);
    if (const char *path = std::getenv("K1_EXACT_DUMP")) {
        FILE *file = std::fopen(path, "wb");
        assert(file != nullptr);
        assert(std::fwrite(dumped.data(), 2u, dumped.size(), file) ==
               dumped.size());
        std::fclose(file);
    }

    k1_exact_hop_t gap{};
    fill_hop_pcm(pcm, 99u);
    assert(k1_exact_hop_pack(&gap, 99u, 1u, 90000u, 97500u, 0u, pcm) ==
           K1_EXACT_OK);
    assert(k1_exact_stream_ingest(&stream, &diag, &gap) == K1_EXACT_OK);
    assert((gap.flags & K1_EXACT_FLAG_DISCONTINUITY) != 0u);
    assert(diag.sample_discontinuities == 1u);

    k1_exact_hop_t mutated{};
    fill_hop_pcm(pcm, 8u);
    assert(k1_exact_hop_pack(&mutated, 8u, 1u, 0u, 7500u, 0u, pcm) ==
           K1_EXACT_OK);
    mutated.pcm[10] = (int16_t)(mutated.pcm[10] - (int16_t)(mutated.pcm[10] / 64));
    assert(k1_exact_hop_check(&mutated) == K1_EXACT_MUTATED);
    assert(k1_exact_stream_ingest(&stream, &diag, &mutated) == K1_EXACT_MUTATED);
    assert(diag.crc_fail >= 1u);

    uint8_t grb[3];
    assert(k1_ws281x_pack_grb24(0x12, 0x34, 0x56, grb));
    assert(grb[0] == 0x34 && grb[1] == 0x12 && grb[2] == 0x56);
    uint8_t packed16[6];
    Pixel16 px{0x12AB, 0, 0};
    packPixel(px, packed16);
    assert(std::memcmp(packed16, grb, 3) != 0);

    uint8_t one_pixel[6]{0x80, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint32_t duty[K1_WS281X_GPT_DUTY_CAP];
    k1_ws281x_duty_meta_t meta{};
    assert(k1_ws281x_build_gpt_duty(one_pixel, 6u, 3u, 100000000u, duty,
                                    K1_WS281X_GPT_DUTY_CAP, &meta));
    assert(duty[0] == meta.t1h_counts - 1u);
    assert(!k1_ws281x_build_gpt_duty(one_pixel, 5u, 3u, 100000000u, duty,
                                     K1_WS281X_GPT_DUTY_CAP, &meta));
    assert(!k1_ws281x_gpt_quantise(3u, 1000u, &meta));

    k1_ws281x_cost_t gpt80{};
    k1_ws281x_cost_t podr80{};
    k1_ws281x_cost_t gpt160{};
    k1_ws281x_cost_t podr160{};
    assert(k1_ws281x_gpt_cost(80u, 3u, &gpt80));
    assert(k1_ws281x_gpio_podr_cost(80u, 3u, 50u, &podr80));
    assert(k1_ws281x_gpt_cost(160u, 3u, &gpt160));
    assert(k1_ws281x_gpio_podr_cost(160u, 3u, 50u, &podr160));
    assert(gpt80.data_bytes == 15360u);
    assert(gpt80.data_ns == 4800000u);
    assert(gpt80.reset_ns == 300000u);
    assert(gpt80.emit_ns == 5100000u);
    assert(podr80.bytes == 204000u);
    assert(podr80.data_ns == 4800000u);
    assert(podr80.reset_ns == 300000u);
    assert(gpt160.data_bytes == 30720u);
    assert(gpt160.data_ns == 9600000u);
    assert(gpt160.emit_ns == 9900000u);
    assert(podr160.bytes == 396000u);
    k1_ws281x_cost_t gpt_2x80{};
    assert(k1_ws281x_gpt_cost_lanes(2u, 80u, 3u, &gpt_2x80));
    assert(gpt_2x80.data_ns == 4800000u);
    assert(gpt_2x80.emit_ns == 5100000u);
    assert(k1_ws281x_gpt_buffer_bytes(2u, 80u, 3u, 1u) == 30720u);
    assert(k1_ws281x_gpt_buffer_bytes(2u, 160u, 3u, 1u) == 61440u);
    assert(k1_ws281x_gpt_buffer_bytes(2u, 160u, 3u, 2u) == 122880u);
    assert(!k1_ws281x_gpio_podr_cost(80u, 3u, 100u, &podr80));

    k1_ram_diag_note_dma_done(&diag, 3840u, 1000u);
    k1_ram_diag_note_dma_fault(&diag, 7u, 1001u);
    k1_ram_diag_note_frame_submit(&diag, 2000u);
    k1_ram_diag_note_frame_complete(&diag, 2150u);
    assert(diag.dma_complete == 1u);
    assert(diag.frame_age_us == 150u);

    k1_shared_ctrl_t shm{};
    k1_shared_init(&shm);
    k1_latest_snapshot_t body{};
    k1_latest_snapshot_t got{};
    k1_latest_snapshot_t got2{};
    for (uint32_t i = 0; i < 8u; ++i) {
        body.vp_frame_id = i;
        body.output_hash = 0x100u + i;
        body.heartbeat = i + 1u;
        k1_shared_publish(&shm, &body);
    }
    k1_shared_ticket_t ticket{};
    assert(k1_shared_consume_prepare(&shm, &ticket) == 1);
    assert(ticket.intended == 0u);
    body.vp_frame_id = 8u;
    body.output_hash = 0x108u;
    body.heartbeat = 9u;
    k1_shared_publish(&shm, &body);
    int first = k1_shared_consume_finish(&shm, &diag, &ticket, &got);
    if (first) {
        assert(got.sequence == 8u);
        assert(got.vp_frame_id == 8u);
        assert(got.output_hash == 0x108u);
    }
    int second = k1_shared_consume(&shm, &diag, &got2);
    if (second) {
        assert(got2.sequence != 1u);
        assert(got2.sequence >= 8u);
    }
    if (first && second) {
        assert(!(got.sequence == 8u && got2.sequence == 1u));
    }
    assert(shm.race_drop >= 1u);

    k1_shared_ctrl_t crc_ring{};
    k1_shared_init(&crc_ring);
    body.vp_frame_id = 7u;
    body.output_hash = 0xA5A5A5A5u;
    k1_shared_publish(&crc_ring, &body);
    crc_ring.slots[(crc_ring.write_seq - 1u) % K1_SNAP_SLOTS].body.output_hash ^=
        1u;
    uint32_t crc_before = diag.crc_fail;
    (void)k1_shared_consume(&crc_ring, &diag, &got);
    assert(diag.crc_fail > crc_before);

    std::printf(
        "K1_GOLD_EXTRACT=PASS exact_hops=%u discontinuities=%u "
        "samples_preserved=%u crc_fail=%u gpt80_data_bytes=%u "
        "gpt80_data_ns=%u gpt80_reset_ns=%u podr80_bytes=%u "
        "gpt160_data_bytes=%u gpt160_emit_ns=%u podr160_bytes=%u "
        "double_buf_2x160=%u concurrent_2x80_data_ns=%u concurrent_2x80_emit_ns=%u "
        "race_drop=%u snap_first_seq=%u "
        "snap_second=%d queue_owned=%u\n",
        stream.hops_accepted, diag.sample_discontinuities, preserved,
        diag.crc_fail, gpt80.data_bytes, gpt80.data_ns, gpt80.reset_ns,
        podr80.bytes, gpt160.data_bytes, gpt160.emit_ns, podr160.bytes,
        k1_ws281x_gpt_buffer_bytes(2u, 160u, 3u, 2u), gpt_2x80.data_ns,
        gpt_2x80.emit_ns, shm.race_drop,
        first ? got.sequence : 0u, second, diag.queue_owned);
    return 0;
}

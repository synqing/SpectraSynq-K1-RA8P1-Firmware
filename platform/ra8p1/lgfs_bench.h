// lgfs_bench.h — LGFS optical test cards on the Titan product image (K1_LGFS_BENCH).
//
// Captain request 2026-10-16: film-only characterisation of the K1 light-guide
// film stack, photographed with an iPhone. The card replaces the rendered scene
// at the wire for as long as it is selected; `off` hands the wire straight back
// to the live renderer (autonomous music resumes on the next frame).
//
// Values are true 16-bit WS2816 codes from the shared kit header lgfs_cards.h:
// no brightness, no output/board intensity, no palette, no dither. The kit's
// power cap (LGFS_POWER_CAP_Q16, default 0.094 of both-edge white = 24/255)
// refuses any brighter selection before it is published.
//
// Edge mapping: LGFS edge 0 ("top" in the kit) -> board 0 (primary, P601/P604);
// LGFS edge 1 ("bottom") -> board 1 (top output). Within a board the lane split
// is the same as PaletteRuntime::packBoardGrb48Lane, so the logical index is the
// renderer's 0..159 index; the INDEX/CORNERS cards recover the physical layout.
//
// Threading: lgfs_command() runs where fixture commands run; it only publishes
// into a two-slot buffer. lgfs_begin_frame()/lgfs_pack_lane() run in the
// realtime owner (palette_step emit) and never block.
//
// v2 (Captain 16 Oct 2026, render-accuracy battery):
//  - Frame log: every accepted wire submission of either board (live music or
//    card) is copied into a RAM ring with its generation and board frame time.
//    Opcode 29 reads it, so a host can record exactly what the film was shown
//    while a camera films it. The copy is two 480-byte memcpy per board frame.
//  - freeze: holds the last logged frame of each board on the wire, byte for
//    byte, until the next selection. It is the show's own frame, so it is not
//    subject to the card power cap.
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "lgfs_cards.h"

#define K1_LGFS_OPCODE 28u
#define K1_LGFS_LOG_OPCODE 29u
#ifndef K1_LGFS_LOG_ENTRIES
#define K1_LGFS_LOG_ENTRIES 128u   // 2 boards x 100 fps -> 0.64 s of history; host polls at 10 Hz
#endif

namespace k1::titan::lgfs {

struct Slot { LgfsSelection sel; std::uint32_t active; };
inline Slot g_slot[2]{};
inline volatile std::uint32_t g_pub = 0u;   // index of the published slot
inline volatile std::uint32_t g_seq = 0u;   // bumps on every accepted command

// Realtime-owner state.
inline std::uint32_t g_seen = 0u;
inline LgfsSelection g_live{};
inline bool g_live_on = false;
inline std::uint32_t g_frame = 0u;
inline std::uint32_t g_frames_sent = 0u;

// Frame log (realtime owner writes; fixture commands read on the same main loop).
struct LogEntry {
  std::uint32_t seq;         // 1, 2, 3 ... across both boards
  std::uint16_t board;       // 0 = primary (kit edge 0), 1 = top output (kit edge 1)
  std::uint16_t flags;       // bit0 card shown, bit1 freeze
  std::uint32_t generation;  // wire generation handed to the DMA
  std::uint32_t card_frame;  // frame counter of the card (0 when live)
  std::uint64_t time_us;     // board frame time (palette clock)
  std::uint8_t lanes[960];   // DIN0 then DIN1, GRB48 exactly as submitted
};
static_assert(sizeof(LogEntry) == 984u, "log entry layout is part of opcode 29");
inline LogEntry g_log[K1_LGFS_LOG_ENTRIES]{};
inline std::uint32_t g_log_seq = 0u;
inline std::uint32_t g_log_last[2] = {0u, 0u};   // seq of the last entry per board
inline std::uint8_t g_hold[2][960]{};
inline bool g_hold_valid[2] = {false, false};

#if defined(__ARM_ARCH)
inline void barrier() { __asm volatile("dmb" ::: "memory"); }
#else
inline void barrier() { __atomic_thread_fence(__ATOMIC_SEQ_CST); }  // host tests
#endif

inline void publish(const LgfsSelection* sel, bool on) {
  const std::uint32_t next = g_pub ^ 1u;
  if (sel) g_slot[next].sel = *sel; else std::memset(&g_slot[next].sel, 0, sizeof(LgfsSelection));
  g_slot[next].active = on ? 1u : 0u;
  barrier();
  g_pub = next;
  barrier();
  g_seq = g_seq + 1u;
}

// Realtime owner: once per emitted frame, before packing. True while a card is shown.
inline bool lgfs_begin_frame() {
  const std::uint32_t seq = g_seq;
  if (seq != g_seen) {
    barrier();
    const Slot s = g_slot[g_pub];
    g_live = s.sel; g_live_on = s.active != 0u; g_frame = 0u; g_seen = seq;
    if (g_live_on && g_live.card == LGFS_FREEZE) {
      for (unsigned b = 0; b < 2u; ++b) {
        const std::uint32_t last = g_log_last[b];
        g_hold_valid[b] = last != 0u && g_log_seq - last < K1_LGFS_LOG_ENTRIES;
        if (g_hold_valid[b]) std::memcpy(g_hold[b], g_log[last % K1_LGFS_LOG_ENTRIES].lanes, 960u);
      }
    }
  }
  if (!g_live_on) return false;
  ++g_frame;
  return true;
}

// Pack one 80-pixel DIN lane of `board` from the live card (GRB48, 480 bytes).
// Same split as PaletteRuntime::packBoardGrb48Lane: board 1 DIN0 = right half.
inline std::size_t lgfs_pack_lane(std::uint8_t* out, std::size_t capacity, unsigned board, unsigned lane) {
  namespace v = k1::core::visual;
  if (!out || capacity < v::kPackedBytesPerLane || board > 1u || lane > 1u) return 0u;
#if defined(K1_TITAN_TOP_OUTPUT) && defined(K1_EMOTISCOPE_PORT)
  const unsigned half = board == 1u ? 1u - lane : lane;
#else
  const unsigned half = lane;
#endif
  if (g_live.card == LGFS_FREEZE) {   // replay the held wire bytes of this board and DIN
    if (g_hold_valid[board]) std::memcpy(out, g_hold[board] + lane * 480u, 480u);
    else std::memset(out, 0, 480u);
    return v::kPackedBytesPerLane;
  }
  const std::uint32_t frame = g_frame - 1u;
  for (unsigned i = 0; i < v::kPixelsPerHalf; ++i) {
    const LgfsPixel p = lgfs_pixel(&g_live, frame, static_cast<int>(board), static_cast<int>(half * v::kPixelsPerHalf + i));
    v::packPixel(v::Pixel16{p.r, p.g, p.b}, out + i * v::kPackedBytesPerPixel);
  }
  return v::kPackedBytesPerLane;
}

inline void lgfs_note_sent() { ++g_frames_sent; }

// Realtime owner: after a submission of `board` was accepted.
inline void lgfs_log(unsigned board, const std::uint8_t* din0, const std::uint8_t* din1,
                     std::uint32_t generation, std::uint64_t time_us) {
  if (board > 1u || !din0 || !din1) return;
  const std::uint32_t seq = g_log_seq + 1u;
  LogEntry& e = g_log[seq % K1_LGFS_LOG_ENTRIES];
  e.seq = seq; e.board = static_cast<std::uint16_t>(board);
  e.flags = static_cast<std::uint16_t>((g_live_on ? 1u : 0u) | (g_live_on && g_live.card == LGFS_FREEZE ? 2u : 0u));
  e.generation = generation; e.card_frame = g_live_on ? g_frame : 0u; e.time_us = time_us;
  std::memcpy(e.lanes, din0, 480u); std::memcpy(e.lanes + 480u, din1, 480u);
  g_log_seq = seq; g_log_last[board] = seq;
  if (board == 0u) ++g_frames_sent;
}

// Opcode 29: payload u32 version 1, u32 after_seq. Reply: 24-byte header
// {u32 version 2, u32 oldest_seq, u32 newest_seq, u32 count, u32 entry_bytes, u32 capacity}
// then `count` entries with seq > after_seq, oldest first, as many as fit in `cap`.
// A gap (after_seq < oldest_seq - 1) means the host polled too slowly; it is visible
// in the sequence numbers and the host reports it.
inline std::size_t lgfs_log_read(std::uint32_t after_seq, std::uint8_t* out, std::size_t cap) {
  auto put = [](std::uint8_t* p, std::uint32_t v) { for (unsigned i = 0; i < 4u; ++i) p[i] = static_cast<std::uint8_t>(v >> (8u * i)); };
  if (!out || cap < 24u) return 0u;
  const std::uint32_t newest = g_log_seq;
  const std::uint32_t oldest = newest > K1_LGFS_LOG_ENTRIES - 1u ? newest - (K1_LGFS_LOG_ENTRIES - 1u) : 1u;
  std::uint32_t first = after_seq + 1u;
  if (first < oldest) first = oldest;
  std::uint32_t count = 0u;
  std::size_t n = 24u;
  for (std::uint32_t q = first; newest != 0u && q <= newest && n + sizeof(LogEntry) <= cap; ++q, ++count) {
    std::memcpy(out + n, &g_log[q % K1_LGFS_LOG_ENTRIES], sizeof(LogEntry));
    n += sizeof(LogEntry);
  }
  put(out, 2u); put(out + 4, newest ? oldest : 0u); put(out + 8, newest); put(out + 12, count);
  put(out + 16, static_cast<std::uint32_t>(sizeof(LogEntry))); put(out + 20, K1_LGFS_LOG_ENTRIES);
  return n;
}

// Fixture command: ASCII selection in, JSON reply out. Returns bytes written, or -1.
inline int lgfs_command(const char* text, char* json, std::size_t n, bool emit_enabled, bool top_enabled) {
  char canon[LGFS_TEXT_MAX];
  char err[32];
  if (lgfs__ieq(text, "status")) {
    const Slot s = g_slot[g_pub];
    if (s.active) lgfs_format(&s.sel, canon, sizeof canon); else std::strcpy(canon, "off");
    return std::snprintf(json, n,
        "{\"op\":28,\"ok\":true,\"lgfs\":%d,\"card\":\"%s\",\"edges\":2,\"leds\":160,"
        "\"cap_q16\":%lu,\"power_q16\":%lu,\"frame\":%lu,\"frames_sent\":%lu,\"log_seq\":%lu,\"log_capacity\":%lu,\"emit\":%d,\"top_output\":%d,"
        "\"map\":\"edge0=board0 primary P601/P604; edge1=board1 top\",\"scaling\":\"none\"}",
        LGFS_VERSION, canon, (unsigned long)LGFS_POWER_CAP_Q16,
        (unsigned long)(s.active ? lgfs_selection_power_q16(&s.sel) : 0u),
        (unsigned long)g_frame, (unsigned long)g_frames_sent, (unsigned long)g_log_seq,
        (unsigned long)K1_LGFS_LOG_ENTRIES, emit_enabled ? 1 : 0, top_enabled ? 1 : 0);
  }
  LgfsSelection sel;
  if (lgfs_parse(text, &sel, err, sizeof err) != 0)
    return std::snprintf(json, n, "{\"op\":28,\"ok\":false,\"error\":\"%s\"}", err);
  const bool on = sel.card != LGFS_OFF;
  publish(on ? &sel : nullptr, on);
  if (on) lgfs_format(&sel, canon, sizeof canon); else std::strcpy(canon, "off");
  return std::snprintf(json, n, "{\"op\":28,\"ok\":true,\"card\":\"%s\",\"power_q16\":%lu,\"emit\":%d,\"top_output\":%d}",
                       canon, (unsigned long)(on ? lgfs_selection_power_q16(&sel) : 0u),
                       emit_enabled ? 1 : 0, top_enabled ? 1 : 0);
}

}  // namespace k1::titan::lgfs

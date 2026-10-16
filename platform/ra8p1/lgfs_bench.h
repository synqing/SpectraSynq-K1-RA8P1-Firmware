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
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "lgfs_cards.h"

#define K1_LGFS_OPCODE 28u

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
  const std::uint32_t frame = g_frame - 1u;
  for (unsigned i = 0; i < v::kPixelsPerHalf; ++i) {
    const LgfsPixel p = lgfs_pixel(&g_live, frame, static_cast<int>(board), static_cast<int>(half * v::kPixelsPerHalf + i));
    v::packPixel(v::Pixel16{p.r, p.g, p.b}, out + i * v::kPackedBytesPerPixel);
  }
  return v::kPackedBytesPerLane;
}

inline void lgfs_note_sent() { ++g_frames_sent; }

// Fixture command: ASCII selection in, JSON reply out. Returns bytes written, or -1.
inline int lgfs_command(const char* text, char* json, std::size_t n, bool emit_enabled, bool top_enabled) {
  char canon[LGFS_TEXT_MAX];
  char err[32];
  if (lgfs__ieq(text, "status")) {
    const Slot s = g_slot[g_pub];
    if (s.active) lgfs_format(&s.sel, canon, sizeof canon); else std::strcpy(canon, "off");
    return std::snprintf(json, n,
        "{\"op\":28,\"ok\":true,\"lgfs\":%d,\"card\":\"%s\",\"edges\":2,\"leds\":160,"
        "\"cap_q16\":%lu,\"power_q16\":%lu,\"frame\":%lu,\"frames_sent\":%lu,\"emit\":%d,\"top_output\":%d,"
        "\"map\":\"edge0=board0 primary P601/P604; edge1=board1 top\",\"scaling\":\"none\"}",
        LGFS_VERSION, canon, (unsigned long)LGFS_POWER_CAP_Q16,
        (unsigned long)(s.active ? lgfs_selection_power_q16(&s.sel) : 0u),
        (unsigned long)g_frame, (unsigned long)g_frames_sent, emit_enabled ? 1 : 0, top_enabled ? 1 : 0);
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

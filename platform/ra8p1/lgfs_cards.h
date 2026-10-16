/* lgfs_cards.h — LGFS (light-guide film stack) optical test cards for the K1.
 *
 * SINGLE SOURCE OF TRUTH. The same file is compiled into
 *   - the ESP32-S3 firmware (SpectraSynq_K1_Firmware, env k1_b489_ws2816_lgfs_bench),
 *   - the Titan RA8P1 firmware (SpectraSynq-K1-RA8P1-Firmware, platform/ra8p1),
 *   - the host harness (host/lgfs_host.c) that makes the previews and the
 *     expected frames the analysis compares against.
 * Change it in the kit first, then copy verbatim. Bump LGFS_VERSION on any
 * change to the grammar or the rendering.
 *
 * Header-only. C99 and C++11. Includes only <stdint.h> <string.h> <stddef.h>.
 * No floating point, no heap, no globals, no large stack use: lgfs_pixel() is a
 * pure per-LED function, so firmware can pack straight to the wire.
 *
 * Geometry and values
 *   Two edges: 0 = top, 1 = bottom. 160 logical LEDs per edge, logical index =
 *   the firmware's buffer index 0..159 (DIN-A = 0..79, DIN-B = 80..159 on both
 *   boards; physical left/right and any reversal are recovered by the INDEX and
 *   CORNERS cards in the analysis, never assumed here).
 *   Pixel values are 16-bit WS2816 PWM codes 0..65535 per channel. They go to
 *   the wire as-is: no gamma, no dither, no look, no brightness scaling. The
 *   firmware's current limiter is the only stage allowed after lgfs_render(),
 *   and it must report its scale (see the briefs).
 *
 * Power cap
 *   lgfs_parse() refuses any selection whose summed codes exceed
 *   LGFS_POWER_CAP_Q16 / 65536 of "both edges full white" (320 LEDs x 3 x 65535).
 *   Default 0.094 = the 24/255 diagnostic ceiling. Evidence for the ceiling:
 *   S3 9087A500, 2026-08-27: both strips white at 140/255 tripped TG1WDT;
 *   80/255 held. Raise per board with -DLGFS_POWER_CAP_Q16=<q16> only on the
 *   Captain's say-so.
 *
 * Grammar (ASCII, case-insensitive, comma separated, no spaces needed):
 *   off
 *   blank
 *   index,<bit 0..7>[,<level>]              both edges; LED i on iff (i>>bit)&1
 *   corners[,<level>]                       top0 red, top159 green, bot0 blue, bot159 white
 *   wedge[,<edge>][,<ch>]                   16 blocks of 10 LEDs at codes 1,2,4,...,32768
 *   impulse,<edge>,<pos 0..159>[,<level>][,<ch>]   one LED
 *   pair,<edge>,<sep 1..79>[,<level>][,<ch>]       two LEDs about the centre, sep apart
 *   comb,<edge>,<period 2..80>[,<level>][,<ch>]    LED i on iff i % period == 0
 *   uniform,<edge>[,<level>][,<ch>]         every LED on the edge
 *   mix[,<pair rb|rg|gb|rw|bw>][,<level>]   top = first colour, bottom = second
 *   half,<edge>[,<level>][,<ch>]            LEDs 0..79 on, 80..159 off
 *   gradient,<edge>[,<max>][,<ch>]          code = max*i/159
 *   lowcodes[,<edge>]                       5 blocks of 32 LEDs at codes 1,2,4,8,16
 *   tick[,<level>]                          top LED 0 / LED 159 alternate every frame
 *   <edge>  = top|t|0, bottom|b|1, both|2 (both only where the card allows it)
 *   <ch>    = w (default) | r | g | b
 *   <level> = 0..65535 decimal, or max (= 65535)
 * Defaults: impulse/pair/comb/half/corners/tick 16384; index 8192;
 *           uniform/mix 6144 (= 24/255); gradient max 16384.
 */
#ifndef LGFS_CARDS_H
#define LGFS_CARDS_H

#include <stdint.h>
#include <string.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LGFS_VERSION 1
#define LGFS_EDGES 2
#define LGFS_LEDS 160
#define LGFS_FULL 65535u
#define LGFS_MAX_TOKENS 6
#define LGFS_TEXT_MAX 48

#ifndef LGFS_POWER_CAP_Q16
#define LGFS_POWER_CAP_Q16 6160u /* 0.094 * 65536: the 24/255 diagnostic ceiling */
#endif

typedef struct { uint16_t r, g, b; } LgfsPixel;

typedef enum {
  LGFS_OFF = 0, LGFS_BLANK, LGFS_INDEX, LGFS_CORNERS, LGFS_WEDGE, LGFS_IMPULSE,
  LGFS_PAIR, LGFS_COMB, LGFS_UNIFORM, LGFS_MIX, LGFS_HALF, LGFS_GRADIENT,
  LGFS_LOWCODES, LGFS_TICK, LGFS_CARD_COUNT
} LgfsCard;

enum { LGFS_EDGE_TOP = 0, LGFS_EDGE_BOTTOM = 1, LGFS_EDGE_BOTH = 2 };
enum { LGFS_CH_W = 0, LGFS_CH_R = 1, LGFS_CH_G = 2, LGFS_CH_B = 3 };
/* mix pairs: top colour, bottom colour */
enum { LGFS_MIX_RB = 0, LGFS_MIX_RG, LGFS_MIX_GB, LGFS_MIX_RW, LGFS_MIX_BW, LGFS_MIX_COUNT };

typedef struct {
  LgfsCard card;
  uint8_t edge;     /* LGFS_EDGE_* */
  uint8_t channel;  /* LGFS_CH_* */
  uint8_t pair;     /* LGFS_MIX_* (mix only) */
  uint16_t pos;     /* impulse position; index bit; pair sep; comb period */
  uint16_t level;   /* 16-bit code */
} LgfsSelection;

static const char* const LGFS_CARD_NAMES[LGFS_CARD_COUNT] = {
  "off", "blank", "index", "corners", "wedge", "impulse", "pair", "comb",
  "uniform", "mix", "half", "gradient", "lowcodes", "tick"
};
static const char* const LGFS_MIX_NAMES[LGFS_MIX_COUNT] = { "rb", "rg", "gb", "rw", "bw" };

static inline const char* lgfs_card_name(LgfsCard c) {
  return (c >= 0 && c < LGFS_CARD_COUNT) ? LGFS_CARD_NAMES[c] : "?";
}

/* ---- small helpers (no libc beyond string.h) ---- */
static inline int lgfs__ieq(const char* a, const char* b) {
  for (;; ++a, ++b) {
    int ca = *a, cb = *b;
    if (ca >= 'A' && ca <= 'Z') ca += 32;
    if (cb >= 'A' && cb <= 'Z') cb += 32;
    if (ca != cb) return 0;
    if (ca == 0) return 1;
  }
}
static inline int lgfs__uint(const char* s, uint32_t max, uint16_t* out) {
  uint32_t v = 0; int n = 0;
  if (lgfs__ieq(s, "max")) { *out = (uint16_t)max; return 1; }
  for (; *s; ++s, ++n) {
    if (*s < '0' || *s > '9' || n > 5) return 0;
    v = v * 10u + (uint32_t)(*s - '0');
    if (v > max) return 0;
  }
  if (n == 0) return 0;
  *out = (uint16_t)v; return 1;
}
static inline int lgfs__edge(const char* s, int allow_both, uint8_t* out) {
  if (lgfs__ieq(s, "top") || lgfs__ieq(s, "t") || lgfs__ieq(s, "0")) { *out = LGFS_EDGE_TOP; return 1; }
  if (lgfs__ieq(s, "bottom") || lgfs__ieq(s, "b") || lgfs__ieq(s, "1")) { *out = LGFS_EDGE_BOTTOM; return 1; }
  if (allow_both && (lgfs__ieq(s, "both") || lgfs__ieq(s, "2"))) { *out = LGFS_EDGE_BOTH; return 1; }
  return 0;
}
static inline int lgfs__ch(const char* s, uint8_t* out) {
  if (lgfs__ieq(s, "w")) { *out = LGFS_CH_W; return 1; }
  if (lgfs__ieq(s, "r")) { *out = LGFS_CH_R; return 1; }
  if (lgfs__ieq(s, "g")) { *out = LGFS_CH_G; return 1; }
  if (lgfs__ieq(s, "b")) { *out = LGFS_CH_B; return 1; }
  return 0;
}
static inline void lgfs__set_err(char* err, size_t n, const char* msg) {
  if (!err || !n) return;
  size_t i = 0;
  for (; msg[i] && i + 1 < n; ++i) err[i] = msg[i];
  err[i] = 0;
}
static inline LgfsPixel lgfs__px(uint8_t ch, uint16_t level) {
  LgfsPixel p = { 0, 0, 0 };
  switch (ch) {
    case LGFS_CH_R: p.r = level; break;
    case LGFS_CH_G: p.g = level; break;
    case LGFS_CH_B: p.b = level; break;
    default: p.r = p.g = p.b = level; break;
  }
  return p;
}

/* One pixel of a selection: edge 0 (top) or 1 (bottom), index 0..159.
 * Pure function of (selection, frame, edge, index): no arrays, no state.
 * frame only matters for TICK. */
static inline LgfsPixel lgfs_pixel(const LgfsSelection* s, uint32_t frame, int edge, int i) {
  static const LgfsPixel zero = { 0, 0, 0 };
  int on_edge;
  if (!s || edge < 0 || edge > 1 || i < 0 || i >= LGFS_LEDS) return zero;
  on_edge = (s->edge == LGFS_EDGE_BOTH) || ((int)s->edge == edge);
  switch (s->card) {
    case LGFS_INDEX:
      return ((i >> s->pos) & 1) ? lgfs__px(LGFS_CH_W, s->level) : zero;
    case LGFS_CORNERS:
      if (edge == 0 && i == 0) return lgfs__px(LGFS_CH_R, s->level);
      if (edge == 0 && i == LGFS_LEDS - 1) return lgfs__px(LGFS_CH_G, s->level);
      if (edge == 1 && i == 0) return lgfs__px(LGFS_CH_B, s->level);
      if (edge == 1 && i == LGFS_LEDS - 1) return lgfs__px(LGFS_CH_W, s->level);
      return zero;
    case LGFS_WEDGE:
      return on_edge ? lgfs__px(s->channel, (uint16_t)(1u << (i / 10))) : zero;
    case LGFS_IMPULSE:
      return (on_edge && i == (int)s->pos) ? lgfs__px(s->channel, s->level) : zero;
    case LGFS_PAIR: {
      const int a = (LGFS_LEDS / 2) - ((s->pos + 1) / 2), b = (LGFS_LEDS / 2) + (s->pos / 2);
      return (on_edge && (i == a || i == b)) ? lgfs__px(s->channel, s->level) : zero;
    }
    case LGFS_COMB:
      return (on_edge && i % s->pos == 0) ? lgfs__px(s->channel, s->level) : zero;
    case LGFS_UNIFORM:
      return on_edge ? lgfs__px(s->channel, s->level) : zero;
    case LGFS_MIX: {
      static const uint8_t pairs[LGFS_MIX_COUNT][2] = {
        { LGFS_CH_R, LGFS_CH_B }, { LGFS_CH_R, LGFS_CH_G }, { LGFS_CH_G, LGFS_CH_B },
        { LGFS_CH_R, LGFS_CH_W }, { LGFS_CH_B, LGFS_CH_W } };
      return lgfs__px(pairs[s->pair][edge], s->level);
    }
    case LGFS_HALF:
      return (on_edge && i < LGFS_LEDS / 2) ? lgfs__px(s->channel, s->level) : zero;
    case LGFS_GRADIENT:
      return on_edge ? lgfs__px(s->channel, (uint16_t)(((uint32_t)s->level * (uint32_t)i) / (LGFS_LEDS - 1))) : zero;
    case LGFS_LOWCODES:
      return on_edge ? lgfs__px(LGFS_CH_W, (uint16_t)(1u << (i / 32))) : zero;
    case LGFS_TICK:
      return (edge == 0 && i == ((frame & 1u) ? (LGFS_LEDS - 1) : 0)) ? lgfs__px(LGFS_CH_W, s->level) : zero;
    case LGFS_OFF: case LGFS_BLANK: default:
      return zero;
  }
}

/* Render a selection into both edges (convenience for hosts and MCUs with RAM to spare). */
static inline void lgfs_render(const LgfsSelection* s, uint32_t frame,
                               LgfsPixel top[LGFS_LEDS], LgfsPixel bottom[LGFS_LEDS]) {
  for (int i = 0; i < LGFS_LEDS; ++i) {
    top[i] = lgfs_pixel(s, frame, 0, i);
    bottom[i] = lgfs_pixel(s, frame, 1, i);
  }
}

#define LGFS__FULL_SUM (2ull * LGFS_LEDS * 3ull * LGFS_FULL)  /* both edges full white */

/* Summed codes as a fraction of both edges full white, Q16 (65536 = 1.0). */
static inline uint32_t lgfs_power_q16(const LgfsPixel top[LGFS_LEDS], const LgfsPixel bottom[LGFS_LEDS]) {
  uint64_t sum = 0;
  for (int i = 0; i < LGFS_LEDS; ++i) {
    sum += (uint64_t)top[i].r + top[i].g + top[i].b;
    sum += (uint64_t)bottom[i].r + bottom[i].g + bottom[i].b;
  }
  return (uint32_t)((sum << 16) / LGFS__FULL_SUM);
}

/* Same figure straight from a selection, without arrays (frame 0; TICK is one LED either way). */
static inline uint32_t lgfs_selection_power_q16(const LgfsSelection* s) {
  uint64_t sum = 0;
  for (int e = 0; e < 2; ++e)
    for (int i = 0; i < LGFS_LEDS; ++i) {
      const LgfsPixel p = lgfs_pixel(s, 0, e, i);
      sum += (uint64_t)p.r + p.g + p.b;
    }
  return (uint32_t)((sum << 16) / LGFS__FULL_SUM);
}

/* Parse text into a selection. Returns 0 on success, -1 on error (err filled).
 * Enforces the power cap by rendering once; MCUs call this on a command, not
 * per frame. */
static inline int lgfs_parse(const char* text, LgfsSelection* out, char* err, size_t errlen) {
  char buf[LGFS_TEXT_MAX];
  char* tok[LGFS_MAX_TOKENS];
  int ntok = 0;
  LgfsSelection s;
  size_t len;
  if (!text || !out) { lgfs__set_err(err, errlen, "no text"); return -1; }
  len = strlen(text);
  if (len == 0 || len >= sizeof(buf)) { lgfs__set_err(err, errlen, "text length"); return -1; }
  memcpy(buf, text, len + 1);
  /* trim trailing CR/LF/space */
  while (len && (buf[len - 1] == '\r' || buf[len - 1] == '\n' || buf[len - 1] == ' ')) buf[--len] = 0;
  tok[ntok++] = buf;
  for (size_t i = 0; i < len; ++i) {
    if (buf[i] == ',') {
      buf[i] = 0;
      if (ntok >= LGFS_MAX_TOKENS) { lgfs__set_err(err, errlen, "too many args"); return -1; }
      tok[ntok++] = &buf[i + 1];
    }
  }
  memset(&s, 0, sizeof s);
  s.card = LGFS_CARD_COUNT;
  for (int c = 0; c < LGFS_CARD_COUNT; ++c)
    if (lgfs__ieq(tok[0], LGFS_CARD_NAMES[c])) { s.card = (LgfsCard)c; break; }
  if (s.card == LGFS_CARD_COUNT) { lgfs__set_err(err, errlen, "unknown card"); return -1; }

#define LGFS__NEED(n) if (ntok < (n) + 1) { lgfs__set_err(err, errlen, "missing arg"); return -1; }
#define LGFS__OPT_LEVEL(k, def) do { s.level = (def); \
    if (ntok > (k) && !lgfs__uint(tok[k], LGFS_FULL, &s.level)) { lgfs__set_err(err, errlen, "bad level"); return -1; } } while (0)
#define LGFS__OPT_CH(k) do { s.channel = LGFS_CH_W; \
    if (ntok > (k) && !lgfs__ch(tok[k], &s.channel)) { lgfs__set_err(err, errlen, "bad channel"); return -1; } } while (0)
#define LGFS__MAXTOK(n) if (ntok > (n)) { lgfs__set_err(err, errlen, "too many args"); return -1; }

  switch (s.card) {
    case LGFS_OFF: case LGFS_BLANK:
      LGFS__MAXTOK(1); s.edge = LGFS_EDGE_BOTH; break;
    case LGFS_INDEX:
      LGFS__NEED(1); LGFS__MAXTOK(3); s.edge = LGFS_EDGE_BOTH;
      if (!lgfs__uint(tok[1], 7, &s.pos)) { lgfs__set_err(err, errlen, "bit 0..7"); return -1; }
      LGFS__OPT_LEVEL(2, 8192); break;
    case LGFS_CORNERS:
      LGFS__MAXTOK(2); s.edge = LGFS_EDGE_BOTH; LGFS__OPT_LEVEL(1, 16384); break;
    case LGFS_WEDGE:
      LGFS__MAXTOK(3); s.edge = LGFS_EDGE_TOP;
      if (ntok > 1 && !lgfs__edge(tok[1], 0, &s.edge)) { lgfs__set_err(err, errlen, "bad edge"); return -1; }
      LGFS__OPT_CH(2); s.level = 32768; break;
    case LGFS_IMPULSE:
      LGFS__NEED(2); LGFS__MAXTOK(5);
      if (!lgfs__edge(tok[1], 1, &s.edge)) { lgfs__set_err(err, errlen, "bad edge"); return -1; }
      if (!lgfs__uint(tok[2], LGFS_LEDS - 1, &s.pos)) { lgfs__set_err(err, errlen, "pos 0..159"); return -1; }
      LGFS__OPT_LEVEL(3, 16384); LGFS__OPT_CH(4); break;
    case LGFS_PAIR:
      LGFS__NEED(2); LGFS__MAXTOK(5);
      if (!lgfs__edge(tok[1], 1, &s.edge)) { lgfs__set_err(err, errlen, "bad edge"); return -1; }
      if (!lgfs__uint(tok[2], 79, &s.pos) || s.pos == 0) { lgfs__set_err(err, errlen, "sep 1..79"); return -1; }
      LGFS__OPT_LEVEL(3, 16384); LGFS__OPT_CH(4); break;
    case LGFS_COMB:
      LGFS__NEED(2); LGFS__MAXTOK(5);
      if (!lgfs__edge(tok[1], 1, &s.edge)) { lgfs__set_err(err, errlen, "bad edge"); return -1; }
      if (!lgfs__uint(tok[2], 80, &s.pos) || s.pos < 2) { lgfs__set_err(err, errlen, "period 2..80"); return -1; }
      LGFS__OPT_LEVEL(3, 16384); LGFS__OPT_CH(4); break;
    case LGFS_UNIFORM:
      LGFS__NEED(1); LGFS__MAXTOK(4);
      if (!lgfs__edge(tok[1], 1, &s.edge)) { lgfs__set_err(err, errlen, "bad edge"); return -1; }
      LGFS__OPT_LEVEL(2, 6144); LGFS__OPT_CH(3); break;
    case LGFS_MIX:
      LGFS__MAXTOK(3); s.edge = LGFS_EDGE_BOTH; s.pair = LGFS_MIX_RB;
      if (ntok > 1) {
        int found = 0;
        for (int p = 0; p < LGFS_MIX_COUNT; ++p) if (lgfs__ieq(tok[1], LGFS_MIX_NAMES[p])) { s.pair = (uint8_t)p; found = 1; }
        if (!found) { lgfs__set_err(err, errlen, "pair rb|rg|gb|rw|bw"); return -1; }
      }
      LGFS__OPT_LEVEL(2, 6144); break;
    case LGFS_HALF:
      LGFS__NEED(1); LGFS__MAXTOK(4);
      if (!lgfs__edge(tok[1], 1, &s.edge)) { lgfs__set_err(err, errlen, "bad edge"); return -1; }
      LGFS__OPT_LEVEL(2, 16384); LGFS__OPT_CH(3); break;
    case LGFS_GRADIENT:
      LGFS__NEED(1); LGFS__MAXTOK(4);
      if (!lgfs__edge(tok[1], 1, &s.edge)) { lgfs__set_err(err, errlen, "bad edge"); return -1; }
      LGFS__OPT_LEVEL(2, 16384); LGFS__OPT_CH(3); break;
    case LGFS_LOWCODES:
      LGFS__MAXTOK(2); s.edge = LGFS_EDGE_TOP;
      if (ntok > 1 && !lgfs__edge(tok[1], 1, &s.edge)) { lgfs__set_err(err, errlen, "bad edge"); return -1; }
      s.level = 16; break;
    case LGFS_TICK:
      LGFS__MAXTOK(2); s.edge = LGFS_EDGE_TOP; LGFS__OPT_LEVEL(1, 16384); break;
    default:
      lgfs__set_err(err, errlen, "unknown card"); return -1;
  }
#undef LGFS__NEED
#undef LGFS__OPT_LEVEL
#undef LGFS__OPT_CH
#undef LGFS__MAXTOK

  if (lgfs_selection_power_q16(&s) > LGFS_POWER_CAP_Q16) {
    lgfs__set_err(err, errlen, "power cap"); return -1;
  }
  *out = s;
  return 0;
}

/* Canonical text of a selection (what status should echo). Returns chars written. */
static inline int lgfs_format(const LgfsSelection* s, char* buf, size_t n) {
  /* tiny formatter: avoids snprintf on the MCUs */
  char tmp[LGFS_TEXT_MAX]; size_t k = 0;
  const char* name = lgfs_card_name(s->card);
  static const char* const edges[3] = { "top", "bottom", "both" };
  static const char chs[4] = { 'w', 'r', 'g', 'b' };
#define LGFS__PUTC(ch) do { if (k + 1 < sizeof tmp) tmp[k++] = (ch); } while (0)
#define LGFS__PUT(str) do { const char* q = (str); while (*q && k + 1 < sizeof tmp) tmp[k++] = *q++; } while (0)
#define LGFS__PUTN(v) do { char d[6]; int m = 0; uint32_t x = (v); do { d[m++] = (char)('0' + x % 10); x /= 10; } while (x); \
    while (m && k + 1 < sizeof tmp) tmp[k++] = d[--m]; } while (0)
  LGFS__PUT(name);
  switch (s->card) {
    case LGFS_INDEX: LGFS__PUT(","); LGFS__PUTN(s->pos); LGFS__PUT(","); LGFS__PUTN(s->level); break;
    case LGFS_CORNERS: case LGFS_TICK: LGFS__PUT(","); LGFS__PUTN(s->level); break;
    case LGFS_WEDGE: LGFS__PUT(","); LGFS__PUT(edges[s->edge]); LGFS__PUT(","); LGFS__PUTC(chs[s->channel]); break;
    case LGFS_IMPULSE: case LGFS_PAIR: case LGFS_COMB:
      LGFS__PUT(","); LGFS__PUT(edges[s->edge]); LGFS__PUT(","); LGFS__PUTN(s->pos);
      LGFS__PUT(","); LGFS__PUTN(s->level); LGFS__PUT(","); LGFS__PUTC(chs[s->channel]); break;
    case LGFS_UNIFORM: case LGFS_HALF: case LGFS_GRADIENT:
      LGFS__PUT(","); LGFS__PUT(edges[s->edge]); LGFS__PUT(","); LGFS__PUTN(s->level);
      LGFS__PUT(","); LGFS__PUTC(chs[s->channel]); break;
    case LGFS_MIX: LGFS__PUT(","); LGFS__PUT(LGFS_MIX_NAMES[s->pair]); LGFS__PUT(","); LGFS__PUTN(s->level); break;
    case LGFS_LOWCODES: LGFS__PUT(","); LGFS__PUT(edges[s->edge]); break;
    default: break;
  }
#undef LGFS__PUTC
#undef LGFS__PUT
#undef LGFS__PUTN
  if (k >= sizeof tmp) k = sizeof tmp - 1;
  tmp[k] = 0;
  if (!buf || !n) return (int)k;
  if (k + 1 > n) k = n - 1;
  memcpy(buf, tmp, k); buf[k] = 0;
  return (int)k;
}

#ifdef __cplusplus
}
#endif
#endif /* LGFS_CARDS_H */

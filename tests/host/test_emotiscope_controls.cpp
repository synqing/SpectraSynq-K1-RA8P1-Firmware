#include "emotiscope_control.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace ec = k1::emotiscope;

static int g_fails = 0;

#define CHECK(cond) do { \
  if (!(cond)) { \
    std::fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); \
    ++g_fails; \
  } \
} while (0)

static void check_f(const char* what, float got, float want) {
  if (!(std::fabs(got - want) <= 1e-5f)) {
    std::fprintf(stderr, "FAIL %s got %.9g want %.9g\n", what, got, want);
    ++g_fails;
  }
}

static std::uint32_t fbits(float value) {
  std::uint32_t bits = 0;
  std::memcpy(&bits, &value, sizeof(bits));
  return bits;
}

struct Entry { std::uint16_t field, kind; std::uint32_t raw; };

static std::vector<std::uint8_t> build_patch(std::uint32_t id, std::uint32_t revision,
                                            std::uint32_t boot, const Entry* entries, int count) {
  std::uint32_t mask = 0;
  for (int i = 0; i < count; ++i) mask |= 1u << (entries[i].field - 1u);
  std::vector<std::uint8_t> out(24u + 8u * static_cast<unsigned>(count));
  std::uint8_t* p = out.data();
  ec::donorPutU32(p + 0, ec::kDonorCfgPatch);
  ec::donorPutU32(p + 4, revision);
  ec::donorPutU32(p + 8, id);
  ec::donorPutU32(p + 12, boot);
  ec::donorPutU32(p + 16, mask);
  ec::donorPutU32(p + 20, static_cast<std::uint32_t>(count));
  for (int i = 0; i < count; ++i) {
    std::uint8_t* q = p + 24 + 8 * i;
    ec::donorPutU16(q, entries[i].field);
    ec::donorPutU16(q + 2, entries[i].kind);
    ec::donorPutU32(q + 4, entries[i].raw);
  }
  return out;
}

static std::vector<std::uint8_t> empty_patch(std::uint32_t id, std::uint32_t revision, std::uint32_t boot) {
  return build_patch(id, revision, boot, nullptr, 0);
}

static void build_get(std::uint8_t out[16], std::uint32_t id, std::uint32_t boot) {
  std::memset(out, 0, 16);
  ec::donorPutU32(out + 0, ec::kDonorCfgGetState);
  ec::donorPutU32(out + 4, id);
  ec::donorPutU32(out + 8, boot);
}

static void adopt(ec::DonorObserved* observed, const ec::DonorIntent& intent, const ec::DonorApplyResult& result) {
  if (result.revision_changed) {
    observed->revision = result.revision;
    observed->accepted_request_id = intent.request_id;
  }
  if (result.brightness_changed) observed->hardware_brightness = result.hardware_brightness;
  if (result.emission_changed) observed->emission = result.emission;
  if (result.output_precision_changed) observed->output_precision = result.output_precision;
  if (result.palette_changed) observed->palette = result.palette;
  if (result.intensity_a_changed) observed->output_intensity_a = result.output_intensity_a;
  if (result.intensity_b_changed) observed->output_intensity_b = result.output_intensity_b;
}

static ec::DonorApplyResult apply_bytes(ec::DonorDedup* dedup, ec::DonorObserved* observed,
                                        const std::uint8_t* bytes, std::uint32_t size) {
  ec::DonorApplyResult result{};
  ec::DonorIntent intent{};
  const std::uint32_t parsed = ec::donorParse(bytes, size, &intent);
  if (parsed != 0u) {
    result.status = 1000u + parsed;
    return result;
  }
  ec::donorApply(dedup, intent, *observed, &result);
  adopt(observed, intent, result);
  return result;
}

static ec::DonorApplyResult apply_vec(ec::DonorDedup* dedup, ec::DonorObserved* observed,
                                      const std::vector<std::uint8_t>& bytes) {
  return apply_bytes(dedup, observed, bytes.data(), static_cast<std::uint32_t>(bytes.size()));
}

static void print_hex(const char* tag, const std::uint8_t* bytes, std::size_t size) {
  std::fputs(tag, stdout);
  std::fputc(' ', stdout);
  for (std::size_t i = 0; i < size; ++i) std::printf("%02x", bytes[i]);
  std::fputc('\n', stdout);
}

static ec::DonorObserved fresh_observed() {
  ec::DonorObserved observed{};
  observed.boot_id = 0x11u;
  observed.hardware_brightness = 100u;
  observed.emission = 1u;
  observed.output_intensity_a = observed.output_intensity_b = 1.0f;
  return observed;
}

static void check_default_body(const std::uint8_t* body) {
  CHECK(ec::donorGetU32(body + 0) == ec::kDonorStateMagic);
  CHECK(ec::donorGetU32(body + 4) == 2u);
  CHECK(ec::donorGetU32(body + 8) == 1u);
  CHECK(ec::donorGetU32(body + 12) == 0x11u);
  CHECK(ec::donorGetU32(body + 16) == 0u);
  check_f("brightness", ec::donorGetF32(body + 24), 1.0f);
  check_f("softness", ec::donorGetF32(body + 28), 0.25f);
  check_f("color", ec::donorGetF32(body + 32), 0.33f);
  check_f("warmth", ec::donorGetF32(body + 36), 0.0f);
  check_f("range", ec::donorGetF32(body + 40), 0.0f);
  check_f("speed", ec::donorGetF32(body + 44), 0.50f);
  check_f("saturation", ec::donorGetF32(body + 48), 0.75f);
  check_f("background", ec::donorGetF32(body + 52), 0.25f);
  check_f("hue", ec::donorGetF32(body + 56), 0.33f);
  CHECK(ec::donorGetU32(body + 60) == 1u);
  CHECK(ec::donorGetU32(body + 64) == 39u);
  CHECK(ec::donorGetU32(body + 68) == 100u);
  CHECK(ec::donorGetU32(body + 72) == 0xDFDFu);
  CHECK(ec::donorGetU32(body + 92) == 0u);
  CHECK(ec::donorGetU32(body + 96) == 0u);
  CHECK(ec::donorGetU32(body + 100) == 0u);
  CHECK(ec::donorGetU32(body + 104) == 0u);
  CHECK(ec::donorGetU32(body + 140) == 0u);
  check_f("intensity A", ec::donorGetF32(body + 144), 1.0f);
  check_f("intensity B", ec::donorGetF32(body + 148), 1.0f);
  for (unsigned i = 152; i < 160; ++i) CHECK(body[i] == 0u);
}

static int behavioral() {
  CHECK(k1_emotiscope_init(1u));
  CHECK(!k1_emotiscope_init(2u));
  const char* names[] = {"Analog", "Spectrum", "Octave", "Metronome", "Spectronome", "Hype", "Bloom"};
  for (std::uint32_t mode = 0; mode < 7u; ++mode)
    CHECK(std::strcmp(k1_emotiscope_mode_name(mode), names[mode]) == 0);
  CHECK(std::strcmp(k1_emotiscope_mode_name(7u), "Neutral") == 0);
  CHECK(std::strcmp(k1_emotiscope_mode_name(8u), "Self Test") == 0);

  const std::uint8_t crc_sample[] = {'1','2','3','4','5','6','7','8','9'};
  const std::uint32_t crc_vector = ec::donorCrc32(crc_sample, 9);
  CHECK(crc_vector == 0xCBF43926u);
  std::printf("CRC_VECTOR %08x\n", crc_vector);

  std::uint8_t golden[32];
  ec::donorEncodeGoldenPatch(golden);
  static const std::uint8_t frozen[32] = {
    0x02,0x01,0x4d,0x45, 0,0,0,0, 1,0,0,0, 0x11,0,0,0,
    1,0,0,0, 1,0,0,0, 1,0, 1,0, 0,0,0,0x3f
  };
  CHECK(std::memcmp(golden, frozen, 32) == 0);
  print_hex("GOLDEN", golden, 32);
  CHECK(ec::donorProfileRejectsPaletteConfigure());
  CHECK(ec::donorProfileAcceptsConfigSub(ec::kDonorCfgGetState));
  CHECK(ec::donorProfileAcceptsConfigSub(ec::kDonorCfgPatch));
  for (std::uint32_t sub = 1; sub <= 12u; ++sub) CHECK(!ec::donorProfileAcceptsConfigSub(sub));

  K1EmotiscopeConfig before{};
  k1_emotiscope_get_config(&before);
  std::uint32_t changed = 0, transition = 0;
  K1EmotiscopePatch noop{};
  CHECK(k1_emotiscope_patch(&noop, &changed, &transition));
  CHECK(changed == 0u && transition == 0u);
  K1EmotiscopePatch rejected{};
  rejected.mask = 0x4001u;
  rejected.brightness = 0.1f;
  CHECK(!k1_emotiscope_patch(&rejected, &changed, &transition));
  rejected.mask = 3u;
  rejected.brightness = 0.2f;
  rejected.softness = NAN;
  CHECK(!k1_emotiscope_patch(&rejected, &changed, &transition));
  rejected = {};
  rejected.mask = 256u;
  rejected.current_mode = 7;
  CHECK(!k1_emotiscope_patch(&rejected, &changed, &transition));
  rejected.current_mode = 8;
  CHECK(!k1_emotiscope_patch(&rejected, &changed, &transition));
  rejected.current_mode = -1;
  CHECK(!k1_emotiscope_patch(&rejected, &changed, &transition));
  K1EmotiscopeConfig after_reject{};
  k1_emotiscope_get_config(&after_reject);
  CHECK(std::memcmp(&before, &after_reject, sizeof(before)) == 0);

  ec::DonorDedup ring{};
  ec::DonorObserved ring_obs = fresh_observed();
  const auto first = empty_patch(1u, 0u, 0x11u);
  ec::DonorApplyResult result = apply_vec(&ring, &ring_obs, first);
  CHECK(result.status == 0u && result.changed == 0u && ring_obs.revision == 0u);
  result = apply_vec(&ring, &ring_obs, first);
  CHECK(result.status == 0u);
  CHECK(ec::donorGetU32(result.response + 96) == 1u);
  const Entry conflict_entry{1u, ec::kDonorKindF32, fbits(0.5f)};
  const auto conflict = build_patch(1u, 0u, 0x11u, &conflict_entry, 1);
  result = apply_vec(&ring, &ring_obs, conflict);
  CHECK(result.status == ec::kDonorStatusConflict);
  K1EmotiscopeConfig still{};
  k1_emotiscope_get_config(&still);
  check_f("unchanged brightness", still.brightness, 1.0f);
  result = apply_vec(&ring, &ring_obs, first);
  CHECK(ec::donorGetU32(result.response + 96) == 1u);
  for (std::uint32_t id = 2; id <= 64u; ++id) {
    result = apply_vec(&ring, &ring_obs, empty_patch(id, 0u, 0x11u));
    CHECK(result.status == 0u);
  }
  result = apply_vec(&ring, &ring_obs, first);
  CHECK(result.status == ec::kDonorStatusReconcile);
  check_f("tomb brightness", still.brightness, 1.0f);
  k1_emotiscope_get_config(&still);
  check_f("tomb brightness live", still.brightness, 1.0f);
  result = apply_vec(&ring, &ring_obs, empty_patch(65u, 0u, 0x11u));
  CHECK(result.status == 0u);
  result = apply_vec(&ring, &ring_obs, first);
  CHECK(result.status == 0u);
  CHECK(ec::donorGetU32(result.response + 96) == 0u);

  ec::DonorDedup dedup{};
  ec::DonorObserved observed = fresh_observed();
  std::uint8_t get1[16];
  build_get(get1, 1u, 0x11u);
  result = apply_bytes(&dedup, &observed, get1, 16u);
  CHECK(result.status == 0u && result.changed == 0u && observed.revision == 0u);
  check_default_body(result.response);
  print_hex("DEFAULT", result.response, ec::kDonorStateBytes);
  result = apply_bytes(&dedup, &observed, get1, 16u);
  CHECK(result.status == 0u && ec::donorGetU32(result.response + 96) == 1u);

  CHECK(k1_emotiscope_test_lpf_drag() == 0.0f);
  k1_emotiscope_test_set_novelty_sample(0.37f);
  const Entry same_mode{9u, ec::kDonorKindI32, 1u};
  result = apply_vec(&dedup, &observed, build_patch(2u, observed.revision, 0x11u, &same_mode, 1));
  CHECK(result.status == 0u && result.mode_transition == 0u && observed.revision == 0u);
  CHECK(k1_emotiscope_test_lpf_drag() == 0.0f);
  CHECK(k1_emotiscope_test_novelty_sample() == 0.37f);
  const Entry analog{9u, ec::kDonorKindI32, 0u};
  result = apply_vec(&dedup, &observed, build_patch(3u, observed.revision, 0x11u, &analog, 1));
  CHECK(result.status == 0u && result.mode_changed == 1u && result.mode_transition == 1u);
  CHECK(result.mode == 0);
  CHECK(k1_emotiscope_test_lpf_drag() == 1.0f);
  CHECK(k1_emotiscope_test_novelty_sample() == 0.37f);
  CHECK(ec::donorGetU32(result.response + 72) == 0xDFFFu);
  const Entry screen_off{11u, ec::kDonorKindBool, 0u};
  result = apply_vec(&dedup, &observed, build_patch(4u, observed.revision, 0x11u, &screen_off, 1));
  CHECK(result.status == 0u);
  CHECK(k1_emotiscope_test_novelty_sample() == 0.37f);
  const Entry screen_on{11u, ec::kDonorKindBool, 1u};
  result = apply_vec(&dedup, &observed, build_patch(5u, observed.revision, 0x11u, &screen_on, 1));
  CHECK(result.status == 0u);
  CHECK(k1_emotiscope_test_novelty_sample() == 0.37f);
  CHECK(k1_emotiscope_test_lpf_drag() == 1.0f);

  k1_emotiscope_test_set_raw_color(1.4f);
  const Entry soft{2u, ec::kDonorKindF32, fbits(0.40f)};
  const std::uint32_t revision_before_color = observed.revision;
  result = apply_vec(&dedup, &observed, build_patch(6u, observed.revision, 0x11u, &soft, 1));
  CHECK(result.status == 0u && observed.revision == revision_before_color + 1u);
  K1EmotiscopeConfig raw{};
  k1_emotiscope_get_config(&raw);
  check_f("raw color", raw.color, 1.4f);
  check_f("raw hue", k1_emotiscope_effective_hue(), 0.4f);
  check_f("softness", raw.softness, 0.40f);
  std::uint8_t get_raw[16];
  build_get(get_raw, 7u, 0x11u);
  result = apply_bytes(&dedup, &observed, get_raw, 16u);
  CHECK(result.status == 0u && result.changed == 0u);
  check_f("wire color", ec::donorGetF32(result.response + 32), 1.4f);
  check_f("wire hue", ec::donorGetF32(result.response + 56), 0.4f);
  print_hex("RAWGET", result.response, ec::kDonorStateBytes);
  const Entry hue{3u, ec::kDonorKindF32, fbits(0.20f)};
  result = apply_vec(&dedup, &observed, build_patch(8u, observed.revision, 0x11u, &hue, 1));
  CHECK(result.status == 0u);
  k1_emotiscope_get_config(&raw);
  check_f("replaced color", raw.color, 0.20f);
  check_f("replaced hue", k1_emotiscope_effective_hue(), 0.20f);
  raw.color = 1.4f;
  CHECK(!k1_emotiscope_set_config(&raw));
  k1_emotiscope_get_config(&raw);
  check_f("set_config kept", raw.color, 0.20f);
  K1EmotiscopeConfig mode7 = raw;
  mode7.current_mode = 7;
  CHECK(k1_emotiscope_set_config(&mode7));
  mode7.current_mode = 8;
  mode7.color = 0.20f;
  CHECK(!k1_emotiscope_set_config(&mode7));
  k1_emotiscope_get_config(&raw);
  CHECK(raw.current_mode == 7);
  const Entry back{9u, ec::kDonorKindI32, 1u};
  result = apply_vec(&dedup, &observed, build_patch(80u, observed.revision, 0x11u, &back, 1));
  CHECK(result.status == 0u && result.mode == 1);
  k1_emotiscope_test_set_raw_color(-1.2f);
  check_f("negative hue", k1_emotiscope_effective_hue(), 0.8f);
  const Entry soft_again{2u, ec::kDonorKindF32, fbits(0.40f)};
  const std::uint32_t rev_neg = observed.revision;
  result = apply_vec(&dedup, &observed, build_patch(9u, observed.revision, 0x11u, &soft_again, 1));
  CHECK(result.status == 0u && observed.revision == rev_neg);
  k1_emotiscope_get_config(&raw);
  check_f("kept negative", raw.color, -1.2f);
  const Entry restore_color{3u, ec::kDonorKindF32, fbits(0.33f)};
  result = apply_vec(&dedup, &observed, build_patch(81u, observed.revision, 0x11u, &restore_color, 1));
  CHECK(result.status == 0u);

  const Entry bloom{9u, ec::kDonorKindI32, 6u};
  result = apply_vec(&dedup, &observed, build_patch(10u, observed.revision, 0x11u, &bloom, 1));
  CHECK(result.status == 0u);
  CHECK(ec::donorGetU32(result.response + 72) == 0xDFDFu);
  const Entry ranged{5u, ec::kDonorKindF32, fbits(0.50f)};
  result = apply_vec(&dedup, &observed, build_patch(11u, observed.revision, 0x11u, &ranged, 1));
  CHECK(ec::donorGetU32(result.response + 72) == 0xFFDFu);
  result = apply_vec(&dedup, &observed, build_patch(12u, observed.revision, 0x11u, &analog, 1));
  CHECK(ec::donorGetU32(result.response + 72) == 0xFFFFu);
  const Entry clear_range{5u, ec::kDonorKindF32, fbits(0.0f)};
  result = apply_vec(&dedup, &observed, build_patch(13u, observed.revision, 0x11u, &clear_range, 1));
  CHECK(ec::donorGetU32(result.response + 72) == 0xDFFFu);

  const float donor_before = 1.0f;
  k1_emotiscope_get_config(&raw);
  check_f("donor still full", raw.brightness, donor_before);
  const Entry output{15u, ec::kDonorKindU32, 90u};
  result = apply_vec(&dedup, &observed, build_patch(15u, observed.revision, 0x11u, &output, 1));
  CHECK(result.status == 0u && result.brightness_changed == 1u && result.hardware_brightness == 90u);
  CHECK(observed.hardware_brightness == 90u);
  k1_emotiscope_get_config(&raw);
  check_f("donor untouched", raw.brightness, 1.0f);
  CHECK(ec::donorGetU32(result.response + 68) == 90u);
  check_f("donor field", ec::donorGetF32(result.response + 24), 1.0f);
  const Entry dim{1u, ec::kDonorKindF32, fbits(0.40f)};
  result = apply_vec(&dedup, &observed, build_patch(16u, observed.revision, 0x11u, &dim, 1));
  check_f("donor dim", ec::donorGetF32(result.response + 24), 0.40f);
  CHECK(ec::donorGetU32(result.response + 68) == 90u);

  const Entry emit_off{16u, ec::kDonorKindBool, 0u};
  const std::uint32_t rev_emit = observed.revision;
  result = apply_vec(&dedup, &observed, build_patch(17u, observed.revision, 0x11u, &emit_off, 1));
  CHECK(result.emission_fell == 1u && result.emission_changed == 1u && result.emission == 0u);
  CHECK(observed.revision == rev_emit + 1u);
  CHECK((ec::donorGetU32(result.response + 64) & 32u) == 0u);
  result = apply_vec(&dedup, &observed, build_patch(18u, observed.revision, 0x11u, &emit_off, 1));
  CHECK(result.status == 0u && result.emission_changed == 0u && result.emission_fell == 0u && result.changed == 0u);
  const Entry emit_on{16u, ec::kDonorKindBool, 1u};
  result = apply_vec(&dedup, &observed, build_patch(19u, observed.revision, 0x11u, &emit_on, 1));
  CHECK(result.emission_fell == 0u && result.emission_changed == 1u && result.emission == 1u);
  CHECK((ec::donorGetU32(result.response + 64) & 32u) == 32u);

  const Entry stale_entry{1u, ec::kDonorKindF32, fbits(0.10f)};
  const float brightness_now = ec::donorGetF32(result.response + 24);
  result = apply_vec(&dedup, &observed, build_patch(22u, 0u, 0x11u, &stale_entry, 1));
  CHECK(result.status == ec::kDonorStatusStale);
  k1_emotiscope_get_config(&raw);
  check_f("stale kept", raw.brightness, brightness_now);
  result = apply_vec(&dedup, &observed, build_patch(22u, observed.revision, 0x11u, &stale_entry, 1));
  CHECK(result.status == 0u);
  check_f("stale retry", ec::donorGetF32(result.response + 24), 0.10f);
  result = apply_vec(&dedup, &observed, build_patch(23u, observed.revision, 0x99u, &stale_entry, 1));
  CHECK(result.status == ec::kDonorStatusWrongBoot);
  const Entry warmth{4u, ec::kDonorKindF32, fbits(0.20f)};
  result = apply_vec(&dedup, &observed, build_patch(23u, observed.revision, 0x11u, &warmth, 1));
  CHECK(result.status == 0u);
  check_f("boot retry", ec::donorGetF32(result.response + 36), 0.20f);

  const std::uint32_t rev_scene = observed.revision;
  Entry scene[16];
  const float scene_f[8] = {0.10f,0.20f,0.30f,0.40f,0.50f,0.60f,0.70f,0.80f};
  for (int i = 0; i < 8; ++i) scene[i] = Entry{static_cast<std::uint16_t>(i + 1), ec::kDonorKindF32, fbits(scene_f[i])};
  scene[8] = Entry{9u, ec::kDonorKindI32, 6u};
  scene[9] = Entry{10u, ec::kDonorKindBool, 0u};
  scene[10] = Entry{11u, ec::kDonorKindBool, 0u};
  scene[11] = Entry{12u, ec::kDonorKindBool, 0u};
  scene[12] = Entry{13u, ec::kDonorKindBool, 1u};
  scene[13] = Entry{14u, ec::kDonorKindBool, 1u};
  scene[14] = Entry{15u, ec::kDonorKindU32, 80u};
  scene[15] = Entry{16u, ec::kDonorKindBool, 0u};
  const auto scene_bytes = build_patch(21u, observed.revision, 0x11u, scene, 16);
  CHECK(scene_bytes.size() == 152u);
  result = apply_vec(&dedup, &observed, scene_bytes);
  CHECK(result.status == 0u);
  CHECK(observed.revision == rev_scene + 1u);
  CHECK(result.mode_transition == 1u && result.mode == 6);
  CHECK(result.hardware_brightness == 80u && result.emission == 0u && result.emission_fell == 1u);
  CHECK(ec::donorGetU32(result.response + 72) == 0xFFDFu);
  CHECK(ec::donorGetU32(result.response + 64) == 24u);
  CHECK(ec::donorGetU32(result.response + 76) == 21u);
  k1_emotiscope_get_config(&raw);
  check_f("scene brightness", raw.brightness, 0.10f);
  check_f("scene color", raw.color, 0.30f);
  check_f("scene speed", raw.speed, 0.60f);
  check_f("scene saturation", raw.saturation, 0.70f);
  CHECK(raw.current_mode == 6);
  CHECK(!raw.mirror_mode && !raw.screensaver && !raw.temporal_dithering);
  CHECK(raw.auto_color_cycle && raw.reverse_color_range);
  std::uint8_t get_scene[16];
  build_get(get_scene, 24u, 0x11u);
  result = apply_bytes(&dedup, &observed, get_scene, 16u);
  CHECK(result.changed == 0u && observed.revision == rev_scene + 1u);
  CHECK(ec::donorGetU32(result.response + 76) == 21u);
  check_f("get scene", ec::donorGetF32(result.response + 24), 0.10f);

  const auto no_op = build_patch(25u, observed.revision, 0x11u, &scene[0], 1);
  const std::uint32_t rev_noop = observed.revision;
  result = apply_vec(&dedup, &observed, no_op);
  CHECK(result.status == 0u && result.changed == 0u && observed.revision == rev_noop);
  result = apply_vec(&dedup, &observed, no_op);
  CHECK(ec::donorGetU32(result.response + 96) == 1u);
  const auto blank = empty_patch(26u, observed.revision, 0x11u);
  result = apply_vec(&dedup, &observed, blank);
  CHECK(result.changed == 0u && observed.revision == rev_noop);
  result = apply_vec(&dedup, &observed, blank);
  CHECK(ec::donorGetU32(result.response + 96) == 1u);

  ec::DonorIntent bad{};
  bad.sub = ec::kDonorCfgPatch;
  bad.request_id = 27u;
  bad.boot_id = 0x11u;
  bad.expected_revision = observed.revision;
  bad.mask = 1u;
  bad.brightness = NAN;
  bad.payload_crc = 0xabcdefu;
  ec::DonorApplyResult rejected_apply{};
  ec::donorApply(&dedup, bad, observed, &rejected_apply);
  CHECK(rejected_apply.status == ec::kDonorStatusInvalid);
  result = apply_vec(&dedup, &observed, build_patch(27u, observed.revision, 0x11u, &warmth, 1));
  CHECK(result.status == 0u);

  // Output precision: field 17, bool, owner-held like emission, reported as flag bit 6.
  {
    ec::DonorIntent intent{};
    const Entry on{17u, 2u, 1u};
    auto packet = build_patch(40u, observed.revision, 0x11u, &on, 1);
    CHECK(ec::donorParse(packet.data(), std::uint32_t(packet.size()), &intent) == 0u);
    CHECK(intent.mask == (1u << 16) && intent.output_precision == 1u);
    const std::uint32_t before_revision = observed.revision;
    ec::DonorApplyResult r{};
    ec::donorApply(&dedup, intent, observed, &r);
    CHECK(r.status == 0u && r.output_precision_changed == 1u && r.output_precision == 1u);
    CHECK(r.revision == before_revision + 1u);
    CHECK((ec::donorGetU32(r.response + 64) & 64u) == 64u);
    CHECK(ec::donorGetU32(r.response + 72) == ec::donorGetU32(r.response + 72) &&
          (ec::donorGetU32(r.response + 72) >> 16) == 0u);
    adopt(&observed, intent, r);
    observed.output_precision = r.output_precision;
    // Same value again: accepted, no change, no revision bump.
    auto again = build_patch(41u, observed.revision, 0x11u, &on, 1);
    CHECK(ec::donorParse(again.data(), std::uint32_t(again.size()), &intent) == 0u);
    ec::DonorApplyResult same{};
    ec::donorApply(&dedup, intent, observed, &same);
    CHECK(same.status == 0u && same.output_precision_changed == 0u && same.revision_changed == 0u);
    // GET reports the owner's value.
    std::uint8_t get[16];
    build_get(get, 42u, 0x11u);
    CHECK(ec::donorParse(get, 16u, &intent) == 0u);
    ec::DonorApplyResult got{};
    ec::donorApply(&dedup, intent, observed, &got);
    CHECK((ec::donorGetU32(got.response + 64) & 64u) == 64u);
    observed.output_precision = 0u;
    build_get(get, 43u, 0x11u);
    CHECK(ec::donorParse(get, 16u, &intent) == 0u);
    ec::donorApply(&dedup, intent, observed, &got);
    CHECK((ec::donorGetU32(got.response + 64) & 64u) == 0u);
    // Value 2, field 19, and wrong kinds are all rejected.
    const Entry two{17u, 2u, 2u};
    auto bad2 = build_patch(44u, observed.revision, 0x11u, &two, 1);
    CHECK(ec::donorParse(bad2.data(), std::uint32_t(bad2.size()), &intent) == ec::kDonorStatusInvalid);
    const Entry f18{19u, 4u, 1u};
    auto bad18 = build_patch(45u, observed.revision, 0x11u, &f18, 1);
    CHECK(ec::donorParse(bad18.data(), std::uint32_t(bad18.size()), &intent) == ec::kDonorStatusInvalid);
    // Wrong kind for field 17.
    const Entry f17f{17u, 1u, 0x3f000000u};
    auto badk = build_patch(46u, observed.revision, 0x11u, &f17f, 1);
    CHECK(ec::donorParse(badk.data(), std::uint32_t(badk.size()), &intent) == ec::kDonorStatusInvalid);
    // All 20 fields fit in the explicit 184-byte maximum.
    Entry all[20];
    for (int i = 0; i < 20; ++i) {
      const std::uint16_t f = std::uint16_t(i + 1);
      const std::uint32_t kind = ec::donorFieldKind(f);
      all[i] = Entry{f, std::uint16_t(kind), kind == ec::kDonorKindF32 ? 0x3f000000u
                                            : f == 9u ? 1u : f == 15u ? 100u : f == 16u ? 1u : f == 18u ? 7u : 0u};
    }
    auto full = build_patch(47u, observed.revision, 0x11u, all, 20);
    CHECK(full.size() == 184u);
    CHECK(ec::donorParse(full.data(), std::uint32_t(full.size()), &intent) == 0u);
    CHECK(intent.mask == 0xfffffu);
    std::vector<std::uint8_t> oversize(full);
    oversize.push_back(0u);
    CHECK(ec::donorParse(oversize.data(), std::uint32_t(oversize.size()), &intent) == ec::kDonorStatusInvalid);
  }

  // Intensity is independent physical-lane state, not a donor scene control.
  {
    ec::DonorDedup gain_dedup{};
    ec::DonorObserved gain_observed = fresh_observed();
    K1EmotiscopeConfig scene_before{};
    k1_emotiscope_get_config(&scene_before);
    const Entry gain_a{19u, ec::kDonorKindF32, fbits(0.25f)};
    const Entry gain_b{20u, ec::kDonorKindF32, fbits(0.75f)};
    const auto packet_a = build_patch(101u, 0u, 0x11u, &gain_a, 1);
    auto gain = apply_vec(&gain_dedup, &gain_observed, packet_a);
    CHECK(gain.status == 0u && gain.intensity_a_changed && !gain.intensity_b_changed);
    CHECK(gain_observed.revision == 1u);
    check_f("A gain", gain_observed.output_intensity_a, 0.25f);
    check_f("B preserved", gain_observed.output_intensity_b, 1.0f);
    CHECK(ec::donorGetU32(gain.response + 4) == 2u);
    check_f("A response", ec::donorGetF32(gain.response + 144), 0.25f);
    check_f("B response", ec::donorGetF32(gain.response + 148), 1.0f);
    for (unsigned i = 152; i < 160; ++i) CHECK(gain.response[i] == 0u);
    const auto first_gain = gain;
    gain = apply_vec(&gain_dedup, &gain_observed, packet_a);
    CHECK(gain.status == 0u && !gain.intensity_a_changed && !gain.intensity_b_changed);
    CHECK(ec::donorGetU32(gain.response + 96) == 1u && gain_observed.revision == 1u);
    CHECK(std::memcmp(gain.response, first_gain.response, 96) == 0);
    CHECK(std::memcmp(gain.response + 100, first_gain.response + 100, 60) == 0);
    gain = apply_vec(&gain_dedup, &gain_observed, build_patch(102u, 1u, 0x11u, &gain_b, 1));
    CHECK(gain.status == 0u && !gain.intensity_a_changed && gain.intensity_b_changed);
    check_f("A preserved", gain_observed.output_intensity_a, 0.25f);
    check_f("B gain", gain_observed.output_intensity_b, 0.75f);
    CHECK(gain_observed.revision == 2u);
    const Entry zero_a{19u, ec::kDonorKindF32, fbits(0.0f)};
    gain = apply_vec(&gain_dedup, &gain_observed, build_patch(103u, 0u, 0x11u, &zero_a, 1));
    CHECK(gain.status == ec::kDonorStatusStale && gain_observed.revision == 2u);
    check_f("stale A preserved", gain_observed.output_intensity_a, 0.25f);
    check_f("stale B preserved", gain_observed.output_intensity_b, 0.75f);
    gain = apply_vec(&gain_dedup, &gain_observed, build_patch(103u, 2u, 0x11u, &zero_a, 1));
    CHECK(gain.status == 0u && gain_observed.revision == 3u);
    check_f("A zero", gain_observed.output_intensity_a, 0.0f);
    check_f("zero leaves B", gain_observed.output_intensity_b, 0.75f);
    for (std::uint16_t field : {19u, 20u}) {
      for (std::uint32_t raw_bits : {0x7fc00000u, 0x7f800000u, 0xff800000u, 0x3f800001u, 0xbf000000u, 0x80000001u}) {
        const Entry bad_gain{field, ec::kDonorKindF32, raw_bits};
        ec::DonorIntent parsed{};
        const auto bytes = build_patch(104u, 3u, 0x11u, &bad_gain, 1);
        CHECK(ec::donorParse(bytes.data(), std::uint32_t(bytes.size()), &parsed) == ec::kDonorStatusInvalid);
      }
    }
    // Caller-side bypass of parse must still reject atomically before donor changes.
    ec::DonorIntent bad_gain{};
    bad_gain.sub = ec::kDonorCfgPatch; bad_gain.request_id = 105u;
    bad_gain.boot_id = 0x11u; bad_gain.expected_revision = 3u;
    bad_gain.mask = 1u | (1u << 18); bad_gain.brightness = 0.99f;
    bad_gain.output_intensity_a = NAN;
    ec::donorApply(&gain_dedup, bad_gain, gain_observed, &gain);
    CHECK(gain.status == ec::kDonorStatusInvalid && !gain.revision_changed);
    K1EmotiscopeConfig scene_after{};
    k1_emotiscope_get_config(&scene_after);
    CHECK(std::memcmp(&scene_before, &scene_after, sizeof(scene_before)) == 0);
  }

  std::printf("BEHAVIORAL %s %d\n", g_fails ? "FAIL" : "OK", g_fails);
  return g_fails ? 1 : 0;
}

static int hex_nibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

static int parse_hex(const char* text, std::uint8_t* out, int cap) {
  int count = 0;
  while (*text && *text != '\n' && *text != '\r') {
    if (*text == ' ') { ++text; continue; }
    const int hi = hex_nibble(*text++);
    const int lo = *text ? hex_nibble(*text++) : -1;
    if (hi < 0 || lo < 0 || count >= cap) return -1;
    out[count++] = static_cast<std::uint8_t>((hi << 4) | lo);
  }
  return count;
}

static int corpus(const char* path) {
  if (!k1_emotiscope_init(1u)) return 2;
  FILE* file = std::fopen(path, "r");
  if (!file) return 2;
  ec::DonorDedup dedup{};
  ec::DonorObserved observed = fresh_observed();
  char line[2048];
  int index = 0;
  while (std::fgets(line, sizeof(line), file)) {
    if (line[0] == '\n' || line[0] == '#' || line[0] == 0) continue;
    std::uint8_t bytes[192];
    const int size = parse_hex(line, bytes, static_cast<int>(sizeof(bytes)));
    if (size < 0) { std::fclose(file); return 2; }
    ec::DonorIntent intent{};
    const std::uint32_t parsed = ec::donorParse(bytes, static_cast<std::uint32_t>(size), &intent);
    std::printf("{\"i\":%d,\"parse\":%u", index, parsed);
    if (parsed == 0u) {
      ec::DonorApplyResult result{};
      ec::donorApply(&dedup, intent, observed, &result);
      adopt(&observed, intent, result);
      std::printf(",\"apply\":%u,\"body\":\"", result.status);
      for (unsigned i = 0; i < ec::kDonorStateBytes; ++i) std::printf("%02x", result.response[i]);
      std::printf("\"");
    }
    std::printf("}\n");
    std::fflush(stdout);
    ++index;
  }
  std::fclose(file);
  return 0;
}

int main(int argc, char** argv) {
  if (argc == 3 && std::strcmp(argv[1], "--corpus") == 0) return corpus(argv[2]);
  return behavioral();
}

#include "emotiscope_control.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace ec = k1::emotiscope;
#ifdef K1_TITAN_TOP_OUTPUT
static constexpr std::uint32_t kLegacyResponseVersion = 4u;
#else
static constexpr std::uint32_t kLegacyResponseVersion = 2u;
#endif

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

#ifdef K1_TITAN_TOP_OUTPUT
static std::vector<std::uint8_t> extended_patch(std::uint32_t id, std::uint32_t revision,
                                               const Entry* entries, unsigned count) {
  std::uint64_t mask = 0;
  for (unsigned i=0;i<count;++i) mask |= std::uint64_t{1} << (entries[i].field-1);
  std::vector<std::uint8_t> out(32+8*count);
  ec::donorPutU32(out.data(),0x454d0202u);
  ec::donorPutU32(out.data()+4,0x11u);
  ec::donorPutU32(out.data()+8,id);
  ec::donorPutU32(out.data()+12,revision);
  ec::donorPutU32(out.data()+16,std::uint32_t(mask));
  ec::donorPutU32(out.data()+20,std::uint32_t(mask>>32));
  ec::donorPutU32(out.data()+24,count);
  for(unsigned i=0;i<count;++i){auto* p=out.data()+32+8*i;
    ec::donorPutU16(p,entries[i].field);ec::donorPutU16(p+2,entries[i].kind);ec::donorPutU32(p+4,entries[i].raw);}
  return out;
}

static void check_extended_protocol() {
  ec::DonorObserved observed{}; observed.boot_id=0x11; observed.hardware_brightness=100; observed.emission=1;
  ec::DonorDedup cache{}; ec::DonorApplyResult result{};
  std::uint8_t get[20]{};
  ec::donorPutU32(get,0x454d0201);ec::donorPutU32(get+4,1200);ec::donorPutU32(get+8,0x11);ec::donorPutU32(get+16,1);
  ec::DonorIntent intent{};
  CHECK(ec::donorParse(get,20,&intent)==0 && intent.board==1);
  ec::donorApply(&cache,intent,observed,&result);
  CHECK(result.status==0 && result.response_bytes==424);
  CHECK(ec::donorGetU32(result.response+4)==6 && ec::donorGetU32(result.response+164)==255 && ec::donorGetU32(result.response+176)==1);
  for(unsigned i=312;i<320;++i)CHECK(result.response[i]==0);
  const Entry features[]={{24,4,1},{28,2,1},{29,4,2},{30,1,fbits(.5f)},{31,1,fbits(60.f)},
    {32,4,1},{39,4,2},{40,1,fbits(.75f)},{41,1,fbits(20.f)},{42,1,fbits(250.f)},{43,2,1},{44,1,fbits(.5f)}};
  auto packet=extended_patch(1201,0,features,sizeof(features)/sizeof(features[0]));
  CHECK(ec::donorParse(packet.data(),packet.size(),&intent)==0);
  CHECK(intent.ml_enabled==1 && intent.emission==0 && intent.edge_colour_enabled==1 && intent.edge_pair_enabled==0);
  ec::donorApply(&cache,intent,observed,&result);
  CHECK(result.status==0 && result.response_bytes==424 && result.edge_colour_changed && result.musical_changed && result.ml_changed);
  CHECK(!result.emission_changed && result.emission==1 && result.affected_boards==2);
  CHECK(ec::donorGetU32(result.response+192)==1 && ec::donorGetU32(result.response+256)==1);
  const auto first=result;
  ec::donorApply(&cache,intent,observed,&result);
  CHECK(result.response_bytes==424 && ec::donorGetU32(result.response+96)==1 && !result.changed && !result.ml_changed);
  CHECK(std::memcmp(first.response+100,result.response+100,324)==0);
  intent.board=0; // Same CRC forged by a direct caller cannot replay another target.
  ec::donorApply(&cache,intent,observed,&result);
  CHECK(result.status==ec::kDonorStatusConflict && !result.changed);
  K1EmotiscopeConfig primary_before{},secondary_before{},after{};
  k1_emotiscope_get_config(&primary_before);k1_secondary_emotiscope_get_config(&secondary_before);
  const Entry invalid[]={{1,1,fbits(.1f)},{15,4,24},{23,2,1},{24,4,1},{30,1,0x7fc00000}};
  packet=extended_patch(1202,0,invalid,5);
  CHECK(ec::donorParse(packet.data(),packet.size(),&intent)==ec::kDonorStatusInvalid);
  // Exercise the actual direct-apply atomic boundary too, before admission.
  intent={};intent.sub=0x454d0202;intent.boot_id=0x11;intent.request_id=1202;intent.board=1;
  intent.mask=1|(1ull<<14)|(1ull<<22)|(1ull<<23)|(1ull<<29);
  intent.brightness=.1f;intent.hardware_brightness=24;intent.secondary_output_enabled=1;intent.edge_colour_strength=NAN;
  static unsigned admission=0;admission=0;
  ec::donorApply(&cache,intent,observed,&result,+[](){++admission;return true;});
  CHECK(result.status==ec::kDonorStatusInvalid && admission==0 && !result.revision_changed && !result.changed);
  CHECK(result.hardware_brightness==100 && result.emission==1 && observed.revision==0);
  k1_emotiscope_get_config(&after);CHECK(std::memcmp(&after,&primary_before,sizeof(after))==0);
  k1_secondary_emotiscope_get_config(&after);CHECK(std::memcmp(&after,&secondary_before,sizeof(after))==0);
  // Independently written v6 full scene packet; distinct values expose field aliases.
  const Entry composed[]={{24,4,1},{45,4,2},{46,1,fbits(.25f)},{47,1,fbits(1600.f)},
    {48,1,fbits(400.f)},{49,1,fbits(600.f)},{50,1,fbits(.125f)},{51,1,fbits(2400.f)},
    {52,1,fbits(.5f)},{53,1,fbits(200.f)},{54,4,2},{55,1,fbits(.375f)},
    {56,1,fbits(.625f)},{57,1,fbits(700.f)},{58,4,1},{59,4,1},{60,1,fbits(300.f)},
    {61,4,2},{62,4,3},{63,4,2}};
  packet=extended_patch(1300,0,composed,20);
  std::printf("COMPOSED_PACKET ");for(auto byte:packet)std::printf("%02x",byte);std::puts("");
  CHECK(ec::donorParse(packet.data(),packet.size(),&intent)==0);
  ec::donorApply(&cache,intent,observed,&result);
  CHECK(result.status==0 && result.composed_changed && result.affected_boards==3);
  CHECK(result.composed.scene==2 && result.composed.trips==2 && result.composed.ml_target==3);
  CHECK(result.composed.outgoing_ms==1600.f && result.composed.return_ms==2400.f);
  CHECK(ec::donorGetU32(result.response+320)==2 && ec::donorGetU32(result.response+396)==0);
  CHECK(ec::donorGetU32(result.response+404)==1);
  std::printf("COMPOSED_STATE ");for(unsigned i=0;i<424;++i)std::printf("%02x",result.response[i]);std::puts("");
  const auto scene_first=result;
  ec::donorApply(&cache,intent,observed,&result);
  CHECK(result.status==0 && !result.composed_changed && ec::donorGetU32(result.response+96)==1);
  CHECK(std::memcmp(scene_first.response+100,result.response+100,324)==0);
  intent.payload_crc++;ec::donorApply(&cache,intent,observed,&result);
  CHECK(result.status==ec::kDonorStatusConflict && !result.composed_changed);
  // Partial updates merge with the current request and preserve untouched fields.
  observed.composed=scene_first.composed;observed.revision=scene_first.revision;
  const Entry partial[]={{24,4,1},{47,1,fbits(1800.f)}};
  packet=extended_patch(1301,observed.revision,partial,2);
  CHECK(ec::donorParse(packet.data(),packet.size(),&intent)==0);
  ec::donorApply(&cache,intent,observed,&result);
  CHECK(result.status==0 && result.composed.outgoing_ms==1800.f && result.composed.return_ms==2400.f);
  intent.request_id=1302;intent.expected_revision=0;
  ec::donorApply(&cache,intent,observed,&result);
  CHECK(result.status==ec::kDonorStatusStale && !result.composed_changed);
  // A poisoned untouched member rejects a merged partial patch before any legacy mutation.
  intent.expected_revision=observed.revision;intent.mask|=1;intent.brightness=.1f;
  observed.composed.return_ms=0;
  ec::donorApply(&cache,intent,observed,&result);
  CHECK(result.status==ec::kDonorStatusInvalid && !result.changed && !result.revision_changed);
  k1_secondary_emotiscope_get_config(&after);CHECK(std::memcmp(&after,&secondary_before,sizeof(after))==0);
  observed.composed=k1_composed_scene_defaults();observed.revision=0;
  for(const auto bad: {Entry{45,4,4},Entry{46,1,fbits(.01f)},Entry{47,1,0x7fc00000},
      Entry{54,4,3},Entry{58,4,3},Entry{59,4,2},Entry{61,4,5},Entry{63,4,4}}){
    packet=extended_patch(1310,0,&bad,1);CHECK(ec::donorParse(packet.data(),packet.size(),&intent)==3);
  }
  packet=extended_patch(1311,0,composed,20);
  ec::donorPutU16(packet.data()+32+8,24); // duplicate/out-of-order field
  CHECK(ec::donorParse(packet.data(),packet.size(),&intent)==3);
  packet=extended_patch(1312,0,partial,2);packet.pop_back();
  CHECK(ec::donorParse(packet.data(),packet.size(),&intent)==3);
  // Exercise the same owning setter used by fixture_app, then read both consumers.
  K1ComposedSceneConfig primary_scene{}, secondary_scene{};
  k1_emotiscope_get_scene_config(&primary_scene);
  CHECK(k1_secondary_emotiscope_set_scene_config(&scene_first.composed));
  k1_secondary_emotiscope_get_scene_config(&secondary_scene);
  CHECK(secondary_scene.scene==2 && secondary_scene.outgoing_ms==1600.f);
  K1ComposedSceneConfig primary_read{};k1_emotiscope_get_scene_config(&primary_read);
  CHECK(primary_read.scene==primary_scene.scene && primary_read.outgoing_ms==primary_scene.outgoing_ms);
  CHECK(primary_read.choreography==1 && primary_read.initiator==1 && primary_read.answer_delay_ms==300.f);
  auto invalid_scene=secondary_scene;invalid_scene.return_ms=0;
  CHECK(!k1_secondary_emotiscope_set_scene_config(&invalid_scene));
  k1_secondary_emotiscope_get_scene_config(&invalid_scene);
  CHECK(invalid_scene.return_ms==2400.f);
  const auto defaults=k1_composed_scene_defaults();
  CHECK(k1_secondary_emotiscope_set_scene_config(&defaults));
  CHECK(k1_emotiscope_set_scene_config(&defaults));
  // Legacy negotiation remains v4 even on the new image.
  ec::donorPutU32(get,0x454d0101);ec::donorPutU32(get+4,1203);
  CHECK(ec::donorParse(get,20,&intent)==0);ec::donorApply(&cache,intent,observed,&result);
  CHECK(result.response_bytes==208 && ec::donorGetU32(result.response+4)==4 && ec::donorGetU32(result.response+164)==7);
}
#endif

static void build_get(std::uint8_t out[16], std::uint32_t id, std::uint32_t boot) {
  std::memset(out, 0, 16);
  ec::donorPutU32(out + 0, ec::kDonorCfgGetState);
  ec::donorPutU32(out + 4, id);
  ec::donorPutU32(out + 8, boot);
}

#ifdef K1_TITAN_TOP_OUTPUT
// Fixture-owned retained source settings, mirrored here for wire corpus replay.
struct RetainedInhouse {float mood=.05f,sensitivity=2.4f,contrast=1.f,photons=1.f;};
static RetainedInhouse retained_inhouse[2];
#endif

static void adopt(ec::DonorObserved* observed, const ec::DonorIntent& intent, const ec::DonorApplyResult& result) {
  if (result.revision_changed) {
    observed->revision = result.revision;
    observed->accepted_request_id = intent.request_id;
  }
  if (result.brightness_changed) observed->hardware_brightness = result.hardware_brightness;
  if (result.emission_changed) observed->emission = result.emission;
  if (result.output_precision_changed) observed->output_precision = result.output_precision;
  if (result.palette_changed) {
    observed->palette = result.palette;
#ifdef K1_TITAN_TOP_OUTPUT
    CHECK(result.board == 1u ? k1_secondary_emotiscope_set_palette(result.palette)
                            : k1_emotiscope_set_palette(result.palette));
#else
    CHECK(k1_emotiscope_set_palette(result.palette));
#endif
  }
  if (result.intensity_a_changed) observed->output_intensity_a = result.output_intensity_a;
  if (result.intensity_b_changed) observed->output_intensity_b = result.output_intensity_b;
#ifdef K1_TITAN_TOP_OUTPUT
  auto& retained=retained_inhouse[result.board];
  if(result.inhouse_mood_changed)retained.mood=result.inhouse_mood;
  if(result.inhouse_sensitivity_changed)retained.sensitivity=result.inhouse_sensitivity;
  if(result.inhouse_square_iter_changed)retained.contrast=result.inhouse_square_iter;
  if(result.inhouse_injection_photons_changed)retained.photons=result.inhouse_injection_photons;
  if (result.primary_intensity_changed) observed->primary_intensity = result.primary_intensity;
  if (result.secondary_intensity_changed) observed->secondary_intensity = result.secondary_intensity;
  if (result.secondary_output_enabled_changed) observed->secondary_output_enabled = result.secondary_output_enabled;
  if (result.response_gain_changed)
    CHECK(result.board == 1u ? k1_secondary_emotiscope_set_response_gain(result.response_gain)
                            : k1_emotiscope_set_response_gain(result.response_gain));
  if (result.edge_response_changed || result.edge_depth_changed)
    CHECK(result.board == 1u ? k1_secondary_emotiscope_set_edge_response(result.edge_response, result.edge_depth)
                            : k1_emotiscope_set_edge_response(result.edge_response, result.edge_depth));
#endif
}

static void observe_scene(ec::DonorObserved* observed, const ec::DonorIntent& intent) {
#ifdef K1_TITAN_TOP_OUTPUT
  observed->board = intent.board;
  const auto& retained=retained_inhouse[intent.board];
  observed->inhouse_mood=retained.mood;observed->inhouse_sensitivity=retained.sensitivity;
  observed->inhouse_square_iter=retained.contrast;observed->inhouse_injection_photons=retained.photons;
  const bool secondary = intent.board == 1u;
  observed->palette = secondary ? k1_secondary_emotiscope_palette() : k1_emotiscope_palette();
  observed->response_gain = secondary ? k1_secondary_emotiscope_response_gain() : k1_emotiscope_response_gain();
  observed->edge_response = secondary ? k1_secondary_emotiscope_edge_response() : k1_emotiscope_edge_response();
  observed->edge_depth = secondary ? k1_secondary_emotiscope_edge_depth() : k1_emotiscope_edge_depth();
#else
  (void)intent;
  observed->palette = k1_emotiscope_palette();
#endif
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
  observe_scene(observed, intent);
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
  CHECK(ec::donorGetU32(body + 4) == kLegacyResponseVersion);
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
  CHECK(ec::donorGetU32(body + 72) == 0xFF8Bu);
  CHECK(ec::donorGetU32(body + 92) == 0u);
  CHECK(ec::donorGetU32(body + 96) == 0u);
  CHECK(ec::donorGetU32(body + 100) == 0u);
  CHECK(ec::donorGetU32(body + 104) == 0u);
  CHECK(ec::donorGetU32(body + 140) == 1u);
  check_f("intensity A", ec::donorGetF32(body + 144), 1.0f);
  check_f("intensity B", ec::donorGetF32(body + 148), 1.0f);
#ifdef K1_TITAN_TOP_OUTPUT
  check_f("primary", ec::donorGetF32(body + 152), 1.0f);
  check_f("secondary", ec::donorGetF32(body + 156), 1.0f);
  CHECK(ec::donorGetU32(body + 160) == 0u && ec::donorGetU32(body + 164) == 7u);
  for (unsigned i = 168; i < 176; ++i) CHECK(body[i] == 0u);
  CHECK(ec::donorGetU32(body + 176) == 0u);
  check_f("response gain", ec::donorGetF32(body + 180), 1.0f);
  CHECK(ec::donorGetU32(body + 184) == 0u);
  check_f("edge depth", ec::donorGetF32(body + 188), 0.0f);
  for (unsigned i = 192; i < 208; ++i) CHECK(body[i] == 0u);
#else
  for (unsigned i = 152; i < 160; ++i) CHECK(body[i] == 0u);
#endif
}

#ifdef K1_TITAN_TOP_OUTPUT
static void check_independent_scene_controls() {
  K1EmotiscopeConfig defaults{}, primary_before{}, secondary_after{};
  k1_emotiscope_default_config(&defaults);
  CHECK(k1_emotiscope_set_config(&defaults));
  CHECK(k1_secondary_emotiscope_set_config(&defaults));
  CHECK(k1_emotiscope_set_palette(1u) && k1_secondary_emotiscope_set_palette(1u));
  CHECK(k1_emotiscope_set_response_gain(1.0f) && k1_secondary_emotiscope_set_response_gain(1.0f));
  CHECK(k1_emotiscope_set_edge_response(0u, 0.0f) && k1_secondary_emotiscope_set_edge_response(0u, 0.0f));
  k1_emotiscope_get_config(&primary_before);
  ec::DonorObserved observed = fresh_observed();
  ec::DonorDedup cache{};
  // Exercise every mirrored scene parameter through the actual secondary consumer.
  const Entry scene[]{
    {1u,1u,fbits(0.21f)}, {2u,1u,fbits(0.32f)},
    {4u,1u,fbits(0.54f)}, {6u,1u,fbits(0.76f)},
    {8u,1u,fbits(0.18f)}, {9u,3u,6u},
    {10u,2u,0u}, {11u,2u,0u}, {12u,2u,0u}, {13u,2u,1u}, {14u,2u,1u},
    {18u,4u,31u}, {24u,4u,1u}, {25u,1u,fbits(1.75f)}, {26u,4u,2u}, {27u,1u,fbits(0.25f)}
  };
  const auto packet = build_patch(1001u, 0u, 0x11u, scene, int(sizeof(scene)/sizeof(scene[0])));
  auto result = apply_vec(&cache, &observed, packet);
  CHECK(result.status == 0u && observed.revision == 1u);
  K1EmotiscopeConfig primary_after{};
  k1_emotiscope_get_config(&primary_after);
  k1_secondary_emotiscope_get_config(&secondary_after);
  CHECK(std::memcmp(&primary_before, &primary_after, sizeof(primary_before)) == 0);
  const float values[]{secondary_after.brightness, secondary_after.softness, secondary_after.color,
    secondary_after.warmth, secondary_after.color_range, secondary_after.speed,
    secondary_after.saturation, secondary_after.background};
  const float expected[]{0.21f,0.32f,0.33f,0.54f,0.0f,0.76f,0.75f,0.18f};
  for (unsigned i=0;i<8u;++i) {
    check_f("secondary actual config", values[i], expected[i]);
    check_f("secondary selected readback", ec::donorGetF32(result.response+24u+4u*i), expected[i]);
  }
  CHECK(secondary_after.current_mode == 6 && !secondary_after.mirror_mode && !secondary_after.screensaver
        && !secondary_after.temporal_dithering && secondary_after.auto_color_cycle && secondary_after.reverse_color_range);
  CHECK(k1_emotiscope_palette() == 1u && k1_secondary_emotiscope_palette() == 31u);
  CHECK(k1_emotiscope_response_gain() == 1.0f && k1_secondary_emotiscope_response_gain() == 1.75f);
  CHECK(k1_emotiscope_edge_response() == 0u && k1_secondary_emotiscope_edge_response() == 2u);
  CHECK(k1_secondary_emotiscope_edge_depth() == 0.25f);
  CHECK(ec::donorGetU32(result.response+176) == 1u && ec::donorGetU32(result.response+140) == 31u);
  CHECK(ec::donorGetF32(result.response+180) == 1.75f && ec::donorGetU32(result.response+184) == 2u
        && ec::donorGetF32(result.response+188) == 0.25f);
  auto replay = apply_vec(&cache, &observed, packet);
  CHECK(replay.status == 0u && ec::donorGetU32(replay.response+96) == 1u && !replay.revision_changed);
  // Even a direct caller forging the original CRC cannot reuse the ID for another target.
  ec::DonorIntent forged{};
  CHECK(ec::donorParse(packet.data(), packet.size(), &forged) == 0u);
  forged.board = 0u;
  ec::donorApply(&cache, forged, observed, &result);
  CHECK(result.status == ec::kDonorStatusConflict && !result.revision_changed);
  const Entry primary[]{ {1u,1u,fbits(0.4f)}, {18u,4u,25u}, {25u,1u,fbits(0.5f)} };
  result = apply_vec(&cache, &observed, build_patch(1002u, 1u, 0x11u, primary, 3));
  CHECK(result.status == 0u && observed.revision == 2u);
  K1EmotiscopeConfig secondary_kept{};
  k1_secondary_emotiscope_get_config(&secondary_kept);
  CHECK(std::memcmp(&secondary_after, &secondary_kept, sizeof(secondary_after)) == 0);
  CHECK(k1_secondary_emotiscope_palette() == 31u && k1_secondary_emotiscope_response_gain() == 1.75f);
  CHECK(k1_emotiscope_palette() == 25u && k1_emotiscope_response_gain() == 0.5f);
  CHECK(ec::donorGetU32(result.response+176) == 0u);
  const Entry selector{24u,4u,1u};
  result = apply_vec(&cache, &observed, build_patch(1003u, 2u, 0x11u, &selector, 1));
  CHECK(result.status == 0u && !result.changed && observed.revision == 2u);
  std::uint8_t get[20]{};
  build_get(get, 1004u, 0x11u); ec::donorPutU32(get+16,1u);
  result = apply_bytes(&cache, &observed, get, 20u);
  CHECK(result.status == 0u && ec::donorGetU32(result.response+176) == 1u);
  check_f("selected secondary GET", ec::donorGetF32(result.response+24), 0.21f);
  build_get(get, 1005u, 0x11u);
  result = apply_bytes(&cache, &observed, get, 16u);
  CHECK(result.status == 0u && ec::donorGetU32(result.response+176) == 0u);
  check_f("legacy GET stays primary", ec::donorGetF32(result.response+24), 0.4f);
  // A malformed response or prohibited DIN-half target cannot partially alter either scene.
  for (std::uint32_t bits : {0x7fc00000u,0x7f800000u,0x40000001u,0x80000001u}) {
    const Entry invalid[]{ {1u,1u,fbits(0.99f)}, {24u,4u,1u}, {25u,1u,bits} };
    result = apply_vec(&cache, &observed, build_patch(1006u, 2u, 0x11u, invalid, 3));
    CHECK(result.status == 1000u+ec::kDonorStatusInvalid && observed.revision == 2u);
  }
  const Entry bad_din[]{ {19u,1u,fbits(0.1f)}, {24u,4u,1u} };
  result = apply_vec(&cache,&observed,build_patch(1007u,2u,0x11u,bad_din,2));
  CHECK(result.status == 1000u+ec::kDonorStatusInvalid);
  const Entry palette_zero[]{ {1u,1u,fbits(0.99f)}, {18u,4u,0u}, {24u,4u,1u} };
  result=apply_vec(&cache,&observed,build_patch(1008u,2u,0x11u,palette_zero,3));
  CHECK(result.status == 1000u+ec::kDonorStatusInvalid && k1_secondary_emotiscope_palette()==31u);
  ec::DonorIntent invalid{}; invalid.sub=ec::kDonorCfgPatch; invalid.request_id=1009u;
  invalid.boot_id=0x11u; invalid.expected_revision=2u; invalid.board=1u;
  invalid.mask=1u|(1u<<23)|(1u<<24); invalid.brightness=0.99f; invalid.response_gain=NAN;
  observe_scene(&observed,invalid); ec::donorApply(&cache,invalid,observed,&result);
  CHECK(result.status==ec::kDonorStatusInvalid && !result.revision_changed);
  k1_secondary_emotiscope_get_config(&secondary_kept);
  CHECK(std::memcmp(&secondary_after,&secondary_kept,sizeof(secondary_after))==0);
  observed.board_capabilities=3u;
  build_get(get,1010u,0x11u);ec::donorPutU32(get+16,1u);
  result=apply_bytes(&cache,&observed,get,20u);
  CHECK(result.status==ec::kDonorStatusInvalid && observed.revision==2u);
  observed.board_capabilities=7u;
  // Direct mixed packets also reject before secondary admission on both targets.
  static unsigned obsolete_admissions=0;
  for(unsigned target=0;target<2u;++target) for(unsigned field:{3u,5u,7u}) {
    ec::DonorIntent mixed{};mixed.sub=ec::kDonorCfgPatch;mixed.request_id=1020u;
    mixed.boot_id=0x11u;mixed.expected_revision=2u;mixed.board=target;
    mixed.mask=1u|(1u<<(field-1u))|(1u<<22)|(1u<<23);
    mixed.brightness=0.99f;mixed.color=0.2f;mixed.color_range=0.2f;mixed.saturation=0.2f;
    mixed.secondary_output_enabled=1u;observe_scene(&observed,mixed);
    obsolete_admissions=0;
    ec::donorApply(&cache,mixed,observed,&result,+[](){++obsolete_admissions;return true;});
    CHECK(result.status==ec::kDonorStatusInvalid && !result.revision_changed && obsolete_admissions==0u);
  }
  ec::DonorIntent fault_enable{};fault_enable.sub=ec::kDonorCfgPatch;fault_enable.request_id=1021u;
  fault_enable.boot_id=0x11u;fault_enable.expected_revision=2u;fault_enable.board=1u;
  fault_enable.mask=(1u<<22)|(1u<<23);fault_enable.secondary_output_enabled=1u;
  observe_scene(&observed,fault_enable);
  ec::donorApply(&cache,fault_enable,observed,&result,+[](){return false;});
  CHECK(result.status==ec::kDonorStatusInvalid && !result.revision_changed);
  CHECK(ec::donorGetU32(result.response+176)==1u && ec::donorGetU32(result.response+164)==5u);
  CHECK(ec::donorGetU32(result.response+160)==0u);
  k1_emotiscope_get_config(&primary_after);k1_secondary_emotiscope_get_config(&secondary_kept);
  check_f("atomic primary brightness",primary_after.brightness,0.4f);
  CHECK(std::memcmp(&secondary_after,&secondary_kept,sizeof(secondary_after))==0);
  Entry full[27];
  for(unsigned i=0;i<27u;++i){const auto field=std::uint16_t(i+1);const auto kind=ec::donorFieldKind(field);
    full[i]={field,std::uint16_t(kind),kind==1u?fbits(0.5f):field==18u?1u:0u};}
  ec::DonorIntent parsed{};const auto maximum=build_patch(1011u,2u,0x11u,full,27);
  CHECK(maximum.size()==240u && ec::donorParse(maximum.data(),maximum.size(),&parsed)==ec::kDonorStatusInvalid);
  for(unsigned target=0;target<2u;++target) for(std::uint16_t field:{3u,5u,7u}) {
    const Entry mixed[]{{1u,1u,fbits(0.99f)},{field,1u,fbits(0.2f)},{24u,4u,target}};
    result=apply_vec(&cache,&observed,build_patch(1012u,2u,0x11u,mixed,3));
    CHECK(result.status==1000u+ec::kDonorStatusInvalid && observed.revision==2u);
  }
  // Receipt routing follows actual changed consumers, rather than selector presence.
  const Entry top_gain_with_primary_noop[]{{1u,1u,fbits(0.4f)},{22u,1u,fbits(0.5f)}};
  result=apply_vec(&cache,&observed,build_patch(1030u,2u,0x11u,top_gain_with_primary_noop,2));
  CHECK(result.status==0u && result.affected_boards==2u && observed.revision==3u);
  const Entry global_brightness{15u,4u,99u};
  result=apply_vec(&cache,&observed,build_patch(1031u,3u,0x11u,&global_brightness,1));
  CHECK(result.status==0u && result.affected_boards==3u && observed.revision==4u);
  const Entry secondary_scene[]{{2u,1u,fbits(0.45f)},{24u,4u,1u}};
  result=apply_vec(&cache,&observed,build_patch(1032u,4u,0x11u,secondary_scene,2));
  CHECK(result.status==0u && result.affected_boards==2u && observed.revision==5u);
  const auto selector_packet=build_patch(1033u,5u,0x11u,&selector,1);
  result=apply_vec(&cache,&observed,selector_packet);
  CHECK(result.status==0u && result.affected_boards==0u && observed.revision==5u);
  result=apply_vec(&cache,&observed,selector_packet);
  CHECK(result.status==0u && result.affected_boards==0u && !result.revision_changed
        && ec::donorGetU32(result.response+96)==1u);
}
#endif

static void check_reverse_applicability() {
  // These local snapshots cover historical positive ranges without writing the
  // obsolete setting. Palette reversal must remain supported at range zero too.
  for (std::int32_t mode=0;mode<7;++mode) for(float range:{0.0f,0.5f}) {
    K1EmotiscopeConfig config{};
    k1_emotiscope_default_config(&config);
    config.current_mode=mode;config.color_range=range;
    const auto expected=mode==0?0xffabu:0xff8bu;
    CHECK(ec::donorApplicability(config)==expected);
    CHECK((expected&(1u<<13))!=0u && (expected&0x54u)==0u);
    ec::DonorObserved observed=fresh_observed();
    std::uint8_t body[ec::kDonorStateBytes]{};
    ec::donorEncodeState(body,config,observed,77u,0u,0u,0u,0u);
    CHECK(ec::donorGetU32(body+4)==ec::kDonorStateVersion);
    CHECK(ec::donorGetU32(body+72)==expected);
    check_f("historical range readback",ec::donorGetF32(body+40),range);
  }
}

static int behavioral() {
  CHECK(k1_emotiscope_init(1u));
#ifdef K1_TITAN_TOP_OUTPUT
  CHECK(k1_secondary_emotiscope_init(2u));
#endif
  CHECK(!k1_emotiscope_init(2u));
  check_reverse_applicability();
  const char* names[] = {"Analog", "Spectrum", "Octave", "Metronome", "Spectronome", "Hype", "Bloom"};
  for (std::uint32_t mode = 0; mode < 7u; ++mode)
    CHECK(std::strcmp(k1_emotiscope_mode_name(mode), names[mode]) == 0);
#ifdef K1_TITAN_TOP_OUTPUT
  CHECK(std::strcmp(k1_emotiscope_mode_name(7u), "Waveform Fast") == 0);
  CHECK(std::strcmp(k1_emotiscope_mode_name(8u), "Waveform") == 0);
#else
  CHECK(std::strcmp(k1_emotiscope_mode_name(7u), "Neutral") == 0);
  CHECK(std::strcmp(k1_emotiscope_mode_name(8u), "Self Test") == 0);
#endif

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
  rejected.current_mode = 9;
  CHECK(!k1_emotiscope_patch(&rejected, &changed, &transition));
  rejected.current_mode = 10;
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
  print_hex("DEFAULT", result.response, result.response_bytes);
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
  CHECK(ec::donorGetU32(result.response + 72) == 0xFFABu);
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
  print_hex("RAWGET", result.response, result.response_bytes);
  // Historical raw readback survives an unrelated masked patch; obsolete HSV writes reject.
  for (std::uint16_t field : {3u,5u,7u}) {
    const Entry mixed[]{ {1u,1u,fbits(0.99f)}, {field,1u,fbits(0.2f)} };
    const auto rev = observed.revision;
    result=apply_vec(&dedup,&observed,build_patch(8u,rev,0x11u,mixed,2));
    CHECK(result.status==1000u+ec::kDonorStatusInvalid && observed.revision==rev);
    k1_emotiscope_get_config(&raw);
    check_f("obsolete mixed write preserved brightness",raw.brightness,1.0f);
    check_f("obsolete mixed write preserved raw colour",raw.color,1.4f);
  }
  raw.color=0.2f;
  CHECK(!k1_emotiscope_set_config(&raw));
  k1_emotiscope_get_config(&raw);
  check_f("set_config retains historical colour",raw.color,1.4f);
  k1_emotiscope_test_set_raw_color(0.33f);
  const Entry bloom{9u,ec::kDonorKindI32,6u};
  result=apply_vec(&dedup,&observed,build_patch(10u,observed.revision,0x11u,&bloom,1));
  CHECK(result.status==0u && ec::donorGetU32(result.response+72)==0xFF8Bu);
  result=apply_vec(&dedup,&observed,build_patch(12u,observed.revision,0x11u,&analog,1));
  CHECK(result.status==0u && ec::donorGetU32(result.response+72)==0xFFABu);

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
  std::vector<Entry> applicable_scene;
  for(const auto& entry:scene) if(entry.field!=3u && entry.field!=5u && entry.field!=7u) applicable_scene.push_back(entry);
  const auto scene_bytes = build_patch(21u, observed.revision, 0x11u, applicable_scene.data(), applicable_scene.size());
  CHECK(scene_bytes.size() == 128u);
  result = apply_vec(&dedup, &observed, scene_bytes);
  CHECK(result.status == 0u);
  CHECK(observed.revision == rev_scene + 1u);
  CHECK(result.mode_transition == 1u && result.mode == 6);
  CHECK(result.hardware_brightness == 80u && result.emission == 0u && result.emission_fell == 1u);
  CHECK(ec::donorGetU32(result.response + 72) == 0xFF8Bu);
  CHECK(ec::donorGetU32(result.response + 64) == 24u);
  CHECK(ec::donorGetU32(result.response + 76) == 21u);
  k1_emotiscope_get_config(&raw);
  check_f("scene brightness", raw.brightness, 0.10f);
  check_f("scene color", raw.color, 0.33f);
  check_f("scene speed", raw.speed, 0.60f);
  check_f("scene saturation", raw.saturation, 0.75f);
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
    CHECK(ec::donorParse(full.data(), std::uint32_t(full.size()), &intent) == ec::kDonorStatusInvalid);
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
    CHECK(ec::donorGetU32(gain.response + 4) == kLegacyResponseVersion);
    check_f("A response", ec::donorGetF32(gain.response + 144), 0.25f);
    check_f("B response", ec::donorGetF32(gain.response + 148), 1.0f);
    for (unsigned i = ec::kDonorStateBytes - 8; i < ec::kDonorStateBytes; ++i) CHECK(gain.response[i] == 0u);
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

#ifdef K1_TITAN_TOP_OUTPUT
  {
    ec::DonorObserved board = fresh_observed();
    ec::DonorDedup cache{};
    const Entry primary{21u, 1u, 0x3e800000u};
    auto response = apply_vec(&cache, &board, build_patch(900u, 0u, 0x11u, &primary, 1));
    CHECK(response.status == 0u && response.primary_intensity_changed);
    CHECK(board.primary_intensity == 0.25f && board.secondary_intensity == 1.0f);
    const Entry secondary{22u, 1u, 0x3f400000u};
    response = apply_vec(&cache, &board, build_patch(901u, board.revision, 0x11u, &secondary, 1));
    CHECK(response.status == 0u && response.secondary_intensity_changed);
    CHECK(board.primary_intensity == 0.25f && board.secondary_intensity == 0.75f);
    CHECK(board.output_intensity_a == 1.0f && board.output_intensity_b == 1.0f);
    auto replay = apply_vec(&cache, &board, build_patch(901u, board.revision - 1u, 0x11u, &secondary, 1));
    CHECK(ec::donorGetU32(replay.response + 96) == 1u && !replay.secondary_intensity_changed);
    K1EmotiscopeConfig before_board{}, after_board{};
    k1_emotiscope_get_config(&before_board);
    board.board_capabilities = 1u;
    const Entry unsupported[]{ {1u, 1u, 0x3f7d70a4u}, {23u, 2u, 1u} };
    const auto revision = board.revision;
    response = apply_vec(&cache, &board, build_patch(902u, revision, 0x11u, unsupported, 2));
    k1_emotiscope_get_config(&after_board);
    CHECK(response.status == ec::kDonorStatusInvalid && board.revision == revision);
    CHECK(std::memcmp(&before_board, &after_board, sizeof(before_board)) == 0);
    CHECK(!board.secondary_output_enabled);
    board.board_capabilities = 7u;
    const auto failed_enable = build_patch(905u, board.revision, 0x11u, unsupported, 2);
    ec::DonorIntent failed_intent{};
    CHECK(ec::donorParse(failed_enable.data(), failed_enable.size(), &failed_intent) == 0u);
    ec::donorApply(&cache, failed_intent, board, &response, +[]() { return false; });
    k1_emotiscope_get_config(&after_board);
    CHECK(response.status == ec::kDonorStatusInvalid && response.revision == revision);
    CHECK(ec::donorGetU32(response.response + 164) == 5u && !response.secondary_output_enabled_changed);
    CHECK(std::memcmp(&before_board, &after_board, sizeof(before_board)) == 0);
    // Direct intents must reject invalid scene data before hardware admission.
    ec::DonorIntent invalid_legacy = failed_intent;
    invalid_legacy.request_id = 906u;
    const std::uint32_t nan_bits = 0x7fc00000u;
    std::memcpy(&invalid_legacy.brightness, &nan_bits, sizeof(nan_bits));
    static unsigned admission_calls = 0;
    admission_calls = 0;
    ec::donorApply(&cache, invalid_legacy, board, &response,
                  +[]() { ++admission_calls; return true; });
    k1_emotiscope_get_config(&after_board);
    CHECK(admission_calls == 0u);
    CHECK(response.status == ec::kDonorStatusInvalid && response.revision == revision);
    CHECK(!response.secondary_output_enabled_changed && response.secondary_output_enabled == 0u);
    CHECK(std::memcmp(&before_board, &after_board, sizeof(before_board)) == 0);
    for (const std::uint32_t raw : {0x7fc00000u, 0x7f800000u, 0xbf000000u, 0x3f800001u, 0x80000001u}) {
      for (const std::uint16_t field : {std::uint16_t(21), std::uint16_t(22)}) {
        const Entry invalid{field, 1u, raw};
        ec::DonorIntent intent{};
        const auto bytes = build_patch(903u, board.revision, 0x11u, &invalid, 1);
        CHECK(ec::donorParse(bytes.data(), bytes.size(), &intent) == ec::kDonorStatusInvalid);
      }
    }
    Entry full[23];
    for (unsigned i = 0; i < 23; ++i) {
      const auto field = std::uint16_t(i + 1);
      const auto kind = ec::donorFieldKind(field);
      full[i] = Entry{field, std::uint16_t(kind), kind == ec::kDonorKindF32 ? 0x3f000000u
                        : field == 9u ? 1u : field == 15u ? 100u : field == 18u ? 1u : 0u};
    }
    const auto maximum = build_patch(904u, board.revision, 0x11u, full, 23);
    ec::DonorIntent full_intent{};
    CHECK(maximum.size() == 208u);
    CHECK(ec::donorParse(maximum.data(), maximum.size(), &full_intent) == ec::kDonorStatusInvalid);
  }
#endif
#ifdef K1_TITAN_TOP_OUTPUT
  check_independent_scene_controls();
  check_extended_protocol();
#endif
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
#ifdef K1_TITAN_TOP_OUTPUT
  if (!k1_secondary_emotiscope_init(2u)) return 2;
#endif
  FILE* file = std::fopen(path, "r");
  if (!file) return 2;
  ec::DonorDedup dedup{};
  ec::DonorObserved observed = fresh_observed();
  char line[2048];
  int index = 0;
  while (std::fgets(line, sizeof(line), file)) {
    if (line[0] == '\n' || line[0] == '#' || line[0] == 0) continue;
    std::uint8_t bytes[568];
    const int size = parse_hex(line, bytes, static_cast<int>(sizeof(bytes)));
    if (size < 0) { std::fclose(file); return 2; }
    ec::DonorIntent intent{};
    const std::uint32_t parsed = ec::donorParse(bytes, static_cast<std::uint32_t>(size), &intent);
    std::printf("{\"i\":%d,\"parse\":%u", index, parsed);
    if (parsed == 0u) {
      ec::DonorApplyResult result{};
      observe_scene(&observed, intent);
      ec::donorApply(&dedup, intent, observed, &result);
      adopt(&observed, intent, result);
      std::printf(",\"apply\":%u,\"body\":\"", result.status);
      for (unsigned i = 0; i < result.response_bytes; ++i) std::printf("%02x", result.response[i]);
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

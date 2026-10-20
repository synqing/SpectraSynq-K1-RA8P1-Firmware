#include "k1_live_protocol.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

static std::vector<std::uint8_t> from_hex(const char* hex) {
  std::string s(hex);
  if (s.size() % 2) {
    std::exit(2);
  }
  std::vector<std::uint8_t> out(s.size() / 2);
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = static_cast<std::uint8_t>(std::strtoul(s.substr(i * 2, 2).c_str(), nullptr, 16));
  }
  return out;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    return 2;
  }
  const auto bytes = from_hex(argv[1]);
  if (bytes.size() < 4) {
    return 3;
  }
  const std::uint32_t sub = k1::titan::liveGetU32(bytes.data());
  if (sub != k1::titan::kLiveCfgPatch) {
    return 4;
  }
  k1::titan::LivePatchEntry entries[8]{};
  std::uint32_t revision = 0, request_id = 0, boot_id = 0;
  std::uint16_t channel = 0, count = 0;
  if (!k1::titan::liveDecodePatch(bytes.data() + 4, bytes.size() - 4, &revision, &request_id,
                                 &channel, &boot_id, entries, &count, 8)) {
    return 5;
  }
  k1::titan::LiveConfigBlob cfg{};
  cfg.version = 2;
  cfg.revision = revision;
  cfg.boot_id = boot_id ? boot_id : 1U;
  cfg.visual_a.hue_position = 0.11F;
  cfg.visual_b.hue_position = 0.22F;
  const auto status = k1::titan::liveApplyPatch(&cfg, revision, cfg.boot_id, channel, entries, count);
  std::printf("status=%u ch=%u req=%u a=%.6f b=%.6f emit=%u\n",
              static_cast<unsigned>(status), unsigned(channel), unsigned(request_id),
              cfg.visual_a.hue_position, cfg.visual_b.hue_position, unsigned(cfg.emit_on));
  return status == k1::titan::LivePatchStatus::Ok ? 0 : 6;
}

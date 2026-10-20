#include "palette_runtime.h"

#include <cassert>
#include <cstdio>
#include <cstring>

int main() {
  const char* configured = k1::titan::configuredPaletteBackend();
  const char* emit_off = k1::titan::runtimePaletteOutputBackend(false);
  const char* emit_on = k1::titan::runtimePaletteOutputBackend(true);
  assert(std::strcmp(emit_off, "disabled") == 0);
#if defined(K1_PALETTE_GPT_DMA)
  assert(std::strcmp(configured, "gpt_dma") == 0);
#elif defined(K1_PALETTE_WS2816)
  assert(std::strcmp(configured, "ws2816_gpio") == 0);
#elif defined(K1_PALETTE_RUNTIME)
  assert(std::strcmp(configured, "gpio_diagnostic") == 0);
#else
  assert(std::strcmp(configured, "unknown") == 0);
#endif
  assert(std::strcmp(emit_on, configured) == 0);
  std::printf("PALETTE_BACKEND configured=%s emit_on=%s emit_off=%s "
              "physical_admission=unproven\n",
              configured, emit_on, emit_off);
  return 0;
}

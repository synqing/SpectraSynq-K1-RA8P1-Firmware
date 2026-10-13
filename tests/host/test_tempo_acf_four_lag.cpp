// Include the production implementation so the local four-row kernel itself,
// including every row hidden by comb interpolation, is tested directly.
#include "../../src/k1/core/audio/tempo_acf.cpp"
#include <cassert>
#include <cstdio>
#include <cstring>

int main() {
    std::array<float, k1::core::audio::kTempoAcfHistoryLength> work{};
    std::uint32_t random = 0x54454d50U;
    unsigned comparisons = 0;
    for (unsigned pattern = 0; pattern < 132U; ++pattern) {
        for (unsigned i = 0; i < work.size(); ++i) {
            random ^= random << 13; random ^= random >> 17; random ^= random << 5;
            const float value = static_cast<float>(static_cast<int>(random & 65535U) - 32768) / 32768.0F;
            if (pattern == 0U) work[i] = 0.0F;
            else if (pattern == 1U) work[i] = -0.0F;
            else if (pattern == 2U) work[i] = i == 256U ? 1.0F : 0.0F;
            else if (pattern == 3U) work[i] = i % 2U ? -4.0F : 4.0F;
            else work[i] = std::ldexp(value, static_cast<int>((random >> 16) % 81U) - 40);
        }
        // Every legal group, not merely the 181 rows at the inhouse rate.
        for (int lag = 0; lag <= 508; ++lag) {
            float actual[4]{}, expected[4]{};
            k1::core::audio::correlateFourLags(work.data(), lag, actual);
            for (int row = 0; row < 4; ++row) {
                const int row_lag = lag + row;
                for (int sample = row_lag; sample < 512; ++sample)
                    expected[row] += work[sample] * work[sample - row_lag];
                assert(std::memcmp(&actual[row], &expected[row], sizeof(float)) == 0);
                ++comparisons;
            }
        }
    }
    std::printf("exact four-lag kernel: %u row comparisons, all lags0..511, signed zero/impulse/alternating/extensive signed finite random inputs\n", comparisons);
}

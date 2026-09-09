#pragma once

#include <cstddef>
#include <cstdint>

namespace k1::core {

struct Pixel8 final {
    std::uint8_t red = 0;
    std::uint8_t green = 0;
    std::uint8_t blue = 0;
};

static_assert(sizeof(Pixel8) == 3U);

class PixelSpan final {
public:
    constexpr PixelSpan(Pixel8* const pixels, const std::size_t size) noexcept
        : pixels_(pixels), size_(size) {}

    [[nodiscard]] constexpr Pixel8* data() const noexcept { return pixels_; }
    [[nodiscard]] constexpr std::size_t size() const noexcept { return size_; }
    [[nodiscard]] constexpr Pixel8& operator[](
        const std::size_t index) const noexcept {
        return pixels_[index];
    }

private:
    Pixel8* pixels_;
    std::size_t size_;
};

class ConstPixelSpan final {
public:
    constexpr ConstPixelSpan(
        const Pixel8* const pixels, const std::size_t size) noexcept
        : pixels_(pixels), size_(size) {}
    constexpr ConstPixelSpan(const PixelSpan pixels) noexcept
        : pixels_(pixels.data()), size_(pixels.size()) {}

    [[nodiscard]] constexpr const Pixel8* data() const noexcept {
        return pixels_;
    }
    [[nodiscard]] constexpr std::size_t size() const noexcept { return size_; }
    [[nodiscard]] constexpr const Pixel8& operator[](
        const std::size_t index) const noexcept {
        return pixels_[index];
    }

private:
    const Pixel8* pixels_;
    std::size_t size_;
};

}  // namespace k1::core

#pragma once

#include <array>
#include <cstdint>

struct NativeLightmapPixelLayoutCandidate {
    std::uint32_t sourceImageFormat{};
    std::uint8_t bytesPerPixel{};
    std::uint8_t flags{};
    std::array<std::int16_t, 4> channelShifts{};
    std::array<std::uint32_t, 4> channelMasks{};
};

// Describes only formats observed after a successful texture allocation.
// This function does not interpret pixels or make rendering decisions.
inline bool nativeLightmapPixelLayoutCandidate(
    std::uint32_t actualD3DFormat,
    NativeLightmapPixelLayoutCandidate& output) noexcept {
    NativeLightmapPixelLayoutCandidate selected{};
    switch (actualD3DFormat) {
    case 21:
        selected = {12, 4, 0, {16, 8, 0, 24}, {255, 255, 255, 255}};
        break;
    case 26:
        selected = {19, 2, 0, {4, 0, -4, 8}, {240, 240, 240, 240}};
        break;
    case 36:
        selected = {25, 8, 0, {0, 16, 32, 48}, {65535, 65535, 65535, 65535}};
        break;
    case 113:
        selected = {24, 8, 3, {0, 16, 32, 48}, {65535, 65535, 65535, 65535}};
        break;
    default:
        return false;
    }
    output = selected;
    return true;
}

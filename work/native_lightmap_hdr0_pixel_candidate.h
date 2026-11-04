#pragma once

#include "native_lightmap_pixel_layout_candidate.h"

#include <array>
#include <cstdint>

inline bool nativeLightmapHdr0PixelCandidate(
    std::uint32_t actualD3DFormat,
    const std::array<std::uint32_t, 4>& quantizedSlots,
    std::array<std::uint8_t, 8>& output,
    std::uint8_t& bytesWritten) noexcept {
    NativeLightmapPixelLayoutCandidate layout{};
    if (!nativeLightmapPixelLayoutCandidate(actualD3DFormat, layout)) return false;

    const std::uint8_t r = static_cast<std::uint8_t>(quantizedSlots[0]);
    const std::uint8_t g = static_cast<std::uint8_t>(quantizedSlots[1]);
    const std::uint8_t b = static_cast<std::uint8_t>(quantizedSlots[2]);
    const std::uint8_t a = static_cast<std::uint8_t>(quantizedSlots[3]);
    std::array<std::uint8_t, 8> staged = output;
    std::uint8_t stagedBytes = 0;

    switch (actualD3DFormat) {
    case 21:
        staged[0] = b; staged[1] = g; staged[2] = r; staged[3] = a;
        stagedBytes = 4;
        break;
    case 26: {
        const std::uint16_t packed = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(a >> 4) << 12) |
            (static_cast<std::uint16_t>(r >> 4) << 8) |
            (static_cast<std::uint16_t>(g >> 4) << 4) |
            static_cast<std::uint16_t>(b >> 4));
        staged[0] = static_cast<std::uint8_t>(packed);
        staged[1] = static_cast<std::uint8_t>(packed >> 8);
        stagedBytes = 2;
        break;
    }
    case 36:
    case 113:
        staged[0] = r; staged[1] = 0;
        staged[2] = g; staged[3] = 0;
        staged[4] = b; staged[5] = 0;
        staged[6] = a; staged[7] = 0;
        stagedBytes = 8;
        break;
    default:
        return false;
    }

    output = staged;
    bytesWritten = stagedBytes;
    return true;
}

#pragma once

#include "native_lightmap_half_candidate.h"
#include "native_lightmap_pixel_layout_candidate.h"

#include <array>
#include <cstddef>
#include <cstdint>

// Encodes one HDR2 RGBA pixel for a verified successful D3D destination
// format. This does not select a device format or address image rows.
inline bool nativeLightmapHdr2PixelCandidate(
    std::uint32_t actualD3DFormat,
    const std::array<std::uint32_t, 4>& rgbaFloatBits,
    std::array<std::uint8_t, 8>& output) noexcept {
    NativeLightmapPixelLayoutCandidate layout{};
    if (!nativeLightmapPixelLayoutCandidate(actualD3DFormat, layout) ||
        (actualD3DFormat != 36u && actualD3DFormat != 113u)) {
        return false;
    }

    std::array<std::uint8_t, 8> staged{};
    for (std::size_t channel = 0; channel < rgbaFloatBits.size(); ++channel) {
        const std::uint16_t word = actualD3DFormat == 36u
            ? static_cast<std::uint16_t>(rgbaFloatBits[channel] & 0xffffu)
            : nativeLightmapHalfBitsCandidate(rgbaFloatBits[channel]);
        staged[channel * 2u] = static_cast<std::uint8_t>(word & 0xffu);
        staged[channel * 2u + 1u] = static_cast<std::uint8_t>(word >> 8u);
    }
    output = staged;
    return true;
}

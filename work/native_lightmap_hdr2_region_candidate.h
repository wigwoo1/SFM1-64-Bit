#pragma once

#include "native_lightmap_hdr2_pixel_candidate.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

using NativeLightmapHdr2BitsCandidate = std::array<std::uint32_t, 4>;
struct NativeLightmapHdr2RegionCandidate {
    std::uint32_t width{}, height{}, x{}, y{};
    std::size_t rowStrideBytes{};
};

namespace native_lightmap_hdr2_region_detail {
inline bool add(std::size_t a, std::size_t b, std::size_t& out) noexcept {
    if (b > (std::numeric_limits<std::size_t>::max)() - a) return false;
    out = a + b;
    return true;
}
inline bool multiply(std::size_t a, std::size_t b, std::size_t& out) noexcept {
    if (a != 0 && b > (std::numeric_limits<std::size_t>::max)() / a) return false;
    out = a * b;
    return true;
}
} // namespace native_lightmap_hdr2_region_detail

// Borrowed source vectors are used synchronously; no references escape.
// Zero dimensions are refused by this owned API. Allocation exceptions may
// propagate, with destination preserved because only the staged copy changes.
inline bool nativeLightmapHdr2RegionCandidate(
    std::uint32_t actualD3DFormat,
    const std::array<const std::vector<NativeLightmapHdr2BitsCandidate>*, 4>& planes,
    const NativeLightmapHdr2RegionCandidate& region,
    std::vector<std::uint8_t>& destination) {
    using namespace native_lightmap_hdr2_region_detail;
    if ((actualD3DFormat != 36u && actualD3DFormat != 113u) || !planes[0] ||
        !region.width || !region.height) return false;
    const std::size_t countPlanes = planes[1] && planes[2] && planes[3] ? 4u : 1u;
    std::size_t count{}, stripWidth{}, rowPixels{}, rowEnd{}, lastRow{}, rowBase{}, end{};
    if (!multiply(region.width, region.height, count) ||
        !multiply(countPlanes, region.width, stripWidth) ||
        !add(region.x, stripWidth, rowPixels) ||
        !multiply(rowPixels, 8u, rowEnd) ||
        !add(region.y, static_cast<std::size_t>(region.height) - 1u, lastRow) ||
        !multiply(lastRow, region.rowStrideBytes, rowBase) ||
        !add(rowBase, rowEnd, end) || rowEnd > region.rowStrideBytes ||
        end > destination.size()) return false;
    for (std::size_t plane = 0; plane < countPlanes; ++plane) {
        if (planes[plane]->size() != count) return false;
    }
    auto staged = destination;
    for (std::size_t y = 0; y < region.height; ++y) {
        for (std::size_t x = 0; x < region.width; ++x) {
            const std::size_t index = y * region.width + x;
            for (std::size_t plane = 0; plane < countPlanes; ++plane) {
                auto bits = (*planes[plane])[index];
                bits[3] = (*planes[0])[index][3];
                std::array<std::uint8_t, 8> pixel{};
                if (!nativeLightmapHdr2PixelCandidate(actualD3DFormat, bits, pixel)) return false;
                const std::size_t offset = (static_cast<std::size_t>(region.y) + y) * region.rowStrideBytes +
                    (static_cast<std::size_t>(region.x) + x + plane * region.width) * 8u;
                std::copy(pixel.begin(), pixel.end(), staged.data() + offset);
            }
        }
    }
    destination.swap(staged);
    return true;
}

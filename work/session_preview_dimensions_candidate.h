#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>

// Preview size follows the saved movie aspect without upscaling or allocating
// a full movie-sized target. Round the shorter side to the nearest pixel.
inline std::array<uint32_t, 2> sessionPreviewDimensionsCandidate(
    uint32_t width, uint32_t height, uint32_t maximumSide = 512) {
    if (!width || !height || !maximumSide)
        throw std::runtime_error("preview dimensions must be positive");
    const uint32_t longest = width > height ? width : height;
    if (longest <= maximumSide) return {width, height};
    auto scaled = [&](uint32_t side) {
        return uint32_t((uint64_t(side) * maximumSide + longest / 2) / longest);
    };
    const auto result = std::array<uint32_t, 2>{scaled(width), scaled(height)};
    if (!result[0] || !result[1])
        throw std::runtime_error("preview aspect cannot fit one pixel");
    return result;
}

#pragma once

#include <array>
#include <cmath>
#include <optional>

// First two rows of the selected base/bump transform, as Phong supplies them
// to the scaled vertex-transform command. This uses the original column and
// translation scaling even when the UV transform rotates or shears.
inline std::optional<std::array<float, 8>> scalePhongDetailUvCandidate(
    const std::array<float, 8>& transform, const std::array<float, 2>& scale) {
    const std::array<float, 8> result{
        transform[0] * scale[0], transform[1] * scale[1], transform[2], transform[3] * scale[0],
        transform[4] * scale[0], transform[5] * scale[1], transform[6], transform[7] * scale[1],
    };
    for (const auto value : result)
        if (!std::isfinite(value)) return {};
    return result;
}

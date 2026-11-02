#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <xmmintrin.h>

// Stateless ordinary lightmap-coordinate calculation. This does not own or
// allocate atlas pages and does not imply full material/rendering admission.
struct SourcePlainLightmapSamplingInputCandidate {
    std::array<std::array<float, 4>, 2> axes{};
    std::array<int32_t, 2> minima{}, size{}, pageOffset{};
    std::array<uint32_t, 2> pageDimensions{};
    std::array<float, 3> position{};
    uint32_t surfaceFlags = 0;
};

inline bool sourcePlainLightmapSamplingCandidate(
    std::array<float, 2>& destination,
    const SourcePlainLightmapSamplingInputCandidate& input) {
    const uint32_t mxcsr = _mm_getcsr();
    // Match the existing CPU lightmap arithmetic policy without changing it.
    if ((mxcsr & 0xe040u) != 0 || (mxcsr & 0x1f80u) != 0x1f80u)
        return false;
    if (input.surfaceFlags != 0)
        return false;

    for (size_t axis = 0; axis != 2; ++axis) {
        const uint32_t dimension = input.pageDimensions[axis];
        const int32_t extent = input.size[axis];
        const int32_t minimum = input.minima[axis];
        const int32_t offset = input.pageOffset[axis];
        if (dimension == 0 || dimension > 32768 || extent < 0 || extent > 32767 ||
            minimum < -32768 || minimum > 32767 || offset < 0 || offset > 32767)
            return false;
        const int64_t allocationEnd = int64_t(offset) + int64_t(extent) + 1;
        if (allocationEnd > int64_t(dimension))
            return false;
    }
    for (const auto& axis : input.axes)
        for (float value : axis)
            if (!std::isfinite(value)) return false;
    for (float value : input.position)
        if (!std::isfinite(value)) return false;

    // Explicit scalar SSE operations preserve the installed sequence and
    // prevent contraction or reassociation by the optimizer.
    const auto roundedAdd = [](float a, float b) {
        return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a), _mm_set_ss(b)));
    };
    const auto roundedSub = [](float a, float b) {
        return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a), _mm_set_ss(b)));
    };
    const auto roundedMul = [](float a, float b) {
        return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a), _mm_set_ss(b)));
    };

    std::array<float, 2> staged{};
    std::array<float, 2> reciprocal{};
    std::array<float, 2> origin{};
    for (size_t axis = 0; axis != 2; ++axis) {
        reciprocal[axis] = _mm_cvtss_f32(_mm_div_ss(
            _mm_set_ss(1.0f), _mm_set_ss(float(input.pageDimensions[axis]))));
        origin[axis] = roundedMul(float(input.pageOffset[axis]), reciprocal[axis]);
    }

    if (input.size[0] == 0) {
        for (size_t axis = 0; axis != 2; ++axis) {
            const float centered = roundedMul(reciprocal[axis], 0.5f);
            staged[axis] = roundedAdd(origin[axis], centered);
        }
    } else {
        for (size_t axis = 0; axis != 2; ++axis) {
            const auto& a = input.axes[axis];
            float value = roundedMul(a[1], input.position[1]);
            float term = roundedMul(input.position[0], a[0]);
            value = roundedAdd(value, term);
            term = roundedMul(a[2], input.position[2]);
            value = roundedAdd(value, term);
            value = roundedAdd(value, a[3]);
            value = roundedSub(value, float(input.minima[axis]));
            value = roundedAdd(value, 0.5f);
            value = roundedMul(reciprocal[axis], value);
            staged[axis] = roundedAdd(origin[axis], value);
        }
    }

    for (float& value : staged) {
        if (!std::isfinite(value)) return false;
        if (value > 1.0f) value = 1.0f;
        else if (0.0f > value) value = 0.0f;
    }
    destination = staged;
    return true;
}

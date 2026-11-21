#pragma once
#include <array>
#include <cmath>
#include <cstdint>

using SourceAmbientOcclusionKernelCandidate = std::array<std::array<float, 4>, 16>;
inline constexpr float sourceAmbientOcclusionRandomUnitCandidate = 1.f / 32767.f;
inline constexpr uint32_t sourceAmbientOcclusionPhaseExtentCandidate = 32;

inline int nextSourceAmbientOcclusionRandomCandidate(uint32_t& state) {
    state = state * 214013u + 2531011u;
    return int((state >> 16) & 32767u);
}

// IFM's render-sample stream is separate from the shader's initializer.
// The supplied state belongs to the current draw, before its optional step.
inline std::array<float, 2> sourceAmbientOcclusionPhaseCandidate(uint32_t sample, uint32_t& state) {
    if (!sample) return {};
    constexpr uint32_t extent = sourceAmbientOcclusionPhaseExtentCandidate;
    const float position = float(nextSourceAmbientOcclusionRandomCandidate(state)) *
                           sourceAmbientOcclusionRandomUnitCandidate;
    const auto cell = uint32_t(std::fmod(double(position), 1.) * extent * extent);
    return {float(cell / extent) / extent, float(cell % extent) / extent};
}

// Reconstruct the shader's cached 4-by-4 stratified sample set. The caller
// supplies the initializer's random state explicitly for repeatable diagnostics.
inline SourceAmbientOcclusionKernelCandidate makeSourceAmbientOcclusionKernelCandidate(uint32_t& state) {
    constexpr int strata = 4;
    constexpr float reciprocalCenteredMax = 1.f / 16383.f;
    constexpr double fullTurn = 6.283185307179586;
    const auto positionInStratum = [&]() {
        return std::abs(float(nextSourceAmbientOcclusionRandomCandidate(state) - 16383) * reciprocalCenteredMax);
    };
    SourceAmbientOcclusionKernelCandidate result{};
    for (int row = 0; row < strata; ++row) {
        for (int column = 0; column < strata; ++column) {
            const float latitude = (float(row) + positionInStratum()) / strata;
            const float polar = 2.f * std::acos(std::sqrt(1.f - latitude));
            const float turn = (float(column) + positionInStratum()) / strata;
            const float azimuth = float(double(turn) * fullTurn);
            const float horizontal = std::sin(polar);
            result[size_t(row * strata + column)] = {std::cos(azimuth) * horizontal,
                                                     std::sin(azimuth) * horizontal,
                                                     std::cos(polar), 1.f};
        }
    }
    for (auto& direction : result) {
        const float scale = float(nextSourceAmbientOcclusionRandomCandidate(state)) * sourceAmbientOcclusionRandomUnitCandidate;
        for (size_t component = 0; component < 3; ++component) direction[component] *= scale;
    }
    return result;
}

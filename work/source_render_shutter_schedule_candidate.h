#pragma once

#include "recovered_tick_seconds_candidate.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

struct SourceRenderShutterSampleCandidate {
    int32_t tick;
    float advanceSeconds;
};

inline constexpr uint32_t sourceRenderRefinementSampleLimitCandidate = 1024;

// The caller has already selected the motion-refinement route and its sample
// counts. Each group evaluates one shutter time, with one or more lens draws.
inline std::vector<SourceRenderShutterSampleCandidate> sourceRenderShutterScheduleCandidate(
        int32_t outputTick, int32_t frameDurationTicks, int32_t shutterTicks,
        uint32_t shutterSamples, uint32_t lensSamples) {
    if (frameDurationTicks <= 0 || shutterTicks < 0 || !shutterSamples || !lensSamples ||
        shutterSamples > sourceRenderRefinementSampleLimitCandidate / lensSamples)
        throw std::invalid_argument("unsupported Source shutter schedule input");

    // The native time operators round each intermediate in float32, including
    // the step before it is used to center the sequence around the output tick.
    const auto step = int64_t(std::floor(float(shutterTicks) / float(shutterSamples) + .5f));
    const auto span = int64_t(std::floor(float(step) * float(shutterSamples - 1) + .5f));
    const auto centerOffset = int64_t(std::floor(float(span) / 2.f + .5f));
    const auto firstTick = int64_t(outputTick) - centerOffset;
    const auto lastTick = firstTick + step * (shutterSamples - 1);
    if (firstTick < std::numeric_limits<int32_t>::min() ||
        lastTick > std::numeric_limits<int32_t>::max() ||
        step > std::numeric_limits<int32_t>::max() || span > std::numeric_limits<int32_t>::max())
        throw std::out_of_range("Source shutter schedule outside time range");

    std::vector<SourceRenderShutterSampleCandidate> samples;
    samples.reserve(shutterSamples * lensSamples);
    for (uint32_t shutter = 0; shutter < shutterSamples; ++shutter) {
        const auto tick = int32_t(firstTick + shutter * step);
        const auto advance = shutter == 0 ? frameDurationTicks - shutterTicks : int32_t(step);
        samples.push_back({tick, recoveredTickSecondsCandidate(advance)});
        for (uint32_t lens = 1; lens < lensSamples; ++lens)
            samples.push_back({tick, 0.f});
    }
    return samples;
}

struct SourceRenderPointOrdinalsCandidate {
    uint32_t lens, raster;
};

inline SourceRenderPointOrdinalsCandidate sourceRenderPointOrdinalsCandidate(
        uint32_t ordinal, uint32_t totalSamples) {
    if (totalSamples > sourceRenderRefinementSampleLimitCandidate || ordinal >= totalSamples)
        throw std::invalid_argument("invalid refinement sample ordinal");
    return {ordinal, (ordinal + 7) % totalSamples};
}

// Independently generated, equal-area disk samples. The ordinal routing above
// follows Source, while this default distribution is specific to SFM64.
inline std::array<float, 2> renderRefinementDiskPointCandidate(uint32_t ordinal, uint32_t count) {
    constexpr double goldenAngle = 2.399963229728653;
    // Quality levels use powers of two. Interleave radial strata across the
    // shutter so early times receive both central and peripheral lens rays.
    uint32_t stratum = 0;
    for (uint32_t span = count; span > 1; span >>= 1) {
        stratum = (stratum << 1) | (ordinal & 1u);
        ordinal >>= 1;
    }
    const double radius = std::sqrt((double(stratum) + .5) / double(count));
    const double angle = stratum * goldenAngle;
    return {float(radius * std::cos(angle)), float(radius * std::sin(angle))};
}

#pragma once

#include "recovered_tick_seconds_candidate.h"
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

struct SourceRenderShutterSampleCandidate {
    int32_t tick;
    float advanceSeconds;
};

// The caller has already selected the motion-refinement route and its sample
// counts. Each group evaluates one shutter time, with one or more lens draws.
inline std::vector<SourceRenderShutterSampleCandidate> sourceRenderShutterScheduleCandidate(
        int32_t outputTick, int32_t frameDurationTicks, int32_t shutterTicks,
        uint32_t shutterSamples, uint32_t lensSamples) {
    constexpr uint32_t sampleLimit = 256;
    if (frameDurationTicks <= 0 || shutterTicks < 0 || !shutterSamples || !lensSamples ||
        shutterSamples > sampleLimit / lensSamples)
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

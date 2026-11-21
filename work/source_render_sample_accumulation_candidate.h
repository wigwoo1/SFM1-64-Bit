#pragma once
#include <array>
#include <cstdint>

// IFM supplies the current sample first and the previous accumulation second.
// The pair is uploaded to the accumulation program's first weight register.
inline std::array<float, 2> sourceRenderSampleWeightsCandidate(uint32_t sampleOrdinal) {
    const float current = 1.f / (float(sampleOrdinal) + 1.f);
    return {current, 1.f - current};
}

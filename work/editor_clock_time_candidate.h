#pragma once
#include "seconds_to_sfm_time_candidate.h"
// 10275640: caller-owned shared cache replaces original global10f321d0.
// Engine sample comparison precision must match its original x87 return ABI.
template<class Sample>
inline std::int32_t editorClockTimeCandidate(float& cachedSeconds,Sample engineSample) {
    if(engineSample>cachedSeconds)cachedSeconds=static_cast<float>(engineSample);
    return secondsToSfmTimeCandidate(cachedSeconds);
}

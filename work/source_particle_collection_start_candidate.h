#pragma once

#include "recovered_tick_seconds_candidate.h"
#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>

// The unbudgeted update takes its history and step limits from shot context.
// Wall-clock-budgeted preview changes those limits while it catches up.
inline constexpr int32_t sourceParticleUnbudgetedHistoryCandidate = 1073741824;
inline constexpr int32_t sourceParticleUnbudgetedStepCandidate = 150;

struct SourceParticleStartCandidate {
    int32_t cursor;
    int32_t skippedTicks;
    float skipSeconds;
    bool stopAfterSkip;
};

inline bool sourceParticleCollectionResetCandidate(int32_t cursor, int32_t target,
                                                   int32_t historyTicks) {
    if (historyTicks <= 0) throw std::invalid_argument("invalid particle history window");
    const int64_t elapsed = int64_t(target) - cursor;
    return elapsed < 0 || elapsed >= historyTicks;
}

// Creation precedes this timing decision. A missing earlier key can fall
// back to the first key, provided the first key is at or before the target.
// Apply control points at the returned cursor before either a zero-duration
// initialization (skippedTicks == 0) or aging over a clipped history window.
template<class Simulating>
inline std::optional<SourceParticleStartCandidate> sourceParticleCollectionStartCandidate(
        int32_t target, std::optional<int32_t> keyAtTarget, std::optional<int32_t> firstKey,
        int32_t historyTicks, float simulationScale, Simulating simulating) {
    if (historyTicks <= 0 || !std::isfinite(simulationScale))
        throw std::invalid_argument("invalid particle collection startup inputs");
    const auto key = keyAtTarget ? keyAtTarget : firstKey;
    if (!key || (!keyAtTarget && *key > target)) return {};
    const int64_t floor = int64_t(target) - historyTicks;
    if (floor < *key) return SourceParticleStartCandidate{*key, 0, 0.f, false};
    const auto cursor = static_cast<int32_t>(floor);
    return SourceParticleStartCandidate{cursor, historyTicks,
                                         recoveredTickSecondsCandidate(historyTicks) * simulationScale,
                                         !simulating(cursor)};
}

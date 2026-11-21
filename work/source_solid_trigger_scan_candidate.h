#pragma once

#include "source_displacement_bounds_gate_candidate.h"
#include <cstdint>
#include <optional>
#include <vector>

using SourceTriggerPointCandidate = std::array<float, 3>;

struct SourceTriggerBoundsCandidate {
    SourceTriggerPointCandidate min, max;
};

struct SourceSolidTriggerSweepCandidate {
    SourceTriggerPointCandidate center{}, delta{}, offset{}, extents{};
    bool isRay = true;
    bool swept = false;
};

inline SourceSolidTriggerSweepCandidate sourceSolidTriggerSweepCandidate(
    const SourceTriggerPointCandidate& origin, const std::optional<SourceTriggerPointCandidate>& previous,
    uint32_t solidType, bool accurate, const SourceTriggerBoundsCandidate& local,
    const SourceTriggerBoundsCandidate& surrounding) {
    constexpr uint32_t bboxSolid = 2;
    auto scan = SourceSolidTriggerSweepCandidate{};
    const auto& start = previous ? *previous : origin;
    const bool useLocalBounds = accurate && solidType == bboxSolid;
    const auto& bounds = useLocalBounds ? local : surrounding;
    for (std::size_t axis = 0; axis < 3; ++axis) {
        const float low = useLocalBounds ? bounds.min[axis] : bounds.min[axis] - origin[axis];
        const float high = useLocalBounds ? bounds.max[axis] : bounds.max[axis] - origin[axis];
        const float midpoint = (low + high) * 0.5f;
        scan.center[axis] = start[axis] + midpoint;
        scan.delta[axis] = origin[axis] - start[axis];
        scan.offset[axis] = -1.f * midpoint;
        scan.extents[axis] = (high - low) * 0.5f;
    }
    const auto squaredLength = [](const SourceTriggerPointCandidate& v) {
        return (double(v[0]) * v[0] + double(v[1]) * v[1]) + double(v[2]) * v[2];
    };
    scan.isRay = squaredLength(scan.extents) < 1e-6;
    const auto& d = scan.delta;
    scan.swept = ((d[0] * d[0] + d[1] * d[1]) + d[2] * d[2]) != 0.f;
    return scan;
}

inline bool sourceSolidTriggerBoundsHitCandidate(
    const SourceSolidTriggerSweepCandidate& sweep, const SourceTriggerBoundsCandidate& bounds) {
    if (!sweep.swept) {
        for (std::size_t axis = 0; axis < 3; ++axis) {
            if (bounds.min[axis] > sweep.center[axis] + sweep.extents[axis] ||
                bounds.max[axis] < sweep.center[axis] - sweep.extents[axis]) return false;
        }
        return true;
    }
    auto expanded = bounds;
    for (std::size_t axis = 0; axis < 3; ++axis) {
        expanded.min[axis] -= sweep.extents[axis];
        expanded.max[axis] += sweep.extents[axis];
    }
    const auto& d = sweep.delta;
    auto delta = _mm_set_ps(1.f, d[2], d[1], d[0]);
    const auto zero = _mm_cmpeq_ps(delta, _mm_setzero_ps());
    delta = _mm_or_ps(delta, _mm_and_ps(zero, _mm_set1_ps(std::numeric_limits<float>::epsilon())));
    const auto approximate = _mm_rcp_ps(delta);
    const auto square = _mm_mul_ps(approximate, approximate);
    const auto refined = _mm_sub_ps(_mm_add_ps(approximate, approximate), _mm_mul_ps(square, delta));
    alignas(16) std::array<float, 4> inverse{};
    _mm_store_ps(inverse.data(), refined);
    return sourceDisplacementBoundsGateCandidate(
        expanded.min, expanded.max, sweep.center, d, {inverse[0], inverse[1], inverse[2]}, 0.f);
}

struct SourceSolidTriggerTargetCandidate {
    uint32_t identity, solidFlags;
    bool isMover, isServerEntity, hasNetworkable;
    SourceTriggerBoundsCandidate triggerBounds;
};

// The spatial index supplies targets in query order. Keep that order and
// finish collecting contacts before their callbacks can change the map.
template<typename CanTouch, typename Clip>
std::vector<uint32_t> sourceSolidTriggerContactsCandidate(
    const SourceSolidTriggerSweepCandidate& sweep, const std::vector<SourceSolidTriggerTargetCandidate>& targets,
    CanTouch canTouch, Clip clip) {
    constexpr uint32_t useTriggerBounds = 0x80;
    constexpr uint32_t solidTraceMask = 0x0200400b;
    std::vector<uint32_t> contacts;
    for (const auto& target : targets) {
        if (target.isMover || !target.isServerEntity || !target.hasNetworkable || !canTouch(target.solidFlags)) continue;
        const bool hit = target.solidFlags & useTriggerBounds
            ? sourceSolidTriggerBoundsHitCandidate(sweep, target.triggerBounds)
            : (clip(sweep, target.identity, solidTraceMask) & solidTraceMask) != 0;
        if (hit) contacts.push_back(target.identity);
    }
    return contacts;
}

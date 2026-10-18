#pragma once
#include <array>
#include <vector>

// Recovered arithmetic from ifm.dll 105CF4A0. World transforms and
// target-handle validation belong to the caller; this is not a DAG evaluator.
struct RecoveredPointConstraintTarget {
    std::array<float, 3> worldPosition;
    std::array<float, 3> offset;
    float weight;
    bool hasResolvedDag;
};
struct RecoveredPointConstraintAverage {
    std::array<float, 3> position;
    float influence;
};
inline RecoveredPointConstraintAverage accumulateRecoveredPointConstraint(
    const std::vector<RecoveredPointConstraintTarget>& targets,
    const std::array<float, 3>& initializedGlobalVector) {
    auto position = initializedGlobalVector;
    float weight = 0.0f;
    for (const auto& target : targets) {
        if (!target.hasResolvedDag) continue;
        for (unsigned c = 0; c < 3; ++c) {
            const float translated = target.worldPosition[c] + target.offset[c];
            const float weighted = translated * target.weight;
            position[c] = position[c] + weighted;
        }
        weight = target.weight + weight;
    }
    if (weight > 0.0f) {
        const float reciprocal = 1.0f / weight;
        for (auto& component : position) component = component * reciprocal;
    }
    // Native COMISS/JBE preserves NaN and negative weights here.
    return {position, weight > 1.0f ? 1.0f : weight};
}

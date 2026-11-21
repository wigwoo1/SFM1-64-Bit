#pragma once

#include "quaternion_transform_matrix.h"
#include <cmath>
#include <stdexcept>

struct SourceParticleControlPointCandidate {
    std::array<float, 3> position, forward, right, up;
};

inline SourceParticleControlPointCandidate sourceParticleControlPointCandidate(
        const std::array<float, 3>& position, const std::array<float, 4>& orientation,
        const RecoveredMatrix3x4& parent, bool bypassParent) {
    const auto local = recoveredQuaternionTransformMatrix(orientation, position);
    const auto matrix = bypassParent ? local : composeRecoveredTransforms(parent, local);
    SourceParticleControlPointCandidate point{};
    for (unsigned axis = 0; axis < 3; ++axis) {
        for (float component : matrix[axis])
            if (!std::isfinite(component)) throw std::runtime_error("nonfinite particle control point");
        point.position[axis] = matrix[axis][3];
        point.forward[axis] = matrix[axis][0];
        point.right[axis] = matrix[axis][1] * -1.f;
        point.up[axis] = matrix[axis][2];
    }
    return point;
}

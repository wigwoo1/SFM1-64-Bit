#pragma once
#include "transform_matrix_inverse.h"
#include <cmath>
// Scale must come from the recovered column-length geometric-mean helper.
// Keeping it explicit prevents silently substituting a general inverse.
inline RecoveredMatrix3x4 correctedRecoveredParentInverse(
    RecoveredMatrix3x4 parent, float recoveredScale) {
    const float squaredScale = recoveredScale * recoveredScale;
    const float correction = 1.0f / squaredScale;
    if (!std::isnan(correction) && correction != 1.0f)
        for (unsigned row=0; row<3; ++row)
            for (unsigned column=0; column<3; ++column)
                parent[row][column] = parent[row][column] * correction;
    return invertRecoveredTransposeTransform(parent);
}
inline RecoveredMatrix3x4 convertRecoveredWorldToLocal(
    const RecoveredMatrix3x4& world,
    const RecoveredMatrix3x4& positionParent, float positionScale,
    const RecoveredMatrix3x4& rotationParent, float rotationScale,
    bool sameParent) {
    const auto positionInverse = correctedRecoveredParentInverse(positionParent, positionScale);
    const auto positionLocal = composeRecoveredTransforms(positionInverse, world);
    if (sameParent) return positionLocal;
    const auto rotationInverse = correctedRecoveredParentInverse(rotationParent, rotationScale);
    auto result = composeRecoveredTransforms(rotationInverse, world);
    for (unsigned row=0; row<3; ++row) result[row][3] = positionLocal[row][3];
    return result;
}

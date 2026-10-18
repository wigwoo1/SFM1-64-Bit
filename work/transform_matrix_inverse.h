#pragma once
#include "transform_matrix_composition.h"
// Recovered 1049D6D0 transpose-based inverse. The caller must apply
// SFM's scale correction first; this is not a general affine inverse.
inline RecoveredMatrix3x4 invertRecoveredTransposeTransform(
    const RecoveredMatrix3x4& matrix) {
    RecoveredMatrix3x4 result{};
    for (unsigned row=0; row<3; ++row) {
        for (unsigned column=0; column<3; ++column)
            result[row][column]=matrix[column][row];
        const float y=result[row][1]*matrix[1][3];
        const float x=result[row][0]*matrix[0][3];
        const float yx=y+x;
        const float z=result[row][2]*matrix[2][3];
        result[row][3]=-(yx+z);
    }
    return result;
}

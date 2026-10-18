#pragma once
#include <array>
using RecoveredMatrix3x4 = std::array<std::array<float, 4>, 3>;
// ifm.dll 10498A60: row-major affine composition. Native SIMD adds
// the z product before y, then x, then the masked translation lane.
inline RecoveredMatrix3x4 composeRecoveredTransforms(
    const RecoveredMatrix3x4& parent, const RecoveredMatrix3x4& local) {
    RecoveredMatrix3x4 result{};
    for (unsigned row = 0; row < 3; ++row) {
        for (unsigned column = 0; column < 4; ++column) {
            const float z = parent[row][2] * local[2][column];
            const float y = parent[row][1] * local[1][column];
            const float x = parent[row][0] * local[0][column];
            const float zy = z + y;
            const float zyx = zy + x;
            result[row][column] = zyx + (column == 3 ? parent[row][3] : 0.0f);
        }
    }
    return result;
}

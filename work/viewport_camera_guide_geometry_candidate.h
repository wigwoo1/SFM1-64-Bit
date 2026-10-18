#pragma once
#include "render_camera_candidate.h"
#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

struct ViewportCameraGuideCandidate {
    std::array<std::array<float, 3>, 8> corners{};
    std::array<uint8_t, 24> lineIndices{};
};

inline ViewportCameraGuideCandidate viewportCameraFrustumCandidate(const RenderCameraCandidate& viewProjection) {
    // Invert the row-major world-to-clip matrix with pivoting. Vulkan clip depth
    // is [0,1], matching recoveredCameraProjectionCandidate.
    double augmented[4][8]{};
    for (size_t row = 0; row < 4; ++row) {
        for (size_t column = 0; column < 4; ++column) {
            const float value = viewProjection.rows[row * 4 + column];
            if (!std::isfinite(value)) throw std::runtime_error("nonfinite camera guide matrix");
            augmented[row][column] = value;
        }
        augmented[row][row + 4] = 1.0;
    }
    for (size_t column = 0; column < 4; ++column) {
        size_t pivot = column;
        for (size_t row = column + 1; row < 4; ++row)
            if (std::abs(augmented[row][column]) > std::abs(augmented[pivot][column])) pivot = row;
        if (std::abs(augmented[pivot][column]) < 1e-12) throw std::runtime_error("singular camera guide matrix");
        if (pivot != column) for (size_t n = 0; n < 8; ++n) std::swap(augmented[pivot][n], augmented[column][n]);
        const double divisor = augmented[column][column];
        for (double& value : augmented[column]) value /= divisor;
        for (size_t row = 0; row < 4; ++row) if (row != column) {
            const double factor = augmented[row][column];
            for (size_t n = 0; n < 8; ++n) augmented[row][n] -= factor * augmented[column][n];
        }
    }

    ViewportCameraGuideCandidate result{};
    for (size_t depth = 0; depth < 2; ++depth) for (size_t corner = 0; corner < 4; ++corner) {
        const double clip[4]{(corner & 1) ? 1.0 : -1.0, (corner & 2) ? 1.0 : -1.0, double(depth), 1.0};
        double world[4]{};
        for (size_t row = 0; row < 4; ++row)
            for (size_t column = 0; column < 4; ++column) world[row] += augmented[row][column + 4] * clip[column];
        if (!std::isfinite(world[3]) || std::abs(world[3]) < 1e-12) throw std::runtime_error("camera guide corner at infinity");
        for (size_t axis = 0; axis < 3; ++axis) {
            const double value = world[axis] / world[3];
            if (!std::isfinite(value) || std::abs(value) > double(FLT_MAX)) throw std::runtime_error("camera guide corner overflow");
            result.corners[depth * 4 + corner][axis] = float(value);
        }
    }
    // Four edges on each clip plane plus four camera-to-far-plane edges.
    result.lineIndices = {0,1, 1,3, 3,2, 2,0, 4,5, 5,7, 7,6, 6,4, 0,4, 1,5, 2,6, 3,7};
    return result;
}

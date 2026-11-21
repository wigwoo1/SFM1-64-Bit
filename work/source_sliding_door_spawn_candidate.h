#pragma once

#include "angle_basis_candidate.h"
#include <cmath>

struct SourceSlidingDoorSpawnCandidate {
    std::array<float, 3> closedOrigin{};
    std::array<float, 3> openOrigin{};
    std::array<float, 3> initialOrigin{};
    std::array<float, 3> direction{};
    float speed = 0.f;
    bool initiallyOpen = false;
    bool startsLocked = false;
};

// Parentless func_door with zero entity rotation and a loaded brush model.
// The movement direction angles describe travel, independently of entity rotation.
// Bounds are the model's loaded collision mins and maxs, including its padding.
inline SourceSlidingDoorSpawnCandidate sourceSlidingDoorSpawnCandidate(
    const std::array<float, 3>& origin, const std::array<float, 3>& moveAngles,
    const std::array<float, 6>& loadedBounds, float lip, float speed,
    int32_t spawnPosition, uint32_t spawnFlags) {
    if (!std::isfinite(lip) || !std::isfinite(speed)) throw std::invalid_argument("invalid door movement");
    for (float value : origin) {
        if (!std::isfinite(value)) throw std::invalid_argument("nonfinite door origin");
    }
    const std::array<float, 3> forwardAngles{moveAngles[0], moveAngles[1], 0.f};
    std::array<uint32_t, 3> angleBits{};
    std::memcpy(angleBits.data(), forwardAngles.data(), sizeof(angleBits));
    const auto basis = angleBasisCandidate(angleBits);

    SourceSlidingDoorSpawnCandidate result;
    std::memcpy(result.direction.data(), basis.data(), sizeof(result.direction));
    result.closedOrigin = origin;
    std::array<float, 3> axialTravel{};
    for (size_t axis = 0; axis < 3; ++axis) {
        const float low = loadedBounds[axis], high = loadedBounds[axis + 3];
        if (!std::isfinite(low) || !std::isfinite(high) || high < low) {
            throw std::invalid_argument("invalid loaded door bounds");
        }
        const float span = high - low;
        axialTravel[axis] = std::abs((span - 2.f) * result.direction[axis]);
    }
    const float travel = (axialTravel[1] + axialTravel[0]) + axialTravel[2] - lip;
    for (size_t axis = 0; axis < 3; ++axis) {
        result.openOrigin[axis] = origin[axis] + result.direction[axis] * travel;
        if (!std::isfinite(result.openOrigin[axis])) throw std::invalid_argument("unrepresentable door destination");
    }
    constexpr uint32_t startOpen = 1u << 0;
    constexpr uint32_t startLocked = 1u << 11;
    result.initiallyOpen = spawnPosition == 1 || (spawnFlags & startOpen);
    result.initialOrigin = result.initiallyOpen ? result.openOrigin : result.closedOrigin;
    result.startsLocked = (spawnFlags & startLocked) != 0;
    result.speed = speed == 0.f ? 100.f : speed;
    return result;
}

#pragma once

#include "source_particle_control_point_candidate.h"
#include "source_particle_random_candidate.h"

#include <algorithm>
#include <limits>

using SourceParticleVectorCandidate = std::array<float, 3>;

struct SourceParticleSphereVolumeSampleCandidate {
    SourceParticleVectorCandidate offset;
    float radiusFraction;
};

inline SourceParticleSphereVolumeSampleCandidate sourceParticleSphereVolumeSampleCandidate(
        SourceParticleRandomCandidate& random, const SourceParticleRandomTableCandidate& table) {
    // Keep the polar and azimuth samples away from the closed interval endpoints.
    const float polar = random.linear(table, .0001f, 1.f);
    const float azimuth = random.linear(table, .0001f, 1.f);
    const float volume = random.linear(table, .0001f, 1.f);
    const float pitch = std::acos(1.f - 2.f * polar);
    const float yaw = static_cast<float>(static_cast<double>(azimuth) * 6.283185307179586);
    const float radius = static_cast<float>(std::cbrt(static_cast<double>(volume)));
    const float horizontal = static_cast<float>(std::sin(static_cast<double>(pitch))) * radius;
    return {{
        horizontal * static_cast<float>(std::cos(static_cast<double>(yaw))),
        horizontal * static_cast<float>(std::sin(static_cast<double>(yaw))),
        radius * static_cast<float>(std::cos(static_cast<double>(pitch)))}, radius};
}

// The ordinary world-space sphere initializer, with no distance bias or
// control-point scaling and with radial speed disabled.
struct SourceParticleSphereSpawnCandidate {
    float distanceMin, distanceMax;
    SourceParticleVectorCandidate localSpeedMin, localSpeedMax;
};

struct SourceParticleSpawnPositionCandidate {
    SourceParticleVectorCandidate position, previousPosition;
};

inline SourceParticleSpawnPositionCandidate sourceParticleSphereSpawnCandidate(
        const SourceParticleSphereSpawnCandidate& spawn,
        const SourceParticleControlPointCandidate& point, const SourceParticleVectorCandidate& previousPoint,
        float now, float delta, float previousDelta, float born, bool writePrevious,
        SourceParticleRandomCandidate& random, const SourceParticleRandomTableCandidate& table) {
    const auto volume = sourceParticleSphereVolumeSampleCandidate(random, table);
    const auto& v = volume.offset;
    const float length = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    const float inverse = 1.f / (length + std::numeric_limits<float>::epsilon());
    const float distance = volume.radiusFraction * (spawn.distanceMax - spawn.distanceMin) + spawn.distanceMin;
    const float clampedBirth = std::max(now - delta, born);
    const float fraction = delta == 0.f ? 1.f : std::max(0.f, (delta - (now - clampedBirth)) / delta);
    SourceParticleSpawnPositionCandidate result{};
    for (size_t axis = 0; axis < v.size(); ++axis) {
        const float origin = delta == 0.f ? point.position[axis] :
            (point.position[axis] - previousPoint[axis]) * fraction + previousPoint[axis];
        result.position[axis] = (v[axis] * inverse) * distance + origin;
    }
    if (writePrevious) {
        const float forward = random.linear(table, spawn.localSpeedMin[0], spawn.localSpeedMax[0]);
        const float right = random.linear(table, spawn.localSpeedMin[1], spawn.localSpeedMax[1]);
        const float up = random.linear(table, spawn.localSpeedMin[2], spawn.localSpeedMax[2]);
        for (size_t axis = 0; axis < v.size(); ++axis) {
            float reverseSpeed = 0.f - point.forward[axis] * forward;
            reverseSpeed -= point.right[axis] * right;
            reverseSpeed -= point.up[axis] * up;
            result.previousPosition[axis] = reverseSpeed * previousDelta + result.position[axis];
        }
    }
    return result;
}

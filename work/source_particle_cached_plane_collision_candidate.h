#pragma once

#include "source_particle_sphere_spawn_candidate.h"

#include <cmath>
#include <xmmintrin.h>

struct SourceParticlePlaneTraceRequestCandidate {
    SourceParticleVectorCandidate start, end;
    uint32_t contentsMask, collisionGroup;
};

struct SourceParticlePlaneTraceHitCandidate {
    float fraction = 1.f;
    SourceParticleVectorCandidate normal{};
};

struct SourceParticleCollisionPlaneCandidate {
    SourceParticleVectorCandidate point{}, normal{};
};

struct SourceParticleCachedPlanesCandidate {
    float sampleTime = -1.f;
    SourceParticleVectorCandidate origin{};
    std::array<SourceParticleCollisionPlaneCandidate, 26> planes{};

    template<class Trace>
    void sample(float now, const SourceParticleVectorCandidate& controlPoint,
                const SourceParticleVectorCandidate& offset, float movementTolerance,
                uint32_t collisionGroup, Trace&& trace) {
        if (sampleTime == now) return;
        SourceParticleVectorCandidate start{}, movement{};
        for (size_t axis = 0; axis < start.size(); ++axis) {
            start[axis] = controlPoint[axis] + offset[axis];
            movement[axis] = start[axis] - origin[axis];
        }
        const float distanceSquared = (movement[0]*movement[0] + movement[1]*movement[1]) + movement[2]*movement[2];
        if (!(sampleTime > 0.f && distanceSquared < movementTolerance*movementTolerance)) {
            origin = start;
            // Mode 1 makes two passes over the same 26 grid directions.
            // The second pass clears the normal of a missed trace.
            for (int pass = 0; pass < 2; ++pass) {
                size_t index = 0;
                for (int x = -1; x <= 1; ++x)
                    for (int y = -1; y <= 1; ++y)
                        for (int z = -1; z <= 1; ++z) {
                            if (!(x || y || z)) continue;
                            const SourceParticleVectorCandidate delta{1000.f*x, 1000.f*y, 1000.f*z};
                            const auto hit = trace(SourceParticlePlaneTraceRequestCandidate{
                                start, {start[0]+delta[0], start[1]+delta[1], start[2]+delta[2]},
                                0x0200400bu, collisionGroup});
                            auto& plane = planes[index++];
                            if (hit.fraction < 1.f) {
                                for (size_t axis = 0; axis < delta.size(); ++axis)
                                    plane.point[axis] = start[axis] + delta[axis]*hit.fraction;
                                plane.normal = hit.normal;
                            } else if (pass == 1) {
                                plane.normal = {};
                            }
                        }
            }
        }
        sampleTime = now;
    }
};

struct SourceParticlePlaneResponseCandidate {
    float radiusScale, bounce, slide;
};

inline bool sourceParticleResolveCachedPlaneCandidate(SourceParticleSpawnPositionCandidate& particle,
        float radius, const std::array<SourceParticleCollisionPlaneCandidate, 26>& planes,
        const SourceParticlePlaneResponseCandidate& settings) {
    const auto dot = [](const auto& a, const auto& b) {
        return (a[0]*b[0] + a[1]*b[1]) + a[2]*b[2];
    };
    SourceParticleVectorCandidate velocity{};
    for (size_t axis = 0; axis < velocity.size(); ++axis)
        velocity[axis] = particle.position[axis] - particle.previousPosition[axis];
    const float lengthSquared = dot(velocity, velocity);
    if (!(lengthSquared > 0.f)) return false;
    const float inverseLength = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(lengthSquared)));
    const float extent = radius * settings.radiusScale;
    SourceParticleVectorCandidate direction{}, expandedEnd{};
    for (size_t axis = 0; axis < direction.size(); ++axis) {
        direction[axis] = velocity[axis] * inverseLength;
        expandedEnd[axis] = particle.position[axis] + direction[axis]*extent;
    }
    float fraction = 1.f;
    SourceParticleVectorCandidate normal{};
    for (const auto& plane : planes) {
        SourceParticleVectorCandidate start{}, end{};
        for (size_t axis = 0; axis < start.size(); ++axis) {
            start[axis] = particle.previousPosition[axis] - plane.point[axis];
            end[axis] = expandedEnd[axis] - plane.point[axis];
        }
        const float startDistance = dot(start, plane.normal);
        const float endDistance = dot(end, plane.normal);
        if (startDistance >= 0.f && endDistance < 0.f) {
            const float candidate = startDistance / (startDistance - endDistance);
            if (candidate < fraction) {
                fraction = candidate;
                normal = plane.normal;
            }
        }
    }
    if (!(fraction < 1.f)) return false;

    const float reflectedNormal = dot(direction, normal);
    const float tangentNormal = dot(velocity, normal);
    const float remaining = std::max(0.f, 1.f - fraction);
    for (size_t axis = 0; axis < velocity.size(); ++axis) {
        float contact = particle.previousPosition[axis] + velocity[axis]*fraction;
        if (settings.bounce != 0.f || settings.slide != 0.f) {
            const float bounce = (direction[axis] - normal[axis]*(reflectedNormal + reflectedNormal)) * settings.bounce;
            const float slide = (velocity[axis] - normal[axis]*tangentNormal) * settings.slide;
            const float residual = bounce + slide;
            contact = (contact + bounce*remaining) + slide*remaining;
            particle.previousPosition[axis] = contact - residual;
        }
        particle.position[axis] = contact;
    }
    return true;
}

#pragma once

#include "source_particle_sphere_spawn_candidate.h"
#include "source_particle_operator_random_candidate.h"

#include <xmmintrin.h>

struct SourceParticleBasicMotionCandidate {
    SourceParticleVectorCandidate gravity;
    float drag;
};

// Record the native drag exponent's float rounding between log, scale, and exp.
inline float sourceParticleMotionDampingCandidate(float drag, float delta, float previousDelta) {
    constexpr float maximumDrag = .9999998807907104f;
    constexpr float dragTimeScale = 29.999998092651367f;
    const float logDrag = std::log(1.f - std::clamp(drag, 0.f, maximumDrag));
    return std::exp((logDrag * dragTimeScale) * delta) * (delta / previousDelta);
}

struct SourceParticleMotionStepCandidate {
    float damping;
    SourceParticleVectorCandidate gravityDisplacement;
};

inline SourceParticleMotionStepCandidate sourceParticleMotionStepCandidate(
        const SourceParticleBasicMotionCandidate& movement, float delta, float previousDelta) {
    SourceParticleMotionStepCandidate step{sourceParticleMotionDampingCandidate(movement.drag, delta, previousDelta), {}};
    const float square = delta * delta;
    for (size_t axis = 0; axis < 3; ++axis)
        step.gravityDisplacement[axis] = movement.gravity[axis] * square;
    return step;
}

inline void sourceAdvanceParticleMotionCandidate(SourceParticleSpawnPositionCandidate& particle,
                                                   const SourceParticleMotionStepCandidate& step) {
    for (size_t axis = 0; axis < 3; ++axis) {
        const float position = particle.position[axis];
        const float displacement = (position - particle.previousPosition[axis]) * step.damping;
        particle.previousPosition[axis] = position;
        particle.position[axis] = (displacement + step.gravityDisplacement[axis]) + position;
    }
}

// Translation-only control point lock history. The operator context starts
// at zero and updates once per invocation after emission.
struct SourceParticleControlPointLockCandidate {
    SourceParticleVectorCandidate lastPoint{};

    SourceParticleVectorCandidate begin(const SourceParticleVectorCandidate& point, float strength) {
        if (lastPoint == SourceParticleVectorCandidate{}) lastPoint = point;
        SourceParticleVectorCandidate translation{};
        for (size_t axis = 0; axis < 3; ++axis)
            translation[axis] = (point[axis] - lastPoint[axis]) * strength;
        lastPoint = point;
        return translation;
    }
};

inline void sourceTranslateLockedParticleCandidate(SourceParticleSpawnPositionCandidate& particle,
        const SourceParticleVectorCandidate& translation, float age, float delta, float attachment = 1.f) {
    const float coverage = std::min(age, delta) * (1.f / delta);
    for (size_t axis = 0; axis < 3; ++axis) {
        const float offset = (translation[axis] * coverage) * attachment;
        particle.position[axis] += offset;
        particle.previousPosition[axis] += offset;
    }
}

struct SourceParticleLockFadeRangeCandidate {
    float minimum, maximum;
    unsigned quarterExponent;
};

struct SourceParticlePartialLockCandidate {
    SourceParticleLockFadeRangeCandidate start, end;
    float instantJumpThreshold;
};

inline float sourceParticleLockFadeSampleCandidate(const SourceParticleLockFadeRangeCandidate& range, float sample) {
    float value = 1.f;
    if (range.quarterExponent & 3u) {
        const float root = std::sqrt(sample);
        if (range.quarterExponent & 1u) value = std::sqrt(root);
        if (range.quarterExponent & 2u) value *= root;
    }
    auto power = range.quarterExponent >> 2;
    while (power) {
        if (power & 1u) value *= sample;
        power >>= 1;
        if (power) sample *= sample;
    }
    return value * (range.maximum - range.minimum) + range.minimum;
}

inline bool sourceParticleLockBypassesFadeCandidate(const SourceParticlePartialLockCandidate& settings,
        const SourceParticleVectorCandidate& displacement, float delta, float maximumStep) {
    const auto& d = displacement;
    const float jump = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    return delta == maximumStep || (settings.instantJumpThreshold != 0.f && jump > settings.instantJumpThreshold);
}

inline float sourceParticlePartialAttachmentCandidate(const SourceParticlePartialLockCandidate& settings,
        float age, float life, float startDraw, float endDraw, bool bypassFade) {
    if (bypassFade) return 1.f;
    const float reciprocal = _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(life)));
    const float phase = std::clamp(age * reciprocal, 0.f, 1.f);
    const float start = sourceParticleLockFadeSampleCandidate(settings.start, startDraw);
    const float end = sourceParticleLockFadeSampleCandidate(settings.end, endDraw);
    return 1.f - std::clamp((phase - start) / (end - start), 0.f, 1.f);
}

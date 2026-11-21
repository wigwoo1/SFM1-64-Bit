#pragma once

#include "source_particle_lifetime_candidate.h"

#include <cmath>

struct SourceParticleVectorOscillationCandidate {
    SourceParticleVectorCandidate rateMin{}, rateMax{};
    SourceParticleVectorCandidate frequencyMin{1.f, 1.f, 1.f}, frequencyMax{1.f, 1.f, 1.f};
    float startMin = 0.f, startMax = 0.f, endMin = 1.f, endMax = 1.f;
    float multiplier = 2.f, phase = .5f;
    bool normalizePhase = true, normalizeWindow = true, offsetHistory = false;
};

inline float sourceParticleOscillationWaveCandidate(float phase) {
    // Round down to an even integer before evaluating the signed parabola.
    // The floating-point bias and mask retain the original rounding at the fold.
    const float absolute = std::abs(phase);
    constexpr float bias = 0x1p23f;
    float rounded = absolute + bias;
    uint32_t bits;
    std::memcpy(&bits, &rounded, sizeof(bits));
    bits &= ~uint32_t{1};
    std::memcpy(&rounded, &bits, sizeof(rounded));
    rounded -= bias;
    const float wrapped = absolute - (absolute < rounded ? rounded - 2.f : rounded);
    const bool secondHalf = wrapped >= 1.f;
    const float x = wrapped - (secondHalf ? 1.f : 0.f);
    const float wave = (4.f - x * 4.f) * x;
    return secondHalf != std::signbit(phase) ? -wave : wave;
}

inline constexpr uint32_t sourceParticleOperatorSampleStrideCandidate = 17;

inline void sourceParticleOscillatePositionCandidate(SourceParticleLifeCandidate& particle,
        const SourceParticleVectorOscillationCandidate& settings, float now, float scaledDelta,
        const SourceParticleRandomTableCandidate& table, uint32_t collectionSeed, uint32_t operatorOffset) {
    if (!(particle.duration > 0.f)) return;
    const uint32_t key = particle.oscillatorId + collectionSeed + operatorOffset;
    const auto range = [&](uint32_t channel, float min, float max) {
        return table[(key + channel) & (table.size() - 1)] * (max - min) + min;
    };
    const float age = now - particle.born;
    const float normalizedAge = age * sourceParticleReciprocalCandidate(particle.duration);
    const float windowTime = settings.normalizeWindow ? normalizedAge : age;
    if (!(range(11, settings.startMin, settings.startMax) <= windowTime &&
          windowTime < range(12, settings.endMin, settings.endMax))) return;

    constexpr uint32_t rateChannel[]{3, 7, 9};
    constexpr uint32_t frequencyChannel[]{8, 12, 15};
    for (size_t axis = 0; axis < particle.motion.position.size(); ++axis) {
        const float rate = range(rateChannel[axis], settings.rateMin[axis], settings.rateMax[axis]);
        const float frequency = range(frequencyChannel[axis], settings.frequencyMin[axis], settings.frequencyMax[axis]);
        const float phase = settings.normalizePhase
            ? frequency * normalizedAge * settings.multiplier + settings.phase
            : frequency * (settings.multiplier * now + settings.phase);
        const float displacement = sourceParticleOscillationWaveCandidate(phase) * (rate * scaledDelta);
        particle.motion.position[axis] += displacement;
        if (settings.offsetHistory) particle.motion.previousPosition[axis] += displacement;
    }
}

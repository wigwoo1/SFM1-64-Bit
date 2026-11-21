#pragma once

#include "source_particle_lifetime_candidate.h"

struct SourceParticleRadiusScaleCandidate {
    float start, end, startScale, endScale, bias;
    bool ease;
};

struct SourceParticleColorFadeCandidate {
    float start, end;
    std::array<float, 3> target;
    bool ease;
};

inline float sourceParticleRefinedReciprocalCandidate(float value) {
    const float estimate = sourceParticleReciprocalCandidate(value);
    return (estimate + estimate) - (estimate * estimate) * value;
}

inline void sourceParticleScaleRadiusCandidate(SourceParticleLifeCandidate& particle, float now,
                                               const SourceParticleRadiusScaleCandidate& settings) {
    if (!(particle.duration > 0.f && settings.start < settings.end)) return;
    const float phase = (now - particle.born) * sourceParticleReciprocalCandidate(particle.duration);
    if (!(settings.start <= phase && phase < settings.end)) return;

    float weight = (phase - settings.start) * sourceParticleRefinedReciprocalCandidate(settings.end - settings.start);
    if (settings.ease) {
        weight = sourceParticleSmoothWeightCandidate(weight);
    } else if (settings.bias != 0.f && settings.bias != .5f) {
        const float bias = sourceParticleRefinedReciprocalCandidate(settings.bias) - 2.f;
        weight /= (1.f - weight) * bias + 1.f;
    }
    const float scale = weight * (settings.endScale - settings.startScale) + settings.startScale;
    particle.radius = scale * particle.appearance.radius;
}

inline void sourceParticleFadeColorCandidate(SourceParticleLifeCandidate& particle, float now,
                                             const SourceParticleColorFadeCandidate& settings) {
    if (!(particle.duration > 0.f) || settings.start == settings.end) return;
    const float phase = (now - particle.born) / particle.duration;
    const float inverseWindow = static_cast<float>(1.0 / (settings.end - settings.start));
    float weight = std::clamp((phase - settings.start) * inverseWindow, 0.f, 1.f);
    if (settings.ease) weight = sourceParticleSmoothWeightCandidate(weight);
    for (size_t channel = 0; channel < particle.tint.size(); ++channel) {
        const float initial = particle.appearance.tint[channel];
        particle.tint[channel] = (settings.target[channel] - initial) * weight + initial;
    }
}

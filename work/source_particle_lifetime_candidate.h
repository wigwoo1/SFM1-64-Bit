#pragma once

#include "source_particle_appearance_candidate.h"
#include "source_particle_sphere_spawn_candidate.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <xmmintrin.h>

struct SourceParticleLifeCandidate {
    uint32_t id;
    float born, duration, initialAlpha, alpha;
    int32_t sequence = 0;
    SourceParticleBirthAppearanceCandidate appearance{};
    SourceParticleSpawnPositionCandidate motion{};
};

struct SourceParticleAlphaFadeCandidate {
    float fadeInStart, fadeInEnd, fadeOutStart, fadeOutEnd, startAlpha, endAlpha;
};

inline float sourceParticleReciprocalCandidate(float number) {
    return _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(number)));
}

inline float sourceParticleFadeWeightCandidate(float phase, float start, float end) {
    const float fraction = std::clamp((phase - start) * sourceParticleReciprocalCandidate(end - start),
                                      0.f, 1.f);
    const float squared = fraction * fraction;
    return squared * 3.f - (fraction * 2.f) * squared;
}

inline bool sourceParticleFadeAndDecayCandidate(SourceParticleLifeCandidate& particle, float now,
                                                const SourceParticleAlphaFadeCandidate& fade) {
    const float age = now - particle.born;
    if (particle.duration <= age) return true;
    const float phase = age * sourceParticleReciprocalCandidate(particle.duration);
    if (fade.fadeInStart <= phase && phase < fade.fadeInEnd) {
        const float start = particle.initialAlpha * fade.startAlpha;
        const float weight = sourceParticleFadeWeightCandidate(phase, fade.fadeInStart, fade.fadeInEnd);
        particle.alpha = (particle.initialAlpha - start) * weight + start;
    }
    if (fade.fadeOutStart <= phase && phase < fade.fadeOutEnd) {
        const float end = particle.initialAlpha * fade.endAlpha;
        const float weight = sourceParticleFadeWeightCandidate(phase, fade.fadeOutStart, fade.fadeOutEnd);
        particle.alpha = (end - particle.initialAlpha) * weight + particle.initialAlpha;
    }
    return false;
}

inline bool sourceParticleLifespanExpiredCandidate(const SourceParticleLifeCandidate& particle, float now) {
    return particle.duration <= now - particle.born || particle.duration <= 0.f;
}

// Expiration operators produce ascending unique slot indices. The caller can
// preserve slot order for renderers that require it, or fill gaps from the tail.
inline void sourceCompactParticleLifeCandidate(std::vector<SourceParticleLifeCandidate>& particles,
                                               const std::vector<size_t>& expired, bool preserveOrder) {
    if (expired.empty()) return;
    const auto count = particles.size() - expired.size();
    if (preserveOrder) {
        size_t destination = expired.front();
        size_t dead = 0;
        for (size_t source = destination; source < particles.size(); ++source) {
            if (dead < expired.size() && source == expired[dead]) {
                ++dead;
            } else {
                particles[destination++] = particles[source];
            }
        }
    } else {
        size_t tail = particles.size();
        size_t dead = expired.size();
        for (const auto hole : expired) {
            if (hole >= count) break;
            --tail;
            while (dead && expired[dead - 1] == tail) {
                --dead;
                --tail;
            }
            particles[hole] = particles[tail];
        }
    }
    particles.resize(count);
}

inline size_t sourceExpireParticleLifeCandidate(std::vector<SourceParticleLifeCandidate>& particles,
                                               float now, const SourceParticleAlphaFadeCandidate* fade,
                                               bool preserveOrder) {
    std::vector<size_t> expired;
    for (size_t slot = 0; slot < particles.size(); ++slot) {
        auto& particle = particles[slot];
        if (fade ? sourceParticleFadeAndDecayCandidate(particle, now, *fade)
                 : sourceParticleLifespanExpiredCandidate(particle, now)) {
            expired.push_back(slot);
        }
    }
    sourceCompactParticleLifeCandidate(particles, expired, preserveOrder);
    return expired.size();
}

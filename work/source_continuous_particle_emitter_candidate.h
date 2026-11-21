#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>

struct SourceContinuousParticleEmissionCandidate {
    float rate = 100.f;
    float startSeconds = 0.f;
    float durationSeconds = 0.f;
};

struct SourceContinuousParticleEmitterCandidate {
    double due = 0.;
    int64_t accounted = 0;
    float originSeconds = 0.f;
    bool emitting = true;
};

struct SourceParticleBirthBatchCandidate {
    int32_t requested = 0, admitted = 0;
    float firstSeconds = 0.f, spacingSeconds = 0.f, lastAllowedSeconds = 0.f;

    template<class Sink>
    void visit(Sink&& sink) const {
        float next = firstSeconds;
        for (int32_t i = 0; i < admitted; ++i) {
            if (next > lastAllowedSeconds) next = lastAllowedSeconds;
            sink(next);
            next += spacingSeconds;
        }
    }
};

// The caller supplies the collection time after its substep and the free
// slots left after aging/killing particles. Rejected births are consumed by
// the accumulator; freeing slots later does not recover them.
inline SourceParticleBirthBatchCandidate sourceContinuousParticleBirthsCandidate(
        SourceContinuousParticleEmitterCandidate& state, const SourceContinuousParticleEmissionCandidate& settings,
        float currentSeconds, float deltaSeconds, float strength, int32_t freeSlots) {
    if (!std::isfinite(settings.rate) || settings.rate < 0.f || !std::isfinite(settings.startSeconds) ||
        !std::isfinite(settings.durationSeconds) || settings.durationSeconds < 0.f ||
        !std::isfinite(currentSeconds) || !std::isfinite(deltaSeconds) || deltaSeconds < 0.f ||
        !std::isfinite(strength) || strength < 0.f || freeSlots < 0)
        throw std::invalid_argument("invalid continuous particle emission inputs");
    const float start = state.originSeconds + settings.startSeconds;
    const float rate = settings.rate * strength;
    if (!state.emitting || currentSeconds < start || rate == 0.f) return {};

    double previous = currentSeconds - deltaSeconds;
    double end = currentSeconds;
    if (settings.durationSeconds != 0.f) {
        const float stop = start + settings.durationSeconds;
        if (previous > stop) return {};
        previous = std::max(previous, double(start));
        end = std::min(end, double(stop));
    }
    const double elapsed = std::min(end - previous, double(deltaSeconds));
    const double accumulated = state.due + double(rate) * elapsed;
    // Retain the original accumulator precision and the signed batch count.
    if (!std::isfinite(accumulated) || accumulated < 0. || accumulated >= 0x1p63)
        throw std::runtime_error("particle emission accumulator overflow");
    const auto accounted = static_cast<int64_t>(std::floor(accumulated));
    const auto requested = accounted - state.accounted;
    if (requested < 0 || requested > INT32_MAX) throw std::runtime_error("particle birth count overflow");
    state.due = accumulated;
    state.accounted = accounted;
    const auto admitted = std::min(static_cast<int32_t>(requested), freeSlots);
    if (!admitted) return {static_cast<int32_t>(requested)};
    const float spacing = static_cast<float>(elapsed / admitted);
    return {static_cast<int32_t>(requested), admitted, static_cast<float>(previous + double(spacing)),
            spacing, static_cast<float>(end)};
}

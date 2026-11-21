#pragma once

#include "source_particle_random_candidate.h"

#include <algorithm>
#include <cmath>
#include <variant>

struct SourceParticleRandomRangeCandidate {
    float minimum, maximum, exponent = 1.f;
};

inline float sourceParticleRandomRangeCandidate(const SourceParticleRandomRangeCandidate& range,
        SourceParticleRandomCandidate& random, const SourceParticleRandomTableCandidate& table) {
    const float draw = random.sample(table);
    const float fraction = range.exponent == 1.f ? draw : std::pow(draw, range.exponent);
    return fraction * (range.maximum - range.minimum) + range.minimum;
}

inline constexpr float sourceParticleByteScaleCandidate = 1.f / 255.f;

inline float sourceParticleRadiansCandidate(float degrees) {
    return static_cast<float>(static_cast<double>(degrees) * .017453292519943295);
}

inline float sourceParticleRandomRotationCandidate(const SourceParticleRandomRangeCandidate& offsets,
        float initialDegrees, SourceParticleRandomCandidate& random,
        const SourceParticleRandomTableCandidate& table) {
    const SourceParticleRandomRangeCandidate radians{
        sourceParticleRadiansCandidate(offsets.minimum), sourceParticleRadiansCandidate(offsets.maximum),
        offsets.exponent};
    return sourceParticleRandomRangeCandidate(radians, random, table) +
           sourceParticleRadiansCandidate(initialDegrees);
}

struct SourceParticleTintRangeCandidate {
    std::array<uint8_t, 4> first, second;
};

inline std::array<float, 3> sourceParticleRandomTintCandidate(const SourceParticleTintRangeCandidate& tint,
        SourceParticleRandomCandidate& random, const SourceParticleRandomTableCandidate& table) {
    const float draw = random.sample(table);
    std::array<float, 3> rgb{};
    for (size_t axis = 0; axis < rgb.size(); ++axis) {
        const float first = tint.first[axis] * sourceParticleByteScaleCandidate;
        const float second = tint.second[axis] * sourceParticleByteScaleCandidate;
        rgb[axis] = draw * (second - first) + first;
    }
    return rgb;
}

inline float sourceParticleYawFlipCandidate(float yaw, float probability,
        SourceParticleRandomCandidate& random, const SourceParticleRandomTableCandidate& table) {
    if (probability <= random.sample(table)) return yaw;
    constexpr double pi = 3.141592653589793;
    constexpr float modulus = static_cast<float>(pi);
    return static_cast<float>(std::fmod(static_cast<double>(yaw), static_cast<double>(modulus)) + pi);
}

struct SourceParticleInitializerFadeCandidate {
    float startIn = 0.f, endIn = 0.f, startOut = 0.f, endOut = 0.f;

    float strength(float time) const {
        if (time < startIn || (endOut > 0.f && time > endOut)) return 0.f;
        const float in = std::max(startIn, endIn);
        const float out = std::max(startOut, in);
        const float stop = std::max(endOut, out);
        if (time < in && in > startIn) return (time - startIn) / (in - startIn);
        if (time > out && stop > out) return (time - stop) / (out - stop);
        return 1.f;
    }
};

struct SourceParticleBirthAppearanceCandidate {
    float roll = 0.f, radius = 5.f, opacity = 1.f, trail = 0.f, yaw = 0.f;
    std::array<float, 3> tint{1.f, 1.f, 1.f};
};

struct SourceParticleScalarAppearanceCandidate {
    float SourceParticleBirthAppearanceCandidate::* field;
    SourceParticleRandomRangeCandidate range;
};
struct SourceParticleRollAppearanceCandidate {
    SourceParticleRandomRangeCandidate range;
    float initialDegrees;
};
struct SourceParticleTintAppearanceCandidate {
    SourceParticleTintRangeCandidate range;
    SourceParticleInitializerFadeCandidate fade;
};
using SourceParticleAppearanceInitializerCandidate = std::variant<SourceParticleScalarAppearanceCandidate,
    SourceParticleRollAppearanceCandidate, SourceParticleTintAppearanceCandidate>;

inline void applySourceParticleAppearanceCandidate(SourceParticleBirthAppearanceCandidate& particle,
        const SourceParticleAppearanceInitializerCandidate& initializer, SourceParticleRandomCandidate& random,
        const SourceParticleRandomTableCandidate& table) {
    if (const auto* scalar = std::get_if<SourceParticleScalarAppearanceCandidate>(&initializer))
        particle.*(scalar->field) = sourceParticleRandomRangeCandidate(scalar->range, random, table);
    else if (const auto* roll = std::get_if<SourceParticleRollAppearanceCandidate>(&initializer))
        particle.roll = sourceParticleRandomRotationCandidate(roll->range, roll->initialDegrees, random, table);
    else
        particle.tint = sourceParticleRandomTintCandidate(std::get<SourceParticleTintAppearanceCandidate>(
            initializer).range, random, table);
}

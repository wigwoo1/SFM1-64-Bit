#pragma once

#include "source_particle_appearance_candidate.h"
#include "source_particle_definition_candidate.h"

inline std::optional<SourceParticleAppearanceInitializerCandidate> readParticleAppearanceInitializerCandidate(
        const Graph& graph, const Element& node, const std::string& name) {
    const auto number = [&](const char* key, float fallback) {
        const auto* value = sourceParticleAttributeCandidate(graph, node, key, 3);
        return value ? wireFloat(value->values.front()) : fallback;
    };
    const auto known = [&](std::initializer_list<std::string_view> allowed) {
        for (const auto& attribute : node.attributes) {
            const auto& key = graph.strings.at(attribute.name);
            if (key != "functionName" && key.compare(0, 9, "operator ") != 0 &&
                std::find(allowed.begin(), allowed.end(), key) == allowed.end()) return false;
        }
        return true;
    };
    const auto range = [&](const char* min, const char* max, const char* exponent) {
        const auto missing = std::numeric_limits<float>::quiet_NaN();
        return SourceParticleRandomRangeCandidate{number(min, missing), number(max, missing), number(exponent, 1.f)};
    };
    const auto finite = [](const SourceParticleRandomRangeCandidate& limits) {
        return std::isfinite(limits.minimum) && std::isfinite(limits.maximum) &&
               std::isfinite(limits.exponent) && limits.exponent > 0.f;
    };
    if (name == "rotation random") {
        if (!known({"rotation_initial", "rotation_offset_min", "rotation_offset_max", "rotation_random_exponent"}))
            return {};
        const auto offsets = range("rotation_offset_min", "rotation_offset_max", "rotation_random_exponent");
        const auto initial = number("rotation_initial", 0.f);
        if (!finite(offsets) || !std::isfinite(initial)) return {};
        return SourceParticleRollAppearanceCandidate{offsets, initial};
    }
    if (name == "radius random") {
        if (!known({"radius_min", "radius_max", "radius_random_exponent"})) return {};
        const auto limits = range("radius_min", "radius_max", "radius_random_exponent");
        if (!finite(limits) || limits.minimum < 0.f || limits.maximum < 0.f) return {};
        return SourceParticleScalarAppearanceCandidate{&SourceParticleBirthAppearanceCandidate::radius, limits};
    }
    if (name == "trail length random") {
        if (!known({"length_min", "length_max", "length_random_exponent"})) return {};
        const auto limits = range("length_min", "length_max", "length_random_exponent");
        if (!finite(limits) || limits.minimum < 0.f || limits.maximum < 0.f) return {};
        return SourceParticleScalarAppearanceCandidate{&SourceParticleBirthAppearanceCandidate::trail, limits};
    }
    if (name == "alpha random") {
        if (!known({"alpha_min", "alpha_max", "alpha_random_exponent"})) return {};
        const auto* min = sourceParticleAttributeCandidate(graph, node, "alpha_min", 2);
        const auto* max = sourceParticleAttributeCandidate(graph, node, "alpha_max", 2);
        if (!min || !max) return {};
        const auto lo = wireInt(min->values.front()), hi = wireInt(max->values.front());
        const SourceParticleRandomRangeCandidate limits{static_cast<float>(lo) * sourceParticleByteScaleCandidate,
            static_cast<float>(hi) * sourceParticleByteScaleCandidate, number("alpha_random_exponent", 1.f)};
        if (!finite(limits) || lo < 0 || hi < 0 || lo > 255 || hi > 255) return {};
        return SourceParticleScalarAppearanceCandidate{&SourceParticleBirthAppearanceCandidate::opacity, limits};
    }
    if (name != "color random" || !known({"color1", "color2", "tint_perc", "tint control point",
            "tint clamp min", "tint clamp max", "tint update movement threshold"}) ||
        number("tint_perc", 0.f) != 0.f) return {};
    const auto* first = sourceParticleAttributeCandidate(graph, node, "color1", 8);
    const auto* second = sourceParticleAttributeCandidate(graph, node, "color2", 8);
    if (!first || !second || first->values.front().wire.size() != 4 || second->values.front().wire.size() != 4)
        return {};
    SourceParticleTintAppearanceCandidate tint{
        {}, {number("operator start fadein", 0.f), number("operator end fadein", 0.f),
             number("operator start fadeout", 0.f), number("operator end fadeout", 0.f)}};
    for (float time : {tint.fade.startIn, tint.fade.endIn, tint.fade.startOut, tint.fade.endOut})
        if (!std::isfinite(time)) return {};
    std::memcpy(tint.range.first.data(), first->values.front().wire.data(), 4);
    std::memcpy(tint.range.second.data(), second->values.front().wire.data(), 4);
    return tint;
}

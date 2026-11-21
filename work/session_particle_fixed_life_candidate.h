#pragma once

#include "source_particle_definition_candidate.h"
#include "source_particle_lifetime_candidate.h"
#include "source_particle_random_candidate.h"

struct SessionParticleLifeProgramCandidate {
    float minimum, maximum;
    std::optional<SourceParticleAlphaFadeCandidate> fade;
    std::optional<std::array<int32_t, 2>> sequence;
    float sequenceFps = 0.f;

    bool needsRandom() const { return minimum != maximum || sequence.has_value(); }
};

// Admit the linear Lifetime Random initializer and an optional following
// Sequence Random and optional sheet lifetime. Other attribute writers can change the
// collection's draw order and require their own initializer implementation.
inline std::optional<SessionParticleLifeProgramCandidate> sessionParticleLifeProgramCandidate(
        const Graph& graph, const Element& definition) {
    const auto number = [&](const Element& element, const char* key, float fallback) {
        const auto* found = sourceParticleAttributeCandidate(graph, element, key, 3);
        return found ? wireFloat(found->values.front()) : fallback;
    };
    const auto supported = [&](uint32_t reference) -> const Element* {
        if (reference >= graph.elements.size())
            return nullptr;
        const auto& node = graph.elements[reference];
        const auto* enabled = sourceParticleAttributeCandidate(graph, node, "operator enabled", 4);
        if (enabled && !enabled->values.front().wire.front()) return nullptr;
        for (const auto* key : {"operator start fadeout", "operator end fadeout", "operator start fadein",
                                "operator end fadein", "operator fade oscillate"})
            if (number(node, key, 0.f) != 0.f) return nullptr;
        for (const auto* key : {"operator strength random scale min", "operator strength random scale max",
                                "operator time scale min", "operator time scale max"})
            if (number(node, key, 1.f) != 1.f) return nullptr;
        return &node;
    };
    for (const auto* group : {"constraints", "forces", "renderers"}) {
        const auto* list = sourceParticleAttributeCandidate(graph, definition, group, 15);
        if (list && !list->values.empty()) return {};
    }
    const auto function = [&](const Element& node) {
        const auto* name = sourceParticleAttributeCandidate(graph, node, "functionName", 5);
        return name ? vmtAsciiLowerCandidate(name->values.front().text) : std::string{};
    };
    const auto* initializers = sourceParticleAttributeCandidate(graph, definition, "initializers", 15);
    const auto* operators = sourceParticleAttributeCandidate(graph, definition, "operators", 15);
    if (!initializers || initializers->values.empty() || initializers->values.size() > 3 ||
        !operators || operators->values.size() != 1) return {};
    const auto* initializer = supported(initializers->values.front().reference);
    const auto* operation = supported(operators->values.front().reference);
    if (!initializer || !operation || function(*initializer) != "lifetime random") return {};
    const float minimum = number(*initializer, "lifetime_min", 0.f);
    const float maximum = number(*initializer, "lifetime_max", 0.f);
    if (!std::isfinite(minimum) || !std::isfinite(maximum) || minimum < 0.f || maximum < 0.f ||
        number(*initializer, "lifetime_random_exponent", 1.f) != 1.f) return {};

    SessionParticleLifeProgramCandidate result{minimum, maximum, {}, {}};
    if (initializers->values.size() >= 2) {
        const auto* sequence = supported(initializers->values[1].reference);
        if (!sequence || function(*sequence) != "sequence random") return {};
        const auto* low = sourceParticleAttributeCandidate(graph, *sequence, "sequence_min", 2);
        const auto* high = sourceParticleAttributeCandidate(graph, *sequence, "sequence_max", 2);
        if (!low || !high) return {};
        const auto first = wireInt(low->values.front()), last = wireInt(high->values.front());
        if (first < 0 || last < first || last >= 64) return {};
        for (const auto& attr : sequence->attributes) {
            const auto& name = graph.strings.at(attr.name);
            if (name != "functionName" && name != "sequence_min" && name != "sequence_max" &&
                name.compare(0, 9, "operator ") != 0) return {};
        }
        result.sequence = std::array<int32_t, 2>{first, last};
    }
    if (initializers->values.size() == 3) {
        const auto* sequenceLifetime = supported(initializers->values[2].reference);
        if (!sequenceLifetime || function(*sequenceLifetime) != "lifetime from sequence") return {};
        result.sequenceFps = number(*sequenceLifetime, "Frames Per Second", 0.f);
        if (!std::isfinite(result.sequenceFps) || result.sequenceFps <= 0.f) return {};
    }
    const auto type = function(*operation);
    if (type == "lifespan decay") return result;
    if (type != "alpha fade and decay") return {};
    const auto read = [&](const char* key) {
        return number(*operation, key, std::numeric_limits<float>::quiet_NaN());
    };
    SourceParticleAlphaFadeCandidate fade{read("start_fade_in_time"), read("end_fade_in_time"),
        read("start_fade_out_time"), read("end_fade_out_time"), read("start_alpha"), read("end_alpha")};
    for (float value : {fade.fadeInStart, fade.fadeInEnd, fade.fadeOutStart, fade.fadeOutEnd,
                        fade.startAlpha, fade.endAlpha})
        if (!std::isfinite(value)) return {};
    result.fade = fade;
    return result;
}

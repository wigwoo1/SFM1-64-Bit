#pragma once

#include "source_particle_definition_candidate.h"
#include "source_particle_collection_start_candidate.h"
#include "session_particle_births_candidate.h"
#include "session_particle_control_points_candidate.h"

struct SessionParticleDefinitionCandidate {
    enum class State { Unavailable, Ambiguous, Embedded, Loaded, Invalid };
    uint32_t particle;
    State state = State::Unavailable;
    std::string name;
    int32_t seed = 0;
    float timeScale = 1.f;
    uint32_t definition = UINT32_MAX;
    std::shared_ptr<const SourceParticleConfigCandidate> config;
    std::optional<SourceParticleStartCandidate> startupAtSample;
    std::optional<uint64_t> controlPointBypassMask;
    std::vector<SessionParticleControlPointSampleCandidate> controlPoints;
    std::vector<SessionParticleBirthsCandidate> birthReplay;
};

inline SessionParticleDefinitionCandidate bindSessionParticleDefinitionCandidate(
        const Graph& graph, uint32_t particle, const SourceParticleCatalogCandidate& catalog) {
    SessionParticleDefinitionCandidate result{};
    result.particle = particle;
    const auto& element = graph.elements.at(particle);
    try {
        if (const auto* name = sourceParticleAttributeCandidate(graph, element, "particleSystemType", 5))
            result.name = name->values.front().text;
        if (const auto* seed = sourceParticleAttributeCandidate(graph, element, "randomSeed", 2))
            result.seed = wireInt(seed->values.front());
        if (const auto* scale = sourceParticleAttributeCandidate(graph, element, "simulationTimeScale", 3))
            result.timeScale = wireFloat(scale->values.front());
        const auto* embedded = sourceParticleAttributeCandidate(graph, element, "particleSystemDefinition", 1);
        if (embedded && embedded->values.front().reference != UINT32_MAX) {
            const auto id = embedded->values.front().reference;
            if (id < graph.elements.size() &&
                graph.strings.at(graph.elements[id].type) == "DmeParticleSystemDefinition") {
                result.state = SessionParticleDefinitionCandidate::State::Embedded;
                result.definition = id;
                result.name = graph.strings.at(graph.elements[id].name);
                return result;
            }
            // An unresolved DMX object reference leaves the named lookup
            // available. A resolved reference of the wrong type is invalid.
            if (id != UINT32_MAX - 1) throw std::runtime_error("invalid embedded particle definition");
        }
        if (const auto* named = catalog.find(result.name)) {
            result.state = SessionParticleDefinitionCandidate::State::Loaded;
            result.config = named->config;
            result.definition = named->element;
        } else if (catalog.ambiguous(result.name)) {
            result.state = SessionParticleDefinitionCandidate::State::Ambiguous;
        }
    } catch (const std::runtime_error&) {
        result.state = SessionParticleDefinitionCandidate::State::Invalid;
    }
    return result;
}

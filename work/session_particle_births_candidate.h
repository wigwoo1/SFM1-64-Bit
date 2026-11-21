#pragma once

#include "session_particle_start_candidate.h"
#include "source_continuous_particle_emitter_candidate.h"
#include "source_particle_collection_timing_candidate.h"
#include "source_particle_definition_candidate.h"

struct SessionParticleBirthsCandidate {
    std::string name;
    uint32_t definition;
    int64_t requested = 0, delivered = 0;
    float firstSeconds = 0.f, lastSeconds = 0.f;
    int32_t lastControlTick = 0;
    std::optional<std::array<float, 3>> emitterPosition;
};

// Replay an isolated emitter tree. The diagnostic sink drains each batch,
// so subsequent batches have their full capacity. A particle store can use
// the same emitter scheduler with the slots available after its operators.
inline std::vector<SessionParticleBirthsCandidate> replaySessionParticleBirthsCandidate(
        const Graph& graph, uint32_t root, const SourceParticleStartCandidate& startup,
        int32_t target, float timeScale, const SessionParticleSimulationCurveCandidate& simulation,
        const SessionParticleSimulationCurveCandidate& emission) {
    if (startup.skippedTicks || target < startup.cursor || timeScale < 0 || !std::isfinite(timeScale)) return {};
    struct Collection {
        SourceContinuousParticleEmissionCandidate emission;
        SourceContinuousParticleEmitterCandidate state;
        float clock, maximumStep;
        int32_t capacity;
    };
    std::vector<SessionParticleBirthsCandidate> result;
    std::vector<Collection> collections;
    std::set<uint32_t> active;
    const auto integer = [&](const Element& e, const char* name, int32_t fallback) {
        const auto* a = sourceParticleAttributeCandidate(graph, e, name, 2);
        return a ? wireInt(a->values.front()) : fallback;
    };
    const auto number = [&](const Element& e, const char* name, float fallback) {
        const auto* a = sourceParticleAttributeCandidate(graph, e, name, 3);
        return a ? wireFloat(a->values.front()) : fallback;
    };
    const auto flag = [&](const Element& e, const char* name, bool fallback) {
        const auto* a = sourceParticleAttributeCandidate(graph, e, name, 4);
        return a ? a->values.front().wire.front() != 0 : fallback;
    };
    const auto visit = [&](auto&& self, uint32_t id) -> bool {
        if (id >= graph.elements.size() || active.size() >= 64 || !active.insert(id).second) return false;
        const auto& def = graph.elements[id];
        if (graph.strings.at(def.type) != "DmeParticleSystemDefinition" ||
            integer(def, "initial_particles", 0) != 0 ||
            number(def, "maximum sim tick rate", 0.f) != 0.f ||
            number(def, "minimum sim tick rate", 0.f) != 0.f) return false;
        const auto* emitters = sourceParticleAttributeCandidate(graph, def, "emitters", 15);
        if (!emitters || emitters->values.size() != 1 || emitters->values[0].reference >= graph.elements.size())
            return false;
        const auto& emitter = graph.elements[emitters->values[0].reference];
        const auto* function = sourceParticleAttributeCandidate(graph, emitter, "functionName", 5);
        if (!function || vmtAsciiLowerCandidate(function->values.front().text) != "emit_continuously" ||
            !flag(emitter, "operator enabled", true) ||
            flag(emitter, "use parent particles for emission scaling", false) ||
            flag(emitter, "emit particles for killed parent particles", false) ||
            integer(emitter, "emission count scale control point", -1) >= 0 ||
            number(emitter, "scale emission to used control points", 0.f) != 0.f) return false;
        for (const auto* name : {"operator start fadein", "operator end fadein", "operator start fadeout",
                                 "operator end fadeout", "operator fade oscillate"})
            if (number(emitter, name, 0.f) != 0.f) return false;
        const float rate = number(emitter, "emission_rate", 100.f);
        const float start = number(emitter, "emission_start_time", 0.f);
        const float duration = number(emitter, "emission_duration", 0.f);
        const auto capacity = integer(def, "max_particles", 1000);
        const auto maximumStep = number(def, "maximum time step", .1f);
        if (!std::isfinite(rate) || rate < 0 || !std::isfinite(start) ||
            !std::isfinite(duration) || duration < 0 || capacity < 0 ||
            !std::isfinite(maximumStep) || maximumStep != .1f) return false;

        result.push_back({graph.strings.at(def.name), id});
        collections.push_back({{rate, start, duration}, {}, 0.f, maximumStep, std::min(capacity, 5000)});
        if (const auto* children = sourceParticleAttributeCandidate(graph, def, "children", 15))
            for (const auto& entry : children->values) {
                if (entry.reference >= graph.elements.size()) return false;
                const auto& child = graph.elements[entry.reference];
                if (graph.strings.at(child.type) != "DmeParticleChild" || number(child, "delay", 0.f) != 0.f)
                    return false;
                const auto* link = sourceParticleAttributeCandidate(graph, child, "child", 1);
                if (!link || !self(self, link->values[0].reference)) return false;
            }
        active.erase(id);
        return true;
    };
    if (!visit(visit, root)) return {};

    int32_t cursor = startup.cursor;
    bool emitting = true;
    for (;;) {
        const auto advance = sourceParticleCollectionStepCandidate(
            cursor, target, sourceParticleUnbudgetedStepCandidate, timeScale, emitting,
            [&](int32_t t) { return simulation.sample(t); }, [&](int32_t t) { return emission.sample(t); },
            [&](int32_t t) { return simulation.keyAt(t).value_or(INT32_MIN + 1); },
            [&](int32_t t) { return emission.keyAt(t).value_or(INT32_MIN + 1); });
        const bool wasEmitting = emitting;
        for (size_t step = 0; step < advance.commandCount; ++step) {
            const auto& command = advance.commands[step];
            if (command.kind == SourceParticleCommandKindCandidate::StartEmission) return {};
            if (command.kind == SourceParticleCommandKindCandidate::StopEmission) {
                emitting = false;
                for (auto& c : collections) c.state.emitting = false;
                continue;
            }
            for (size_t n = 0; n < collections.size(); ++n) {
                auto& c = collections[n];
                // This diagnostic admits the non-subdivided substep path.
                if (command.durationSeconds > c.maximumStep) return {};
                if (command.durationSeconds < 1.e-22) continue;
                c.clock += command.durationSeconds;
                const auto batch = sourceContinuousParticleBirthsCandidate(
                    c.state, c.emission, c.clock, command.durationSeconds, 1.f, c.capacity);
                auto& summary = result[n];
                summary.requested += batch.requested;
                batch.visit([&](float time) {
                    if (summary.delivered == 0) summary.firstSeconds = time;
                    ++summary.delivered;
                    summary.lastSeconds = time;
                    summary.lastControlTick = command.localTick;
                });
            }
        }
        if (advance.complete) return result;
        if (advance.cursor < cursor || (advance.cursor == cursor && wasEmitting == emitting)) return {};
        cursor = advance.cursor;
    }
}

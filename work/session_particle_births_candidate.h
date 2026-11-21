#pragma once

#include "session_particle_start_candidate.h"
#include "session_particle_fixed_life_candidate.h"
#include "source_continuous_particle_emitter_candidate.h"
#include "source_particle_collection_timing_candidate.h"
#include "source_particle_definition_candidate.h"
#include "source_particle_sheet_cache_candidate.h"

#include <functional>

struct SessionParticleLiveMotionCandidate {
    uint32_t id;
    SourceParticleSpawnPositionCandidate motion;
    std::optional<uint32_t> oscillatorId;
};

struct SessionParticleLiveStyleCandidate {
    uint32_t id;
    float radius;
    std::array<float, 3> tint;
};

struct SessionParticleBirthsCandidate {
    std::string name;
    uint32_t definition;
    int64_t requested = 0, delivered = 0;
    float firstSeconds = 0.f, lastSeconds = 0.f;
    int32_t lastControlTick = 0;
    std::optional<std::array<float, 3>> emitterPosition;
    std::optional<size_t> liveCount;
    int64_t expired = 0;
    std::optional<uint32_t> randomDraws;
    uint64_t birthAttributeHash = 14695981039346656037ull;
    bool lifetimeFromSheet = false;
    std::optional<SourceParticleSpawnPositionCandidate> lastSphereBirth;
    std::optional<SourceParticleBirthAppearanceCandidate> lastAppearance;
    int64_t skippedTintBirths = 0;
    std::optional<SessionParticleLiveMotionCandidate> sampledLiveMotion;
    std::optional<uint64_t> operatorRandomVectors;
    std::optional<SessionParticleLiveStyleCandidate> sampledLiveStyle;
    std::optional<size_t> collisionContacts;
};

using SessionParticlePointSamplerCandidate =
    std::function<std::optional<SourceParticleControlPointCandidate>(int32_t)>;
using SessionParticleTraceCandidate =
    std::function<SourceParticlePlaneTraceHitCandidate(const SourceParticlePlaneTraceRequestCandidate&)>;

// Replay an isolated emitter tree. Supported life programs retain particles
// across steps. Other definitions drain each diagnostic batch.
inline std::vector<SessionParticleBirthsCandidate> replaySessionParticleBirthsCandidate(
        const Graph& graph, uint32_t root, const SourceParticleStartCandidate& startup,
        int32_t target, float timeScale, const SessionParticleSimulationCurveCandidate& simulation,
        const SessionParticleSimulationCurveCandidate& emission, int32_t seed = 0,
        const SourceParticleRandomTableCandidate* randomTable = nullptr,
        SourceParticleSheetCacheCandidate* sheets = nullptr,
        const SessionParticlePointSamplerCandidate& pointAt = {}, const SessionParticleTraceCandidate& trace = {}) {
    if (startup.skippedTicks || target < startup.cursor || timeScale < 0 || !std::isfinite(timeScale) ||
        simulation.keys.empty() || emission.keys.empty()) return {};
    struct Collection {
        SourceContinuousParticleEmissionCandidate emission;
        SourceContinuousParticleEmitterCandidate state;
        float clock, maximumStep;
        int32_t capacity;
        std::optional<SessionParticleLifeProgramCandidate> lifetime;
        std::vector<SourceParticleLifeCandidate> live;
        SourceParticleRandomCandidate random;
        const VtfParticleSheetCandidate* sheet;
        std::optional<SourceParticleVectorCandidate> previousPoint;
        SourceParticleControlPointLockCandidate lock{};
        float previousDelta = .05f;
        size_t planeOwner = 0;
        SourceParticleCachedPlanesCandidate planes{};
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
    const auto visit = [&](auto&& self, uint32_t id, size_t planeOwner) -> bool {
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
        const auto capacity = integer(def, "max_particles", -1);
        const auto maximumStep = number(def, "maximum time step", .1f);
        if (!std::isfinite(rate) || rate < 0 || !std::isfinite(start) ||
            !std::isfinite(duration) || duration < 0 || capacity < 0 || capacity > 5000 ||
            !std::isfinite(maximumStep) || maximumStep != .1f) return false;

        auto lifetime = sessionParticleLifeProgramCandidate(graph, def, bool(trace));
        // The root's seed and draw cursor are verified. A child may offset
        // its draw cursor using definition data, so admit fixed children.
        const bool seeded = active.size() == 1 && seed != 0 && randomTable;
        if (lifetime && lifetime->needsRandom() && !seeded) lifetime.reset();
        std::optional<SourceParticleVectorCandidate> previousPoint;
        if (lifetime && lifetime->sphere) {
            const auto startPoint = pointAt ? pointAt(startup.cursor) : std::nullopt;
            if (startPoint) previousPoint = startPoint->position;
            else lifetime.reset();
        }
        const VtfParticleSheetCandidate* sheet = nullptr;
        if (lifetime && lifetime->sequenceFps != 0.f) {
            const auto* material = sourceParticleAttributeCandidate(graph, def, "material", 5);
            if (material && sheets) sheet = sheets->resolve(material->values.front().text);
            if (!sheet) lifetime.reset();
        }
        if (!lifetime) previousPoint.reset();
        result.push_back({graph.strings.at(def.name), id});
        if (lifetime) result.back().liveCount = 0;
        if (lifetime && lifetime->needsRandom()) result.back().randomDraws = 0;
        if (lifetime && lifetime->collision) result.back().collisionContacts = 0;
        result.back().lifetimeFromSheet = sheet != nullptr;
        const size_t current = collections.size();
        collections.push_back({{rate, start, duration}, {}, 0.f, maximumStep, capacity, lifetime, {},
                               {static_cast<uint32_t>(seed), 0}, sheet, previousPoint});
        collections.back().planeOwner = planeOwner;
        if (const auto* children = sourceParticleAttributeCandidate(graph, def, "children", 15))
            for (const auto& entry : children->values) {
                if (entry.reference >= graph.elements.size()) return false;
                const auto& child = graph.elements[entry.reference];
                if (graph.strings.at(child.type) != "DmeParticleChild" || number(child, "delay", 0.f) != 0.f)
                    return false;
                const auto* link = sourceParticleAttributeCandidate(graph, child, "child", 1);
                if (!link || !self(self, link->values[0].reference, current)) return false;
            }
        active.erase(id);
        return true;
    };
    if (!visit(visit, root, 0)) return {};

    // Each diagnostic replay owns a fresh serial manager stream. This scope
    // leaves out other particle instances and the engine's worker scheduling.
    std::optional<SourceParticleOperatorRandomCandidate> operatorRandom;
    if (std::any_of(collections.begin(), collections.end(), [](const auto& c) {
        return c.lifetime && c.lifetime->partialLock.has_value();
    })) operatorRandom.emplace(sourceParticleOperatorStartupSeedCandidate);

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
                const auto point = c.previousPoint ? pointAt(command.localTick) : std::nullopt;
                if (c.previousPoint && !point) return {};
                c.clock += command.durationSeconds;
                const auto batch = sourceContinuousParticleBirthsCandidate(
                    c.state, c.emission, c.clock, command.durationSeconds, 1.f,
                    c.capacity - static_cast<int32_t>(c.live.size()));
                auto& summary = result[n];
                summary.requested += batch.requested;
                const auto begin = c.live.size();
                batch.visit([&](float time) {
                    if (summary.delivered == 0) summary.firstSeconds = time;
                    if (c.lifetime)
                        c.live.push_back({static_cast<uint32_t>(summary.delivered), time,
                                          c.lifetime->minimum, 1.f, 1.f});
                    ++summary.delivered;
                    summary.lastSeconds = time;
                    summary.lastControlTick = command.localTick;
                });
                if (summary.randomDraws) {
                    // Finish each initializer across the batch before the next
                    // initializer draws any values from the collection.
                    for (size_t slot = begin; slot < c.live.size(); ++slot)
                        c.live[slot].duration = c.random.linear(
                            *randomTable, c.lifetime->minimum, c.lifetime->maximum);
                    if (point)
                        for (size_t slot = begin; slot < c.live.size(); ++slot) {
                            auto& particle = c.live[slot];
                            particle.motion = sourceParticleSphereSpawnCandidate(
                                *c.lifetime->sphere, *point, *c.previousPoint, c.clock,
                                command.durationSeconds, c.previousDelta, particle.born,
                                true, c.random, *randomTable);
                            summary.lastSphereBirth = particle.motion;
                        }
                    for (const auto& initializer : c.lifetime->appearance) {
                        const auto* tint = std::get_if<SourceParticleTintAppearanceCandidate>(&initializer);
                        if (tint && tint->fade.strength(c.clock) <= 0.f) {
                            summary.skippedTintBirths += batch.admitted;
                            continue;
                        }
                        for (size_t slot = begin; slot < c.live.size(); ++slot)
                            applySourceParticleAppearanceCandidate(c.live[slot].appearance, initializer,
                                c.random, *randomTable);
                    }
                    if (c.lifetime->sequence) {
                        const auto [first, last] = *c.lifetime->sequence;
                        for (size_t slot = begin; slot < c.live.size(); ++slot)
                            c.live[slot].sequence = c.random.sequence(*randomTable, first, last);
                    }
                    // Yaw flip modifies an existing attribute, and the native
                    // dispatcher runs such stages after the first writes.
                    if (c.lifetime->yawFlip)
                        for (size_t slot = begin; slot < c.live.size(); ++slot)
                            c.live[slot].appearance.yaw = sourceParticleYawFlipCandidate(
                                c.live[slot].appearance.yaw, *c.lifetime->yawFlip, c.random, *randomTable);
                    if (c.sheet)
                        for (size_t slot = begin; slot < c.live.size(); ++slot)
                            c.live[slot].duration = sourceParticleSheetLifetimeCandidate(
                                c.live[slot].duration, c.live[slot].sequence, c.lifetime->sequenceFps, c.sheet);
                    for (size_t slot = begin; slot < c.live.size(); ++slot) {
                        auto& particle = c.live[slot];
                        if (c.lifetime->hasAppearance()) {
                            particle.initialAlpha = particle.alpha = particle.appearance.opacity;
                            particle.radius = particle.appearance.radius;
                            particle.tint = particle.appearance.tint;
                            summary.lastAppearance = particle.appearance;
                        }
                        // Default ID initialization follows the authored stages
                        // and advances the seeded collection cursor once per birth.
                        if (c.lifetime->oscillator) particle.oscillatorId = c.random.nextIndex();
                        uint32_t lifeBits;
                        std::memcpy(&lifeBits, &c.live[slot].duration, sizeof(lifeBits));
                        for (const auto value : {lifeBits, static_cast<uint32_t>(c.live[slot].sequence)})
                            for (unsigned shift = 0; shift < 32; shift += 8)
                                summary.birthAttributeHash =
                                    (summary.birthAttributeHash ^ ((value >> shift) & 255u)) * 1099511628211ull;
                    }
                    summary.randomDraws = c.random.consumed;
                }
                // Native dispatch compacts the kill list after each operator.
                // An empty collection skips its remaining operators.
                if (c.lifetime) {
                    uint32_t operatorOffset = 0;
                    for (const auto update : c.lifetime->updates) {
                        if (c.live.empty()) break;
                        switch (update) {
                        case SessionParticleUpdateCandidate::Expire: {
                            const auto* fade = c.lifetime->fade ? &*c.lifetime->fade : nullptr;
                            summary.expired += sourceExpireParticleLifeCandidate(c.live, c.clock, fade, false);
                            break;
                        }
                        case SessionParticleUpdateCandidate::BasicMovement: {
                            const auto motion = sourceParticleMotionStepCandidate(
                                *c.lifetime->movement, command.durationSeconds, c.previousDelta);
                            for (auto& particle : c.live)
                                sourceAdvanceParticleMotionCandidate(particle.motion, motion);
                            // Basic Movement dispatches the single admitted
                            // constraint before the next authored operator.
                            if (const auto& collision = c.lifetime->collision) {
                                auto& planes = collections[c.planeOwner].planes;
                                planes.sample(c.clock, point->position, collision->offset,
                                              collision->movementTolerance, collision->group, trace);
                                for (auto& particle : c.live)
                                    *summary.collisionContacts += sourceParticleResolveCachedPlaneCandidate(
                                        particle.motion, particle.radius, planes.planes, collision->response);
                            }
                            break;
                        }
                        case SessionParticleUpdateCandidate::ScaleRadius:
                            for (auto& particle : c.live)
                                sourceParticleScaleRadiusCandidate(particle, c.clock, *c.lifetime->radiusScale);
                            break;
                        case SessionParticleUpdateCandidate::FadeColor:
                            for (auto& particle : c.live)
                                sourceParticleFadeColorCandidate(particle, c.clock, *c.lifetime->colorFade);
                            break;
                        case SessionParticleUpdateCandidate::OscillatePosition:
                            for (auto& particle : c.live)
                                sourceParticleOscillatePositionCandidate(particle, *c.lifetime->oscillator, c.clock,
                                    command.durationSeconds, *randomTable, c.random.seed, operatorOffset);
                            break;
                        case SessionParticleUpdateCandidate::ControlPointLock: {
                            const auto translation = c.lock.begin(point->position, 1.f);
                            if (const auto& partial = c.lifetime->partialLock) {
                                const bool bypass = sourceParticleLockBypassesFadeCandidate(
                                    *partial, translation, command.durationSeconds, c.maximumStep);
                                for (size_t groupStart = 0; groupStart < c.live.size(); groupStart += 4) {
                                    const auto start = operatorRandom->next();
                                    const auto end = operatorRandom->next();
                                    const auto lanes = std::min<size_t>(4, c.live.size() - groupStart);
                                    for (size_t lane = 0; lane < lanes; ++lane) {
                                        auto& particle = c.live[groupStart+lane];
                                        const float age = c.clock - particle.born;
                                        const float attachment = sourceParticlePartialAttachmentCandidate(
                                            *partial, age, particle.duration, start[lane], end[lane], bypass);
                                        if (attachment > 0.f)
                                            sourceTranslateLockedParticleCandidate(particle.motion, translation,
                                                age, command.durationSeconds, attachment);
                                    }
                                }
                                summary.operatorRandomVectors = operatorRandom->draws();
                            } else {
                                for (auto& particle : c.live)
                                    sourceTranslateLockedParticleCandidate(particle.motion, translation,
                                        c.clock - particle.born, command.durationSeconds);
                            }
                            break;
                        }
                        }
                        operatorOffset += sourceParticleOperatorSampleStrideCandidate;
                    }
                    summary.liveCount = c.live.size();
                    summary.sampledLiveMotion.reset();
                    summary.sampledLiveStyle.reset();
                    const auto& updates = c.lifetime->updates;
                    const bool hasMotion = c.lifetime->movement || c.lifetime->oscillator || std::find(updates.begin(), updates.end(),
                        SessionParticleUpdateCandidate::ControlPointLock) != updates.end();
                    const bool hasStyle = c.lifetime->radiusScale || c.lifetime->colorFade;
                    if ((hasMotion || hasStyle) && !c.live.empty()) {
                        // Sample the oldest survivor when testing a fade so
                        // the record covers time spent partially attached.
                        const auto& particle = c.lifetime->partialLock
                            ? *std::min_element(c.live.begin(), c.live.end(), [](const auto& a, const auto& b) {
                                return a.born < b.born;
                            })
                            : c.live.back();
                        if (hasMotion)
                            summary.sampledLiveMotion = SessionParticleLiveMotionCandidate{particle.id, particle.motion,
                                c.lifetime->oscillator ? std::optional{particle.oscillatorId} : std::nullopt};
                        if (hasStyle)
                            summary.sampledLiveStyle = SessionParticleLiveStyleCandidate{
                                particle.id, particle.radius, particle.tint};
                    }
                }
                if (point) {
                    c.previousPoint = point->position;
                    c.previousDelta = command.durationSeconds;
                }
            }
        }
        if (advance.complete) return result;
        if (advance.cursor < cursor || (advance.cursor == cursor && wasEmitting == emitting)) return {};
        cursor = advance.cursor;
    }
}

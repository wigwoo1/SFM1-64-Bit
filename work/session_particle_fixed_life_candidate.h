#pragma once

#include "session_particle_appearance_candidate.h"
#include "source_particle_lifetime_candidate.h"
#include "source_particle_motion_candidate.h"

enum class SessionParticleUpdateCandidate { Expire, BasicMovement, ControlPointLock };

struct SessionParticleLifeProgramCandidate {
    float minimum = 0.f, maximum = 0.f;
    std::optional<SourceParticleAlphaFadeCandidate> fade;
    std::optional<std::array<int32_t, 2>> sequence;
    std::optional<SourceParticleSphereSpawnCandidate> sphere;
    std::vector<SourceParticleAppearanceInitializerCandidate> appearance;
    std::optional<float> yawFlip;
    float sequenceFps = 0.f;
    std::optional<SourceParticleBasicMotionCandidate> movement;
    std::optional<SourceParticlePartialLockCandidate> partialLock;
    std::vector<SessionParticleUpdateCandidate> updates;

    bool hasAppearance() const { return !appearance.empty() || yawFlip.has_value(); }
    bool needsRandom() const { return minimum != maximum || sequence || sphere || hasAppearance(); }
};

// Admit the supported stages in their authored order. Other attribute
// writers can change the collection's draw order.
inline std::optional<SessionParticleLifeProgramCandidate> sessionParticleLifeProgramCandidate(
        const Graph& graph, const Element& definition) {
    const auto number = [&](const Element& element, const char* key, float fallback) {
        const auto* found = sourceParticleAttributeCandidate(graph, element, key, 3);
        return found ? wireFloat(found->values.front()) : fallback;
    };
    const auto supported = [&](uint32_t reference, bool allowEnvelope = false) -> const Element* {
        if (reference >= graph.elements.size())
            return nullptr;
        const auto& node = graph.elements[reference];
        const auto* enabled = sourceParticleAttributeCandidate(graph, node, "operator enabled", 4);
        if (enabled && !enabled->values.front().wire.front()) return nullptr;
        if (number(node, "operator fade oscillate", 0.f) != 0.f) return nullptr;
        if (!allowEnvelope)
            for (const auto* key : {"operator start fadeout", "operator end fadeout",
                                    "operator start fadein", "operator end fadein"})
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
    if (!initializers || initializers->values.empty() || initializers->values.size() > 10 ||
        !operators || operators->values.empty() || operators->values.size() > 3) return {};
    const auto* initializer = supported(initializers->values.front().reference);
    if (!initializer || function(*initializer) != "lifetime random") return {};
    const float minimum = number(*initializer, "lifetime_min", 0.f);
    const float maximum = number(*initializer, "lifetime_max", 0.f);
    if (!std::isfinite(minimum) || !std::isfinite(maximum) || minimum < 0.f || maximum < 0.f ||
        number(*initializer, "lifetime_random_exponent", 1.f) != 1.f) return {};

    SessionParticleLifeProgramCandidate result;
    result.minimum = minimum;
    result.maximum = maximum;
    size_t next = 1;
    if (next < initializers->values.size()) {
        const auto* sphere = supported(initializers->values[next].reference, true);
        if (!sphere) return {};
        if (function(*sphere) == "position within sphere random") {
            if (!supported(initializers->values[next].reference)) return {};
            const auto vector = [&](const char* key, SourceParticleVectorCandidate fallback) {
                const auto* field = sourceParticleAttributeCandidate(graph, *sphere, key, 10);
                if (!field) return fallback;
                const auto& v = field->values.front();
                return SourceParticleVectorCandidate{wireFloat(v, 0), wireFloat(v, 1), wireFloat(v, 2)};
            };
            const auto intValue = [&](const char* key, int32_t fallback) {
                const auto* field = sourceParticleAttributeCandidate(graph, *sphere, key, 2);
                return field ? wireInt(field->values.front()) : fallback;
            };
            const auto flag = [&](const char* key) {
                const auto* field = sourceParticleAttributeCandidate(graph, *sphere, key, 4);
                return field && field->values.front().wire.front() != 0;
            };
            if (intValue("control_point_number", 0) != 0 || intValue("create in model", 0) != 0 ||
                flag("bias in local system") || flag("randomly distribute to highest supplied Control Point") ||
                number(*sphere, "speed_min", 0.f) != 0.f || number(*sphere, "speed_max", 0.f) != 0.f ||
                vector("distance_bias", {1.f, 1.f, 1.f}) != SourceParticleVectorCandidate{1.f, 1.f, 1.f} ||
                vector("distance_bias_absolute_value", {}) != SourceParticleVectorCandidate{}) return {};
            static constexpr std::array<std::string_view, 15> fields{
                "functionName", "control_point_number", "create in model", "bias in local system",
                "randomly distribute to highest supplied Control Point",
                "randomly distribution growth time", "distance_min", "distance_max", "distance_bias",
                "distance_bias_absolute_value", "speed_min", "speed_max", "speed_random_exponent",
                "speed_in_local_coordinate_system_min", "speed_in_local_coordinate_system_max"
            };
            for (const auto& attribute : sphere->attributes) {
                const auto& name = graph.strings.at(attribute.name);
                if (std::find(fields.begin(), fields.end(), name) == fields.end() &&
                    name.compare(0, 9, "operator ") != 0) return {};
            }
            SourceParticleSphereSpawnCandidate spawn{
                number(*sphere, "distance_min", 0.f), number(*sphere, "distance_max", 0.f),
                vector("speed_in_local_coordinate_system_min", {}),
                vector("speed_in_local_coordinate_system_max", {})
            };
            if (!std::isfinite(spawn.distanceMin) || !std::isfinite(spawn.distanceMax) ||
                !std::all_of(spawn.localSpeedMin.begin(), spawn.localSpeedMin.end(),
                             [](float speed) { return std::isfinite(speed); }) ||
                !std::all_of(spawn.localSpeedMax.begin(), spawn.localSpeedMax.end(),
                             [](float speed) { return std::isfinite(speed); })) return {};
            result.sphere = spawn;
            ++next;
        }
    }
    std::set<std::string> appeared;
    while (next < initializers->values.size()) {
        const auto reference = initializers->values[next].reference;
        if (reference >= graph.elements.size()) return {};
        const auto name = function(graph.elements[reference]);
        if (name == "sequence random") break;
        const auto* stage = supported(reference, name == "color random");
        if (!stage || !appeared.insert(name).second) return {};
        if (name == "rotation yaw flip random") {
            for (const auto& attribute : stage->attributes) {
                const auto& key = graph.strings.at(attribute.name);
                if (key != "functionName" && key != "Flip Percentage" &&
                    key.compare(0, 9, "operator ") != 0) return {};
            }
            const auto probability = number(*stage, "Flip Percentage", -1.f);
            if (!(probability >= 0.f && probability <= 1.f)) return {};
            result.yawFlip = probability;
        } else {
            const auto appearance = readParticleAppearanceInitializerCandidate(graph, *stage, name);
            if (!appearance) return {};
            result.appearance.push_back(*appearance);
        }
        ++next;
    }
    if (result.hasAppearance()) {
        const auto* color = sourceParticleAttributeCandidate(graph, definition, "color", 8);
        if (color && (color->values.front().wire.size() != 4 ||
                !std::all_of(color->values.front().wire.begin(), color->values.front().wire.end(),
                             [](unsigned char value) { return value == 255; }))) return {};
        if (number(definition, "radius", 5.f) != 5.f || number(definition, "rotation", 0.f) != 0.f)
            return {};
    }
    if (next < initializers->values.size()) {
        const auto* sequence = supported(initializers->values[next++].reference);
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
    if (next < initializers->values.size()) {
        const auto* sequenceLifetime = supported(initializers->values[next++].reference);
        if (!sequenceLifetime || function(*sequenceLifetime) != "lifetime from sequence") return {};
        result.sequenceFps = number(*sequenceLifetime, "Frames Per Second", 0.f);
        if (!std::isfinite(result.sequenceFps) || result.sequenceFps <= 0.f) return {};
    }
    if (next != initializers->values.size()) return {};
    bool expires = false, locked = false;
    for (const auto& value : operators->values) {
        const auto* operation = supported(value.reference);
        if (!operation) return {};
        const auto type = function(*operation);
        if (type == "movement basic") {
            if (!result.sphere || result.movement) return {};
            const auto* gravity = sourceParticleAttributeCandidate(graph, *operation, "gravity", 10);
            SourceParticleBasicMotionCandidate movement{{}, number(*operation, "drag", 0.f)};
            if (gravity)
                for (size_t axis = 0; axis < 3; ++axis)
                    movement.gravity[axis] = wireFloat(gravity->values.front(), axis);
            if (!std::isfinite(movement.drag) ||
                !std::all_of(movement.gravity.begin(), movement.gravity.end(), [](float v) { return std::isfinite(v); }))
                return {};
            result.movement = movement;
            result.updates.push_back(SessionParticleUpdateCandidate::BasicMovement);
        } else if (type == "movement lock to control point") {
            if (!result.sphere || locked || number(*operation, "distance fade range", 0.f) != 0.f)
                return {};
            const auto* index = sourceParticleAttributeCandidate(graph, *operation, "control_point_number", 2);
            const auto* rotation = sourceParticleAttributeCandidate(graph, *operation, "lock rotation", 4);
            if ((index && wireInt(index->values.front()) != 0) ||
                (rotation && rotation->values.front().wire.front() != 0)) return {};
            const auto startMin = number(*operation, "start_fadeout_min", 1.f);
            const auto startMax = number(*operation, "start_fadeout_max", 1.f);
            const auto endMin = number(*operation, "end_fadeout_min", 1.f);
            const auto endMax = number(*operation, "end_fadeout_max", 1.f);
            if (!(0.f <= startMin && startMin <= startMax && startMax <= endMin &&
                  endMin <= endMax && endMax <= 1.f)) return {};
            if (startMin != 1.f) {
                const auto startExponent = number(*operation, "start_fadeout_exponent", 1.f);
                const auto endExponent = number(*operation, "end_fadeout_exponent", 1.f);
                const auto jump = number(*operation, "instant jump threshold", 512.f);
                if (!(0.f <= startExponent && startExponent <= 8.f &&
                      0.f <= endExponent && endExponent <= 8.f && std::isfinite(jump) && jump >= 0.f)) return {};
                SourceParticlePartialLockCandidate settings{
                    {startMin, startMax, static_cast<unsigned>(startExponent * 4.f)},
                    {endMin, endMax, static_cast<unsigned>(endExponent * 4.f)}, jump};
                // The stream's largest value is one step below one. Reject
                // windows that can round to a zero denominator.
                if (sourceParticleLockFadeSampleCandidate(settings.start, 65535.f / 65536.f) >= endMin) return {};
                result.partialLock = settings;
            }
            locked = true;
            result.updates.push_back(SessionParticleUpdateCandidate::ControlPointLock);
        } else {
            if (expires || (type != "lifespan decay" && type != "alpha fade and decay")) return {};
            if (type == "alpha fade and decay") {
                const auto read = [&](const char* key) {
                    return number(*operation, key, std::numeric_limits<float>::quiet_NaN());
                };
                SourceParticleAlphaFadeCandidate fade{read("start_fade_in_time"), read("end_fade_in_time"),
                    read("start_fade_out_time"), read("end_fade_out_time"), read("start_alpha"), read("end_alpha")};
                for (float v : {fade.fadeInStart, fade.fadeInEnd, fade.fadeOutStart, fade.fadeOutEnd,
                                fade.startAlpha, fade.endAlpha})
                    if (!std::isfinite(v)) return {};
                result.fade = fade;
            }
            expires = true;
            result.updates.push_back(SessionParticleUpdateCandidate::Expire);
        }
    }
    return expires ? std::optional{std::move(result)} : std::nullopt;
}

#pragma once

#include "channel_binding.h"
#include "channel_clip_time.h"
#include "graph_ik_parent_frame.h"
#include "graph_quaternion_key_sampler.h"
#include "graph_vector_key_sampler.h"
#include "source_particle_control_point_candidate.h"

struct SessionParticleControlPointSampleCandidate {
    uint32_t index, transform, model;
    SourceParticleControlPointCandidate current;
    std::optional<SourceParticleControlPointCandidate> startup;
};

class SessionParticleControlPointBindingsCandidate {
    struct Slot { uint32_t transform, model; };
    struct PoseTrack {
        uint32_t owner = UINT32_MAX;
        std::optional<uint32_t> clipFrame;
        std::optional<AnimationTimeFrame> clock;
        std::optional<VectorKeySampler> position;
        std::optional<QuaternionKeySampler> orientation;
        bool supported = true;
    };
    std::map<uint32_t, std::vector<Slot>> particles_;
    std::map<uint32_t, PoseTrack> tracks_;

public:
    SessionParticleControlPointBindingsCandidate(
            const Graph& graph, uint32_t shot, uint32_t root, const std::vector<uint32_t>& particles) {
        for (auto id : particles) {
            const auto& element = graph.elements.at(id);
            const auto* transforms = namedAttribute(graph, element, "controlPoints");
            if (!transforms || transforms->type != 15 || transforms->values.size() > 64) continue;
            const auto* models = namedAttribute(graph, element, "controlModels");
            auto& slots = particles_[id];
            slots.reserve(transforms->values.size());
            for (size_t index = 0; index < transforms->values.size(); ++index) {
                auto transform = transforms->values[index].reference;
                if (transform >= graph.elements.size() ||
                    graph.strings.at(graph.elements[transform].type) != "DmeTransform")
                    transform = UINT32_MAX;
                auto model = models && models->type == 15 && index < models->values.size()
                    ? models->values[index].reference : UINT32_MAX;
                if (model >= graph.elements.size() ||
                    graph.strings.at(graph.elements[model].type) != "DmeGameModel")
                    model = UINT32_MAX;
                slots.push_back({transform, model});
                if (transform != UINT32_MAX) tracks_.try_emplace(transform);
            }
        }
        if (tracks_.empty()) return;
        const GraphTransformHierarchy hierarchy(graph, root);
        const auto bindOwner = [&](uint32_t owner) {
            const auto transform = hierarchySingleReference(graph, graph.elements.at(owner), "transform");
            const auto found = tracks_.find(transform);
            if (found == tracks_.end()) return;
            if (found->second.owner != UINT32_MAX) found->second.supported = false;
            found->second.owner = owner;
        };
        bindOwner(root);
        for (const auto& entry : hierarchy.ordinaryParents) bindOwner(entry.first);

        for (const auto& [id, paths] : channelTimePaths(graph, shot)) {
            const auto& element = graph.elements.at(id);
            const auto transform = hierarchySingleReference(graph, element, "toElement");
            const auto found = tracks_.find(transform);
            if (found == tracks_.end() || !found->second.supported) continue;
            auto& track = found->second;
            const auto* attribute = namedAttribute(graph, element, "toAttribute");
            if (!attribute || attribute->type != 5 || attribute->values.size() != 1) continue;
            const auto& name = attribute->values.front().text;
            if (name != "position" && name != "orientation") continue;
            try {
                const ChannelBinding binding(graph, id);
                if (binding.mode != 3) continue;
                if (binding.layers.size() != 1 || paths.size() != 1 || paths.front().size() != 2)
                    throw std::runtime_error("unsupported particle control-point channel");
                const auto frame = paths.front().back();
                if (track.clipFrame && track.clipFrame != frame)
                    throw std::runtime_error("particle control-point channels use different clips");
                track.clipFrame = frame;
                track.clock.emplace(graph, graph.elements.at(frame));
                if (track.clock->duration < 0 || !std::isfinite(track.clock->scale) || track.clock->scale <= 0.f)
                    throw std::runtime_error("invalid particle control-point clip clock");
                if (name == "position" && !track.position) {
                    track.position = bindGraphVectorKeySampler(graph, binding.log, binding.layers[0]);
                } else if (name == "orientation" && !track.orientation) {
                    track.orientation = bindGraphQuaternionKeySampler(graph, binding.log, binding.layers[0]);
                } else {
                    throw std::runtime_error("ambiguous particle control-point channel");
                }
            } catch (const std::runtime_error&) {
                track.supported = false;
            }
        }
    }

    std::vector<SessionParticleControlPointSampleCandidate> sample(
            GraphTransformHierarchy& hierarchy, uint32_t particle, uint64_t bypassParentMask,
            int32_t target, std::optional<int32_t> startup) {
        std::vector<SessionParticleControlPointSampleCandidate> result;
        const auto found = particles_.find(particle);
        if (found == particles_.end()) return result;
        const auto& graph = hierarchy.graph;
        for (uint32_t index = 0; index < found->second.size(); ++index) {
            const auto& slot = found->second[index];
            if (slot.transform == UINT32_MAX) continue;
            auto& track = tracks_.at(slot.transform);
            if (!track.supported) continue;
            const auto& element = graph.elements.at(slot.transform);
            const auto* position = namedAttribute(graph, element, "position");
            const auto* orientation = namedAttribute(graph, element, "orientation");
            if (!position || position->type != 10 || position->values.size() != 1 ||
                position->values.front().wire.size() != 12 ||
                !orientation || orientation->type != 13 || orientation->values.size() != 1 ||
                orientation->values.front().wire.size() != 16)
                throw std::runtime_error("invalid particle control-point transform");
            std::array<float, 3> fallbackPosition;
            std::array<float, 4> fallbackOrientation;
            std::memcpy(fallbackPosition.data(), position->values.front().wire.data(), 12);
            std::memcpy(fallbackOrientation.data(), orientation->values.front().wire.data(), 16);

            const auto parent = track.owner == UINT32_MAX ? GraphTransformHierarchy::identity() :
                graphIkParentFrameCandidate(graph, hierarchy, track.owner);
            const bool bypassParent = (bypassParentMask & (uint64_t{1} << index)) != 0;
            const auto at = [&](int32_t shotTick) {
                const auto tick = track.clock ? track.clock->childTime(shotTick, true) : shotTick;
                return sourceParticleControlPointCandidate(
                    track.position ? track.position->sample(tick) : fallbackPosition,
                    track.orientation ? track.orientation->sample(tick) : fallbackOrientation,
                    parent, bypassParent);
            };
            // The native update freezes the parent at the evaluated frame,
            // then samples the local logs at each simulation step's time.
            SessionParticleControlPointSampleCandidate sample{index, slot.transform, slot.model, at(target), {}};
            if (startup) sample.startup = at(*startup);
            result.push_back(std::move(sample));
        }
        return result;
    }
};

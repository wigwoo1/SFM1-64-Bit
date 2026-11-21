#pragma once

#include "animation_keys.h"
#include "channel_binding.h"
#include "channel_clip_time.h"
#include "source_particle_collection_start_candidate.h"
#include <limits>

struct SessionParticleSimulationKeyCandidate { int32_t tick; bool simulating; };
struct SessionParticleSimulationCurveCandidate {
    AnimationTimeFrame frame;
    std::vector<SessionParticleSimulationKeyCandidate> keys;

    auto upperKey(int32_t shotTick) const {
        return std::upper_bound(keys.begin(), keys.end(), frame.childTime(shotTick, true),
            [](int32_t time, const SessionParticleSimulationKeyCandidate& key) { return time < key.tick; });
    }
    bool sample(int32_t shotTick) const {
        const auto next = upperKey(shotTick);
        return (next == keys.begin() ? keys.begin() : next - 1)->simulating;
    }
    std::optional<int32_t> keyAt(int32_t shotTick) const {
        const auto next = upperKey(shotTick);
        if (next == keys.begin()) return {};
        return frame.parentTime((next - 1)->tick, true);
    }
};
using SessionParticleSimulationKeysCandidate =
    std::map<uint32_t, SessionParticleSimulationCurveCandidate>;

// For a channels clip directly under the shot, the clip's parent time is
// already the collection clock. The engine finds keys in the log clock
// and converts the chosen key back to the clip's parent with clamping.
inline SessionParticleSimulationKeysCandidate readSessionParticleSimulationKeysCandidate(
        const Graph& graph, uint32_t shot, const std::vector<uint32_t>& particles,
        const char* attribute = "simulating") {
    const std::set<uint32_t> selected(particles.begin(), particles.end());
    std::set<uint32_t> seen;
    SessionParticleSimulationKeysCandidate result;
    for (const auto& [channel, paths] : channelTimePaths(graph, shot)) {
        const auto& element = graph.elements.at(channel);
        const auto particle = hierarchySingleReference(graph, element, "toElement");
        if (!selected.count(particle)) continue;
        const auto destination = resolveEndpoint(graph, element, "to");
        if (destination.attribute != attribute) continue;
        if (!seen.insert(particle).second) {
            result.erase(particle);
            continue;
        }
        const auto binding = ChannelBinding(graph, channel);
        if (paths.size() != 1 || paths[0].size() != 2 || binding.mode != 3 || binding.layers.size() != 1 ||
            binding.destination.scalarType != 4 || binding.log == UINT32_MAX) continue;
        const auto& log = graph.elements.at(binding.log);
        if (graph.strings.at(log.type) != "DmeBoolLog" || !hierarchyRefs(graph, log, "curveinfo", 1).empty())
            continue;
        const auto& layer = graph.elements.at(binding.layers[0]);
        if (graph.strings.at(layer.type) != "DmeBoolLogLayer") continue;
        const AnimationTimeFrame frame(graph, graph.elements.at(paths[0].back()));
        if (frame.duration < 0 || !std::isfinite(frame.scale) || frame.scale <= 0.f) continue;
        const KeyTrack track(graph, layer);
        if (track.values->type != 18) continue;
        std::vector<SessionParticleSimulationKeyCandidate> keys;
        keys.reserve(track.times->values.size());
        for (size_t n = 0; n < track.times->values.size(); ++n) {
            const auto& value = track.values->values[n].wire;
            if (value.size() != 1) throw std::runtime_error("invalid particle simulation key");
            const auto tick = wireInt(track.times->values[n]);
            if (tick <= (std::numeric_limits<int32_t>::min)() + 1 ||
                tick >= (std::numeric_limits<int32_t>::max)())
                throw std::runtime_error("particle simulation key outside shot clock");
            keys.push_back({tick, value.front() != 0});
        }
        if (!keys.empty()) result.emplace(particle, SessionParticleSimulationCurveCandidate{frame, std::move(keys)});
    }
    return result;
}

inline std::optional<SourceParticleStartCandidate> sessionParticleStartupCandidate(
        const SessionParticleSimulationKeysCandidate& curves, uint32_t particle, int32_t target,
        float simulationScale) {
    const auto found = curves.find(particle);
    if (found == curves.end() || found->second.keys.empty()) return {};
    const auto& curve = found->second;
    return sourceParticleCollectionStartCandidate(
        target, curve.keyAt(target), curve.frame.parentTime(curve.keys.front().tick, true),
        sourceParticleUnbudgetedHistoryCandidate, simulationScale, [&](int32_t tick) { return curve.sample(tick); });
}

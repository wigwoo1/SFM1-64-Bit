#pragma once
#include "retained_film_track_candidate.h"
#include "session_timeline_frames_candidate.h"
#include "session_timeline_segment.h"

inline std::vector<SfmEditorTimelineLaneCandidate> sessionTimelineTrackLanesCandidate(
    const Graph& graph, uint32_t root, const SessionTimelineFramesCandidate& frames) {
    const auto& rootElement = graph.elements.at(root);
    if (graph.strings.at(rootElement.type) != "DmeFilmClip")
        throw std::runtime_error("timeline lane root is not a DmeFilmClip");
    const auto rootFrameRefs = hierarchyRefs(graph, rootElement, "timeFrame", 1);
    if (rootFrameRefs.size() != 1) throw std::runtime_error("timeline root time frame missing");
    const AnimationTimeFrame rootFrame(graph, graph.elements.at(rootFrameRefs.front()));

    std::vector<uint32_t> groups;
    const auto special = hierarchyRefs(graph, rootElement, "subClipTrackGroup", 1);
    if (special.size() > 1) throw std::runtime_error("subClipTrackGroup is not scalar");
    if (!special.empty()) groups.push_back(special.front());
    const auto regular = hierarchyRefs(graph, rootElement, "trackGroups", 15);
    groups.insert(groups.end(), regular.begin(), regular.end());

    auto scalarBoolean = [&](const Element& element, const char* name, bool fallback) {
        const auto* value = namedAttribute(graph, element, name);
        if (!value) return fallback;
        if (value->type != 4 || value->values.size() != 1 || value->values.front().wire.size() != 1)
            throw std::runtime_error(std::string("invalid track lane boolean: ") + name);
        return value->values.front().wire.front() != 0;
    };
    std::vector<SfmEditorTimelineLaneCandidate> lanes;
    for (const auto groupId : groups) {
        const auto& group = graph.elements.at(groupId);
        if (graph.strings.at(group.type) != "DmeTrackGroup")
            throw std::runtime_error("film clip track group reference is not DmeTrackGroup");
        const auto trackIds = hierarchyRefs(graph, group, "tracks", 15);
        for (const auto trackId : trackIds) {
            const auto& track = graph.elements.at(trackId);
            if (graph.strings.at(track.type) != "DmeTrack")
                throw std::runtime_error("track group reference is not DmeTrack");
            const auto* kind = namedAttribute(graph, track, "clipType");
            if (!kind || kind->type != 2 || kind->values.size() != 1)
                throw std::runtime_error("track clipType is not scalar integer");
            const int32_t clipType = wireInt(kind->values.front());
            if (clipType < 0 || clipType > 3) throw std::runtime_error("unsupported timeline clip type");
            struct Clip { uint32_t id; AnimationTimeFrame frame; };
            std::vector<Clip> clips;
            for (const auto childId : hierarchyRefs(graph, track, "children", 15)) {
                const auto& child = graph.elements.at(childId);
                const auto frameRefs = hierarchyRefs(graph, child, "timeFrame", 1);
                if (frameRefs.size() != 1) continue;
                AnimationTimeFrame clipFrame(graph, graph.elements.at(frameRefs.front()));
                const int64_t end = int64_t(clipFrame.start) + clipFrame.duration;
                if (end > INT32_MAX || end < INT32_MIN || !std::isfinite(clipFrame.scale))
                    throw std::runtime_error("invalid timeline clip time frame");
                if (clipFrame.duration <= 0 || clipFrame.scale == 0) continue;
                clips.push_back({childId, clipFrame});
            }
            std::stable_sort(clips.begin(), clips.end(), [](const Clip& a, const Clip& b) {
                return a.frame.start < b.frame.start;
            });
            SfmEditorTimelineLaneCandidate lane;
            lane.group = groupId;
            lane.track = trackId;
            lane.groupName = graph.strings.at(group.name);
            lane.trackName = graph.strings.at(track.name);
            lane.clipType = uint32_t(clipType);
            lane.visible = scalarBoolean(group, "visible", true);
            uint32_t current = UINT32_MAX;
            size_t start = 0;
            size_t candidate = 0;
            uint32_t active = UINT32_MAX;
            int64_t activeEnd = INT64_MIN;
            auto finish = [&](size_t end) {
                if (current != UINT32_MAX && start < end)
                    lane.segments.push_back({start, end, current,
                        graph.strings.at(graph.elements.at(current).name)});
            };
            for (size_t frameIndex = 0; frameIndex < frames.size(); ++frameIndex) {
                const int32_t trackTick = rootFrame.childTime(frames.tick(frameIndex), false);
                while (candidate < clips.size() && clips[candidate].frame.start <= trackTick) {
                    active = clips[candidate].id;
                    activeEnd = int64_t(clips[candidate].frame.start) + clips[candidate].frame.duration;
                    ++candidate;
                }
                const uint32_t atFrame = active != UINT32_MAX && trackTick < activeEnd ? active : UINT32_MAX;
                if (atFrame == current) continue;
                finish(frameIndex);
                current = atFrame;
                start = frameIndex;
            }
            finish(frames.size());
            lanes.push_back(std::move(lane));
        }
    }
    return lanes;
}

inline std::vector<SfmEditorTimelineSegmentCandidate> sessionTimelineLaneCandidate(
    const Graph& graph, uint32_t root, const SessionTimelineFramesCandidate& frames) {
    RetainedFilmTrackCandidate track(graph, root);
    std::vector<SfmEditorTimelineSegmentCandidate> segments;
    uint32_t current = 0xffffffffu;
    size_t start = 0;
    auto finish = [&](size_t end) {
        if (current == 0xffffffffu || start >= end) return;
        segments.push_back({start, end, current, graph.strings.at(graph.elements.at(current).name)});
    };
    for (size_t frame = 0; frame < frames.size(); ++frame) {
        auto shot = track.select(frames.tick(frame));
        const uint32_t element = shot ? shot->clip : 0xffffffffu;
        if (element == current) continue;
        finish(frame);
        current = element;
        start = frame;
    }
    finish(frames.size());
    return segments;
}

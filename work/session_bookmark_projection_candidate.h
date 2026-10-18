#pragma once
#include "session_timeline_frames_candidate.h"

inline std::vector<size_t> sessionBookmarkFramesCandidate(
    const Graph& graph, uint32_t root, const SessionTimelineFramesCandidate& frames) {
    if (root >= graph.elements.size() || graph.strings.at(graph.elements[root].type) != "DmeFilmClip")
        throw std::runtime_error("bookmark projection root is not a film clip");
    const auto timeFrames = hierarchyRefs(graph, graph.elements[root], "timeFrame", 1);
    if (timeFrames.size() != 1) throw std::runtime_error("bookmark projection root time frame missing");
    const AnimationTimeFrame rootFrame(graph, graph.elements.at(timeFrames.front()));
    if (!frames.size()) return {};
    const int32_t firstTick = rootFrame.childTime(frames.tick(0), false);
    const auto sets = hierarchyRefs(graph, graph.elements[root], "bookmarkSets", 15);
    std::vector<int32_t> bookmarkTicks;
    for (const auto setIndex : sets) {
        if (setIndex >= graph.elements.size() || graph.strings.at(graph.elements[setIndex].type) != "DmeBookmarkSet")
            continue;
        for (const auto bookmarkIndex : hierarchyRefs(graph, graph.elements[setIndex], "bookmarks", 15)) {
            if (bookmarkIndex >= graph.elements.size() || graph.strings.at(graph.elements[bookmarkIndex].type) != "DmeBookmark")
                continue;
            const auto* time = namedAttribute(graph, graph.elements[bookmarkIndex], "time");
            if (!time || time->type != 7 || time->values.size() != 1) continue;
            bookmarkTicks.push_back(wireInt(time->values.front()));
        }
    }
    std::vector<size_t> projected;
    projected.reserve(bookmarkTicks.size());
    for (const int32_t tick : bookmarkTicks) {
        if (tick < firstTick) continue;
        size_t low = 0, high = frames.size();
        while (low < high) {
            const size_t middle = low + (high - low) / 2;
            if (rootFrame.childTime(frames.tick(middle), false) < tick) low = middle + 1;
            else high = middle;
        }
        if (low == frames.size()) continue;
        size_t nearest = low;
        if (low) {
            const int64_t after = int64_t(rootFrame.childTime(frames.tick(low), false)) - tick;
            const int64_t before = int64_t(tick) - rootFrame.childTime(frames.tick(low - 1), false);
            if (before <= after) nearest = low - 1;
        }
        projected.push_back(nearest);
    }
    std::sort(projected.begin(), projected.end());
    projected.erase(std::unique(projected.begin(), projected.end()), projected.end());
    return projected;
}

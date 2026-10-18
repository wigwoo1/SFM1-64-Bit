#pragma once
#include "session_timeline_document_candidate.h"

template<class Publish>
inline bool addBookmarkAtPlayheadCandidate(SessionTimelineDocumentCandidate& document,
                                          int32_t rootParentTick, Publish publish) {
    const auto& graph = document.graph();
    const uint32_t root = sessionRenderRootCandidate(graph);
    const auto& clip = graph.elements.at(root);
    if (graph.strings.at(clip.type) != "DmeFilmClip")
        throw std::runtime_error("Add Bookmark at Playhead requires a root film clip");
    const auto timeFrames = hierarchyRefs(graph, clip, "timeFrame", 1);
    if (timeFrames.size() != 1) throw std::runtime_error("session root time frame is unavailable");
    const int32_t clipLocalTick = AnimationTimeFrame(graph, graph.elements.at(timeFrames.front())).childTime(rootParentTick, false);
    const Attribute* sets = nullptr;
    for (const auto& attribute : clip.attributes)
        if (graph.strings.at(attribute.name) == "bookmarkSets") { sets = &attribute; break; }
    if (!sets || (sets->type != 1 && sets->type != 15))
        throw std::runtime_error("The active clip has no bookmark-set references");

    uint32_t selectedSet = UINT32_MAX;
    for (const auto& value : sets->values) {
        if (value.reference >= graph.elements.size() || graph.strings.at(graph.elements[value.reference].type) != "DmeBookmarkSet")
            continue;
        if (selectedSet == UINT32_MAX) selectedSet = value.reference;
        if (graph.strings.at(graph.elements[value.reference].name) == "default set") {
            selectedSet = value.reference;
            break;
        }
    }
    if (selectedSet == UINT32_MAX) throw std::runtime_error("The active clip has no valid bookmark set");

    return document.transact(
        [&](SessionTimelineDocumentCandidate& staged) { return staged.addBookmarkAtTime(selectedSet, clipLocalTick); },
        [&](const Graph& staged) { publish(staged, root); });
}

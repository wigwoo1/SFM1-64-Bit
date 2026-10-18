#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_timeline_document_candidate.h"
#include "session_timeline_lane_candidate.h"
#include <iostream>

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("usage: test_session_track_clip_trim input.dmx output.dmx");
        const auto graph = loadGraph(argv[1]);
        const auto root = sessionRenderRootCandidate(graph);
        const auto frames = sessionTimelineFramesCandidate(graph, root);
        const auto lanes = sessionTimelineTrackLanesCandidate(graph, root, frames);
        auto selectedLane = lanes.end();
        auto selectedSegment = std::vector<SfmEditorTimelineSegmentCandidate>::const_iterator{};
        for (auto lane = lanes.begin(); lane != lanes.end() && selectedLane == lanes.end(); ++lane) {
            if (lane->clipType != 1 && lane->clipType != 2) continue;
            auto segment = std::find_if(lane->segments.begin(), lane->segments.end(), [](const auto& value) {
                return value.endFrame - value.startFrame >= 2;
            });
            if (segment != lane->segments.end()) { selectedLane = lane; selectedSegment = segment; }
        }
        if (selectedLane == lanes.end()) throw std::runtime_error("benchmark has no editable Sound/overlay segment");

        const uint32_t clip = selectedSegment->element;
        const auto frameRefs = hierarchyRefs(graph, graph.elements.at(clip), "timeFrame", 1);
        if (frameRefs.size() != 1) throw std::runtime_error("selected track clip has no unique timeframe");
        const AnimationTimeFrame before(graph, graph.elements.at(frameRefs.front()));
        const AnimationTimeFrame rootFrame(graph, graph.elements.at(
            hierarchyRefs(graph, graph.elements.at(root), "timeFrame", 1).at(0)));
        const size_t trimFrame = selectedSegment->startFrame + 1;
        const int32_t trackTick = rootFrame.childTime(frames.tick(trimFrame), false);
        if (trackTick <= before.start || trackTick >= int64_t(before.start) + before.duration)
            throw std::runtime_error("chosen benchmark frame does not fall inside clip trim range");

        SessionTimelineDocumentCandidate document(graph);
        document.trimClipStart(clip, trackTick);
        auto readFrame = [&](const Graph& source) {
            const auto refs = hierarchyRefs(source, source.elements.at(clip), "timeFrame", 1);
            if (refs.size() != 1) throw std::runtime_error("trimmed clip timeframe missing");
            return AnimationTimeFrame(source, source.elements.at(refs.front()));
        };
        auto afterStart = readFrame(document.graph());
        if (afterStart.start != trackTick || afterStart.duration != before.duration - (trackTick - before.start) ||
            afterStart.offset != before.offset + (trackTick - before.start))
            throw std::runtime_error("Sound/overlay trim-start fields incorrect");
        if (!document.undo()) throw std::runtime_error("trim-start undo missing");
        auto undone = readFrame(document.graph());
        if (undone.start != before.start || undone.duration != before.duration || undone.offset != before.offset)
            throw std::runtime_error("trim-start undo did not restore timeframe");
        if (!document.redo()) throw std::runtime_error("trim-start redo missing");
        if (!document.trimClipEnd(clip, trackTick + 1)) throw std::runtime_error("trim-end made no change");
        const auto afterEnd = readFrame(document.graph());
        if (afterEnd.duration != 1) throw std::runtime_error("trim-end duration incorrect");

        document.saveAs(argv[2]);
        const auto reloaded = loadGraph(argv[2]);
        const auto savedFrameRefs = hierarchyRefs(reloaded, reloaded.elements.at(clip), "timeFrame", 1);
        const AnimationTimeFrame saved(reloaded, reloaded.elements.at(savedFrameRefs.at(0)));
        if (saved.start != afterEnd.start || saved.duration != afterEnd.duration || saved.offset != afterEnd.offset)
            throw std::runtime_error("saved track clip trim did not round-trip");

        const auto overlayLane = std::find_if(lanes.begin(), lanes.end(), [](const auto& lane) { return lane.clipType == 2; });
        if (overlayLane == lanes.end() || overlayLane->segments.empty()) throw std::runtime_error("benchmark overlay clip missing");
        const uint32_t overlayClip = overlayLane->segments.front().element;
        const auto overlayRefs = hierarchyRefs(graph, graph.elements.at(overlayClip), "timeFrame", 1);
        if (overlayRefs.size() != 1) throw std::runtime_error("overlay timeframe missing");
        const AnimationTimeFrame overlayBefore(graph, graph.elements.at(overlayRefs.front()));
        const int64_t overlayOldEnd = int64_t(overlayBefore.start) + overlayBefore.duration;
        int64_t overlayNewEnd = int64_t(rootFrame.childTime(frames.tick(overlayLane->segments.front().startFrame), false)) + 1;
        if (overlayNewEnd <= overlayBefore.start) overlayNewEnd = int64_t(overlayBefore.start) + 1;
        if (overlayNewEnd >= overlayOldEnd) overlayNewEnd = overlayOldEnd - 1;
        if (overlayNewEnd <= overlayBefore.start || overlayNewEnd >= overlayOldEnd)
            throw std::runtime_error("benchmark overlay clip has no trim-able range");
        SessionTimelineDocumentCandidate overlayDocument(graph);
        if (!overlayDocument.trimClipEnd(overlayClip, int32_t(overlayNewEnd)))
            throw std::runtime_error("overlay trim-end made no change");
        auto overlayAfter = [&]() {
            const auto refs = hierarchyRefs(overlayDocument.graph(), overlayDocument.graph().elements.at(overlayClip), "timeFrame", 1);
            return AnimationTimeFrame(overlayDocument.graph(), overlayDocument.graph().elements.at(refs.at(0)));
        }();
        if (int64_t(overlayAfter.start) + overlayAfter.duration != overlayNewEnd)
            throw std::runtime_error("overlay trim-end fields incorrect");
        if (!overlayDocument.undo() || !overlayDocument.redo()) throw std::runtime_error("overlay undo/redo failed");

        SessionTimelineDocumentCandidate deletionDocument(graph);
        const uint32_t track = selectedLane->track;
        auto containsClip = [&](const Graph& source) {
            for (const auto& attribute : source.elements.at(track).attributes)
                if (source.strings.at(attribute.name) == "children")
                    return std::any_of(attribute.values.begin(), attribute.values.end(),
                        [&](const Value& value) { return value.reference == clip; });
            return false;
        };
        if (!deletionDocument.removeTrackClip(clip) || containsClip(deletionDocument.graph()) || !deletionDocument.dirty())
            throw std::runtime_error("track clip deletion did not detach reference / mark dirty");
        if (!deletionDocument.undo() || !containsClip(deletionDocument.graph()) || deletionDocument.dirty())
            throw std::runtime_error("track clip deletion undo failed");
        if (!deletionDocument.redo() || containsClip(deletionDocument.graph()) || !deletionDocument.dirty())
            throw std::runtime_error("track clip deletion redo failed");
        const std::string deletedPath = std::string(argv[2]) + ".deleted.dmx";
        deletionDocument.saveAs(deletedPath.c_str());
        const auto deleted = loadGraph(deletedPath.c_str());
        if (containsClip(deleted)) throw std::runtime_error("deleted track clip reference returned after save/reload");
        const auto deletedRoot = sessionRenderRootCandidate(deleted);
        const auto deletedFrames = sessionTimelineFramesCandidate(deleted, deletedRoot);
        const auto deletedLanes = sessionTimelineTrackLanesCandidate(deleted, deletedRoot, deletedFrames);
        if (std::any_of(deletedLanes.begin(), deletedLanes.end(), [&](const auto& lane) {
                return std::any_of(lane.segments.begin(), lane.segments.end(),
                    [&](const auto& segment) { return segment.element == clip; });
            })) throw std::runtime_error("deleted clip still appears in rebuilt timeline");

        SessionTimelineDocumentCandidate movingDocument(graph);
        uint32_t movedClip = UINT32_MAX;int32_t movedTo = 0,movedBeforeOffset = 0,movedBeforeDuration = 0;
        const int32_t domainStart = rootFrame.childTime(frames.tick(0), false);
        const int32_t domainEnd = rootFrame.childTime(frames.tick(frames.size() - 1), false) + 1;
        for (const auto& lane : lanes) {
            if (lane.clipType != 1) continue;
            const auto childIds = hierarchyRefs(graph, graph.elements.at(lane.track), "children", 15);
            for (uint32_t candidate : childIds) {
                const auto refs = hierarchyRefs(graph, graph.elements.at(candidate), "timeFrame", 1);
                if (refs.size() != 1) continue;
                const AnimationTimeFrame candidateBefore(graph, graph.elements.at(refs.front()));
                if (candidateBefore.duration <= 0) continue;
                bool foundMove = false;
                for (size_t frame = 0; frame < frames.size() && !foundMove; ++frame) {
                    const int32_t destination = rootFrame.childTime(frames.tick(frame), false);
                    const int64_t destinationEnd = int64_t(destination) + candidateBefore.duration;
                    if (destination == candidateBefore.start || destination < domainStart || destinationEnd > domainEnd) continue;
                    bool collision = false;
                    for (uint32_t sibling : childIds) {
                        if (sibling == candidate) continue;
                        const auto siblingRefs = hierarchyRefs(graph, graph.elements.at(sibling), "timeFrame", 1);
                        if (siblingRefs.size() != 1) continue;
                        const AnimationTimeFrame siblingFrame(graph, graph.elements.at(siblingRefs.front()));
                        if (siblingFrame.duration > 0 && int64_t(destination) < int64_t(siblingFrame.start) + siblingFrame.duration &&
                            int64_t(siblingFrame.start) < destinationEnd) { collision = true; break; }
                    }
                    if (!collision) { movedClip = candidate; movedTo = destination;movedBeforeDuration=candidateBefore.duration;movedBeforeOffset=candidateBefore.offset;foundMove = true; }
                }
                if (foundMove) break;
            }
            if (movedClip != UINT32_MAX) break;
        }
        if (movedClip == UINT32_MAX) throw std::runtime_error("benchmark has no non-overlapping Sound clip move destination");
        if (!movingDocument.moveTrackClip(movedClip, movedTo) || !movingDocument.dirty())
            throw std::runtime_error("track clip move failed to apply / mark dirty");
        const auto movedRefs = hierarchyRefs(movingDocument.graph(), movingDocument.graph().elements.at(movedClip), "timeFrame", 1);
        const AnimationTimeFrame moved(movingDocument.graph(), movingDocument.graph().elements.at(movedRefs.at(0)));
        if (moved.start != movedTo || moved.duration != movedBeforeDuration || moved.offset != movedBeforeOffset)
            throw std::runtime_error("track clip move changed more than the start time");
        if (!movingDocument.undo() || movingDocument.dirty()) throw std::runtime_error("track clip move undo failed");
        if (!movingDocument.redo() || !movingDocument.dirty()) throw std::runtime_error("track clip move redo failed");
        const std::string movedPath = std::string(argv[2]) + ".moved.dmx";
        movingDocument.saveAs(movedPath.c_str());
        const auto movedReloaded = loadGraph(movedPath.c_str());
        const auto movedReloadedRefs = hierarchyRefs(movedReloaded, movedReloaded.elements.at(movedClip), "timeFrame", 1);
        const AnimationTimeFrame movedSaved(movedReloaded, movedReloaded.elements.at(movedReloadedRefs.at(0)));
        if (movedSaved.start != movedTo || movedSaved.duration != movedBeforeDuration || movedSaved.offset != movedBeforeOffset)
            throw std::runtime_error("track clip move did not round-trip through DMX");
        auto movedLane = lanes.end();
        for (auto lane = lanes.begin(); lane != lanes.end(); ++lane) {
            if (lane->clipType != 1) continue;
            const auto children = hierarchyRefs(graph, graph.elements.at(lane->track), "children", 15);
            if (std::find(children.begin(), children.end(), movedClip) != children.end()) { movedLane = lane; break; }
        }
        if (movedLane == lanes.end()) throw std::runtime_error("moved clip track not found");
        const auto movedSiblings = hierarchyRefs(graph, graph.elements.at(movedLane->track), "children", 15);
        const auto overlapSibling = std::find_if(movedSiblings.begin(), movedSiblings.end(), [&](uint32_t sibling) { return sibling != movedClip; });
        if (overlapSibling == movedSiblings.end()) throw std::runtime_error("no sibling available for collision check");
        const auto overlapFrameRefs = hierarchyRefs(graph, graph.elements.at(*overlapSibling), "timeFrame", 1);
        if (overlapFrameRefs.size() != 1) throw std::runtime_error("collision sibling timeframe missing");
        const AnimationTimeFrame overlapFrame(graph, graph.elements.at(overlapFrameRefs.front()));
        SessionTimelineDocumentCandidate collisionDocument(graph);
        bool overlapRejected = false;
        try { collisionDocument.moveTrackClip(movedClip, overlapFrame.start); }
        catch (const std::runtime_error&) { overlapRejected = true; }
        if (!overlapRejected || collisionDocument.dirty()) throw std::runtime_error("overlapping track clip move was not rejected atomically");

        uint32_t duplicateSource = UINT32_MAX, duplicateTrack = UINT32_MAX;
        int32_t duplicateStart = 0;
        uint32_t duplicateSourceFrameId = UINT32_MAX;
        for (int desiredType : {3, 2, 1}) {
          if (duplicateSource != UINT32_MAX) break;
          for (const auto& lane : lanes) {
            if (lane.clipType != desiredType) continue;
            const auto children = hierarchyRefs(graph, graph.elements.at(lane.track), "children", 15);
            for (uint32_t candidate : children) {
                const auto refs = hierarchyRefs(graph, graph.elements.at(candidate), "timeFrame", 1);
                if (refs.size() != 1) continue;
                const AnimationTimeFrame candidateFrame(graph, graph.elements.at(refs.front()));
                if (candidateFrame.duration <= 0) continue;
                bool found = false;
                for (size_t frame = 0; frame < frames.size() && !found; ++frame) {
                    const int32_t start = rootFrame.childTime(frames.tick(frame), false);
                    const int64_t end = int64_t(start) + candidateFrame.duration;
                    if (start == candidateFrame.start || start < domainStart || end > domainEnd) continue;
                    bool collision = false;
                    for (uint32_t sibling : children) {
                        const auto siblingRefs = hierarchyRefs(graph, graph.elements.at(sibling), "timeFrame", 1);
                        if (siblingRefs.size() != 1) continue;
                        const AnimationTimeFrame siblingFrame(graph, graph.elements.at(siblingRefs.front()));
                        if (siblingFrame.duration > 0 && int64_t(start) < int64_t(siblingFrame.start) + siblingFrame.duration &&
                            int64_t(siblingFrame.start) < end) { collision = true; break; }
                    }
                    if (!collision) { duplicateSource = candidate; duplicateTrack = lane.track; duplicateStart = start; duplicateSourceFrameId = refs.front(); found = true; }
                }
                if (found) break;
            }
            if (duplicateSource != UINT32_MAX) break;
          }
        }
        if (duplicateSource == UINT32_MAX) throw std::runtime_error("benchmark has no free Sound/overlay/Film duplicate destination");
        const AnimationTimeFrame duplicateSourceFrame(graph, graph.elements.at(duplicateSourceFrameId));
        SessionTimelineDocumentCandidate duplicateDocument(graph);
        const size_t originalElementCount = graph.elements.size();
        if (!duplicateDocument.duplicateTrackClipAt(duplicateSource, duplicateStart) || !duplicateDocument.dirty())
            throw std::runtime_error("track clip duplicate failed to apply / mark dirty");
        const uint32_t duplicateClip = uint32_t(originalElementCount + 1);
        const auto duplicateFrameRefs = hierarchyRefs(duplicateDocument.graph(), duplicateDocument.graph().elements.at(duplicateClip), "timeFrame", 1);
        if (duplicateFrameRefs.size() != 1 || duplicateFrameRefs.front() != originalElementCount)
            throw std::runtime_error("duplicated clip does not reference its copied timeframe");
        const AnimationTimeFrame duplicateFrame(duplicateDocument.graph(), duplicateDocument.graph().elements.at(duplicateFrameRefs.front()));
        if (duplicateFrame.start != duplicateStart || duplicateFrame.duration != duplicateSourceFrame.duration ||
            duplicateFrame.offset != duplicateSourceFrame.offset || duplicateFrame.scale != duplicateSourceFrame.scale)
            throw std::runtime_error("duplicate did not preserve media timing");
        if (duplicateDocument.graph().elements.at(duplicateClip).id == graph.elements.at(duplicateSource).id ||
            duplicateDocument.graph().strings.at(duplicateDocument.graph().elements.at(duplicateClip).name) == graph.strings.at(graph.elements.at(duplicateSource).name))
            throw std::runtime_error("duplicate clip identity or display name is not unique");
        const auto duplicateLanes = sessionTimelineTrackLanesCandidate(duplicateDocument.graph(), root, frames);
        if (std::none_of(duplicateLanes.begin(), duplicateLanes.end(), [&](const auto& lane) {
                return lane.track == duplicateTrack && std::any_of(lane.segments.begin(), lane.segments.end(),
                    [&](const auto& segment) { return segment.element == duplicateClip; });
            })) throw std::runtime_error("duplicated clip missing from rebuilt timeline lane");
        const size_t occupiedElementCount = duplicateDocument.graph().elements.size();
        const size_t occupiedTrackClipCount = hierarchyRefs(duplicateDocument.graph(), duplicateDocument.graph().elements.at(duplicateTrack), "children", 15).size();
        bool duplicateCollisionRejected = false;
        try { duplicateDocument.duplicateTrackClipAt(duplicateSource, duplicateStart); }
        catch (const std::runtime_error&) { duplicateCollisionRejected = true; }
        if (!duplicateCollisionRejected || duplicateDocument.graph().elements.size() != occupiedElementCount ||
            hierarchyRefs(duplicateDocument.graph(), duplicateDocument.graph().elements.at(duplicateTrack), "children", 15).size() != occupiedTrackClipCount)
            throw std::runtime_error("overlapping track clip duplicate was not rejected atomically");
        if (!duplicateDocument.undo() || duplicateDocument.dirty() || duplicateDocument.graph().elements.size() != originalElementCount)
            throw std::runtime_error("duplicate undo did not restore the clean graph");
        if (!duplicateDocument.redo() || !duplicateDocument.dirty()) throw std::runtime_error("duplicate redo failed");
        const std::string duplicatePath = std::string(argv[2]) + ".duplicated.dmx";
        duplicateDocument.saveAs(duplicatePath.c_str());
        if (duplicateDocument.dirty()) throw std::runtime_error("saved duplicate still marks document dirty");
        if (!duplicateDocument.undo() || !duplicateDocument.dirty() || !duplicateDocument.redo() || duplicateDocument.dirty())
            throw std::runtime_error("saved duplicate string-table undo/redo failed");
        const auto duplicateReloaded = loadGraph(duplicatePath.c_str());
        const auto reloadedDuplicateFrame = hierarchyRefs(duplicateReloaded, duplicateReloaded.elements.at(duplicateClip), "timeFrame", 1);
        if (reloadedDuplicateFrame.size() != 1 ||
            AnimationTimeFrame(duplicateReloaded, duplicateReloaded.elements.at(reloadedDuplicateFrame.front())).start != duplicateStart)
            throw std::runtime_error("duplicated track clip failed DMX save/reload");
        const auto reloadedDuplicateLanes = sessionTimelineTrackLanesCandidate(duplicateReloaded, root,
            sessionTimelineFramesCandidate(duplicateReloaded, root));
        if (std::none_of(reloadedDuplicateLanes.begin(), reloadedDuplicateLanes.end(), [&](const auto& lane) {
                return lane.track == duplicateTrack && std::any_of(lane.segments.begin(), lane.segments.end(),
                    [&](const auto& segment) { return segment.element == duplicateClip; });
            })) throw std::runtime_error("saved duplicate missing from reloaded timeline");
        const auto duplicateSourceLane = std::find_if(lanes.begin(), lanes.end(), [&](const auto& lane) {
            return lane.track == duplicateTrack;
        });
        if (duplicateSourceLane == lanes.end()) throw std::runtime_error("duplicate source lane metadata missing");
        const char* duplicateTypeName = duplicateSourceLane->clipType == 1 ? "sound" :
                                        duplicateSourceLane->clipType == 2 ? "overlay" : "film";
        std::cout << "trimmed_type=" << (selectedLane->clipType == 1 ? "sound" : "overlay")
                  << " lane=" << selectedLane->trackName << " clip=" << clip
                  << " undo_redo=passed save_reload=passed overlay_trim_undo_redo=passed"
                  << " track_clip_delete_undo_redo=passed delete_save_reload=passed"
                  << " moved_sound_clip=" << movedClip << " move_undo_redo=passed move_save_reload=passed"
                  << " overlap_rejected=passed duplicated_type=" << duplicateTypeName << " duplicated_clip=" << duplicateClip
                  << " duplicate_undo_redo=passed duplicate_save_reload=passed duplicate_overlap_rejected=passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

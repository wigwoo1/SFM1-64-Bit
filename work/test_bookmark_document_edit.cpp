#define NOMINMAX
#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_timeline_document_candidate.h"
#include "session_bookmark_action_candidate.h"
#include <chrono>
#include <iostream>

static const Attribute* findAttribute(const Graph& graph, uint32_t element, const char* name) {
    for (const auto& attribute : graph.elements.at(element).attributes)
        if (graph.strings.at(attribute.name) == name) return &attribute;
    return nullptr;
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("usage: test_bookmark_document_edit SESSION.dmx");
        SessionTimelineDocumentCandidate document(loadGraph(argv[1]));
        const Graph& initial = document.graph();
        uint32_t clip = UINT32_MAX;
        for (uint32_t i = 0; i < initial.elements.size(); ++i)
            if (initial.strings.at(initial.elements[i].type) == "DmeFilmClip" &&
                initial.strings.at(initial.elements[i].name) == "mtt_soldier") clip = i;
        if (clip == UINT32_MAX) throw std::runtime_error("benchmark clip missing");
        const auto* sets = findAttribute(initial, clip, "bookmarkSets");
        if (!sets || (sets->type != 1 && sets->type != 15) || sets->values.size() != 1) throw std::runtime_error("benchmark bookmark set linkage");
        const uint32_t setIndex = sets->values.front().reference;
        const auto* bookmarkArray = findAttribute(initial, setIndex, "bookmarks");
        if (!bookmarkArray || bookmarkArray->type != 15 || bookmarkArray->values.empty()) throw std::runtime_error("benchmark bookmark array");
        std::set<int32_t> existingTimes;
        for (const auto& item : bookmarkArray->values) {
            const auto* time = findAttribute(initial, item.reference, "time");
            if (!time || time->type != 7 || time->values.size() != 1) throw std::runtime_error("existing bookmark time shape");
            existingTimes.insert(wireInt(time->values.front()));
        }
        const uint32_t root = sessionRenderRootCandidate(initial);
        const auto ticks = sessionTimelineFramesCandidate(initial, root);
        const auto rootTimeFrames = hierarchyRefs(initial, initial.elements.at(root), "timeFrame", 1);
        if (rootTimeFrames.size() != 1 || root != clip) throw std::runtime_error("benchmark root bookmark clip mismatch");
        const AnimationTimeFrame rootTime(initial, initial.elements.at(rootTimeFrames.front()));
        int32_t playheadTick = 0, firstNewTime = 0;
        bool foundFreeClipFrame = false;
        for (size_t frame = 0; frame < ticks.size(); ++frame) {
            const auto rootTick = ticks.tick(frame);
            const auto localTick = rootTime.childTime(rootTick, false);
            if (!existingTimes.count(localTick)) {
                playheadTick = rootTick;
                firstNewTime = localTick;
                foundFreeClipFrame = true;
                break;
            }
        }
        if (!foundFreeClipFrame) throw std::runtime_error("no unbookmarked mtt_soldier timeline frame");
        const size_t originalElements = initial.elements.size();
        const size_t originalStrings = initial.strings.size();
        const size_t originalBookmarks = bookmarkArray->values.size();
        if (!addBookmarkAtPlayheadCandidate(document, playheadTick, [](const Graph&, uint32_t) {}) || !document.dirty())
            throw std::runtime_error("bookmark add or dirty state");
        if (document.graph().elements.size() != originalElements + 1) throw std::runtime_error("bookmark element append");
        const auto* addedArray = findAttribute(document.graph(), setIndex, "bookmarks");
        if (!addedArray || addedArray->values.size() != originalBookmarks + 1) throw std::runtime_error("bookmark set append");
        const uint32_t addedIndex = addedArray->values.back().reference;
        const auto* addedTime = findAttribute(document.graph(), addedIndex, "time");
        const auto* addedDuration = findAttribute(document.graph(), addedIndex, "duration");
        const auto* addedNote = findAttribute(document.graph(), addedIndex, "note");
        if (document.graph().strings.at(document.graph().elements[addedIndex].type) != "DmeBookmark" ||
            !addedTime || addedTime->type != 7 || wireInt(addedTime->values.at(0)) != firstNewTime ||
            !addedDuration || addedDuration->type != 7 || wireInt(addedDuration->values.at(0)) != 0 ||
            !addedNote || addedNote->type != 5 || addedNote->values.at(0).text != "")
            throw std::runtime_error("new bookmark fields");
        if (addBookmarkAtPlayheadCandidate(document, playheadTick, [](const Graph&, uint32_t) {}) || document.graph().elements.size() != originalElements + 1)
            throw std::runtime_error("duplicate bookmark timestamp was not a no-op");
        if (document.undo() != 1 || document.dirty() || document.graph().elements.size() != originalElements ||
            document.graph().strings.size() != originalStrings || findAttribute(document.graph(), setIndex, "bookmarks")->values.size() != originalBookmarks)
            throw std::runtime_error("unsaved bookmark undo did not restore original graph");
        if (document.redo() != 1 || !document.dirty() || document.graph().elements.size() != originalElements + 1)
            throw std::runtime_error("bookmark redo");

        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        const std::string output = "work\\bookmark-document-edit-" + std::to_string(stamp) + ".dmx";
        document.saveAs(output.c_str());
        if (document.dirty()) throw std::runtime_error("saved bookmark left document dirty");
        auto reopened = loadGraph(output.c_str()); validateGraph(reopened);
        const auto* reopenedArray = findAttribute(reopened, setIndex, "bookmarks");
        if (!reopenedArray || reopenedArray->values.size() != originalBookmarks + 1) throw std::runtime_error("reopened bookmark set");
        const uint32_t reopenedIndex = reopenedArray->values.back().reference;
        const auto* reopenedTime = findAttribute(reopened, reopenedIndex, "time");
        const auto* reopenedDuration = findAttribute(reopened, reopenedIndex, "duration");
        const auto* reopenedNote = findAttribute(reopened, reopenedIndex, "note");
        if (!reopenedTime || wireInt(reopenedTime->values.at(0)) != firstNewTime || !reopenedDuration ||
            wireInt(reopenedDuration->values.at(0)) != 0 || !reopenedNote || reopenedNote->values.at(0).text != "")
            throw std::runtime_error("saved bookmark values changed on reopen");
        std::filesystem::remove(output);
        if (document.undo() != 1 || !document.dirty() || document.graph().elements.size() != originalElements)
            throw std::runtime_error("undo after save did not mark document dirty");
        if (document.redo() != 1 || document.dirty() || document.graph().elements.size() != originalElements + 1)
            throw std::runtime_error("redo after save did not restore clean baseline");
        int32_t secondNewTime = firstNewTime + 1;
        while (existingTimes.count(secondNewTime) || secondNewTime == firstNewTime) ++secondNewTime;
        if (document.addBookmarkAtTime(setIndex, secondNewTime) != true || document.redo() != 0)
            throw std::runtime_error("bookmark edit did not clear redo branch");
        std::cout << "bookmark_document_edit=passed elements=" << originalElements
                  << " original_bookmarks=" << originalBookmarks << " saved_reopened_undo_redo=1"
                  << " duplicate_guard=1 save_baseline=1 graph_input=read_only\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

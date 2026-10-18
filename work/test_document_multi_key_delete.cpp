#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_timeline_document_candidate.h"
#include <chrono>
#include <iostream>

static bool sameValue(const Value& a, const Value& b) {
    return a.wire == b.wire && a.reference == b.reference && a.text == b.text;
}

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("session [scalar|vector3]");
        const std::string requested = argv[2];
        if (requested != "scalar" && requested != "vector3") throw std::runtime_error("unknown animation key kind");
        SessionTimelineDocumentCandidate doc(loadGraph(argv[1]));
        uint32_t layer = UINT32_MAX;
        for (uint32_t i = 0; i < doc.graph().elements.size() && layer == UINT32_MAX; ++i) {
            const auto& element = doc.graph().elements[i];
            const auto& type = doc.graph().strings.at(element.type);
            if ((requested == "scalar" && type != "DmeFloatLogLayer") ||
                (requested == "vector3" && type != "DmeVector3LogLayer")) continue;
            try {
                KeyTrack track(doc.graph(), element);
                if (track.times->values.size() >= 3 && track.times->values.size() == track.values->values.size()) layer = i;
            } catch (const std::runtime_error&) {}
        }
        if (layer == UINT32_MAX) throw std::runtime_error("session has no supported layer with at least three keys");

        const Graph original = doc.graph();
        const auto& sourceTrack = KeyTrack(original, original.elements[layer]);
        const auto originalTimes = sourceTrack.times->values;
        const auto originalValues = sourceTrack.values->values;
        std::vector<size_t> deleted{0, originalTimes.size() / 2, originalTimes.size() - 1, originalTimes.size() / 2};
        std::sort(deleted.begin(), deleted.end());
        deleted.erase(std::unique(deleted.begin(), deleted.end()), deleted.end());

        if (doc.deleteAnimationKeys(layer, {}) || doc.dirty()) throw std::runtime_error("empty multi-delete changed the document");
        bool invalidRejected = false;
        try { doc.deleteAnimationKeys(layer, {originalTimes.size()}); }
        catch (const std::runtime_error&) { invalidRejected = true; }
        if (!invalidRejected || doc.dirty()) throw std::runtime_error("invalid multi-delete was not atomic");

        if (!doc.deleteAnimationKeys(layer, deleted) || !doc.dirty()) throw std::runtime_error("multi-key delete missing or not dirty");
        std::vector<Value> expectedTimes, expectedValues;
        for (size_t i = 0; i < originalTimes.size(); ++i) {
            if (!std::binary_search(deleted.begin(), deleted.end(), i)) {
                expectedTimes.push_back(originalTimes[i]);
                expectedValues.push_back(originalValues[i]);
            }
        }
        auto verifyCurrent = [&](const Graph& graph, const char* stage) {
            const auto& track = KeyTrack(graph, graph.elements.at(layer));
            if (track.times->values.size() != expectedTimes.size() || track.values->values.size() != expectedValues.size())
                throw std::runtime_error(std::string(stage) + ": paired array size mismatch");
            for (size_t i = 0; i < expectedTimes.size(); ++i)
                if (!sameValue(track.times->values[i], expectedTimes[i]) || !sameValue(track.values->values[i], expectedValues[i]))
                    throw std::runtime_error(std::string(stage) + ": remaining key data mismatch");
        };
        verifyCurrent(doc.graph(), "delete");
        if (doc.undo() != 1 || doc.dirty()) throw std::runtime_error("multi-key delete undo boundary or clean state mismatch");
        const auto& undone = KeyTrack(doc.graph(), doc.graph().elements[layer]);
        if (undone.times->values.size() != originalTimes.size() || undone.values->values.size() != originalValues.size())
            throw std::runtime_error("multi-key delete undo array size mismatch");
        for (size_t i = 0; i < originalTimes.size(); ++i)
            if (!sameValue(undone.times->values[i], originalTimes[i]) || !sameValue(undone.values->values[i], originalValues[i]))
                throw std::runtime_error("multi-key delete undo did not restore exact arrays");
        if (doc.redo() != 1 || !doc.dirty()) throw std::runtime_error("multi-key delete redo mismatch");
        verifyCurrent(doc.graph(), "redo");

        const auto path = "work\\multi-key-delete-roundtrip-" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".dmx";
        doc.saveAs(path.c_str());
        if (doc.dirty()) throw std::runtime_error("saved multi-key deletion remains dirty");
        const auto reopened = loadGraph(path.c_str());
        verifyCurrent(reopened, "save/reopen");
        std::cout << "document_multi_key_delete_checks=11 layer=" << layer << " kind=" << requested
                  << " deleted=" << deleted.size() << " paired_arrays=true undo_redo=true save_reload=true\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

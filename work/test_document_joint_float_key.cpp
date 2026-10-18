#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_timeline_document_candidate.h"
#include <chrono>
#include <cmath>
#include <limits>

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("session required");
        SessionTimelineDocumentCandidate document(loadGraph(argv[1]));
        uint32_t layer = UINT32_MAX;
        size_t keyIndex = SIZE_MAX;
        int32_t oldTick = 0;
        float oldValue = 0;
        for (uint32_t i = 0; i < document.graph().elements.size() && layer == UINT32_MAX; ++i) {
            const auto& element = document.graph().elements[i];
            if (document.graph().strings.at(element.type) != "DmeFloatLogLayer") continue;
            try {
                KeyTrack track(document.graph(), element);
                if (track.values->type != 17) continue;
                for (size_t k = 0; k + 1 < track.times->values.size(); ++k) {
                    const auto current = wireInt(track.times->values[k]);
                    const int64_t gap = int64_t(wireInt(track.times->values[k + 1])) - current;
                    if (gap <= 1) continue;
                    layer = i;
                    keyIndex = k;
                    oldTick = current;
                    oldValue = wireFloat(track.values->values[k]);
                    break;
                }
            } catch (const std::runtime_error&) { continue; }
        }
        if (layer == UINT32_MAX) throw std::runtime_error("session has no movable float key");
        KeyTrack beforeTrack(document.graph(), document.graph().elements[layer]);
        const int32_t nextTick = int32_t(int64_t(oldTick) +
            (int64_t(wireInt(beforeTrack.times->values[keyIndex + 1])) - oldTick) / 2);
        const float nextValue = oldValue + 1.25f;
        if (!std::isfinite(nextValue)) throw std::runtime_error("fixture value overflow");
        const int32_t neighborTick = wireInt(beforeTrack.times->values[keyIndex + 1]);
        bool rejected = false;
        try { document.setFloatKey(layer, keyIndex, neighborTick + 1, oldValue); }
        catch (const std::runtime_error&) { rejected = true; }
        if (!rejected || document.dirty() ||
            wireInt(KeyTrack(document.graph(), document.graph().elements[layer]).times->values[keyIndex]) != oldTick)
            throw std::runtime_error("key crossing neighboring tick was accepted");
        rejected = false;
        try { document.setFloatKey(layer, keyIndex, oldTick, std::numeric_limits<float>::quiet_NaN()); }
        catch (const std::runtime_error&) { rejected = true; }
        if (!rejected || document.dirty()) throw std::runtime_error("non-finite joint key edit was accepted");
        auto edit = [&](auto& staged) { return staged.setFloatKey(layer, keyIndex, nextTick, nextValue); };
        if (!document.transact(edit, [](const Graph&) {}) || !document.dirty())
            throw std::runtime_error("joint key edit did not commit");
        auto check = [&](int32_t tick, float value) {
            KeyTrack track(document.graph(), document.graph().elements[layer]);
            if (wireInt(track.times->values[keyIndex]) != tick || wireFloat(track.values->values[keyIndex]) != value)
                throw std::runtime_error("joint key tick/value mismatch");
        };
        check(nextTick, nextValue);
        if (document.undo() != 2 || document.dirty()) throw std::runtime_error("joint key edit was not one undo step");
        check(oldTick, oldValue);
        if (document.redo() != 2 || !document.dirty()) throw std::runtime_error("joint key edit redo");
        check(nextTick, nextValue);
        auto path = "work/joint-key-roundtrip-" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".dmx";
        document.saveAs(path.c_str());
        Graph reopened = loadGraph(path.c_str());
        KeyTrack persisted(reopened, reopened.elements.at(layer));
        if (wireInt(persisted.times->values[keyIndex]) != nextTick ||
            wireFloat(persisted.values->values[keyIndex]) != nextValue || document.dirty())
            throw std::runtime_error("joint key edit did not persist through save/reopen");
        std::cout << "joint_float_key_edit=pass layer=" << layer << " index=" << keyIndex
                  << " old_tick=" << oldTick << " new_tick=" << nextTick
                  << " one_undo=1 journal_records=2 save_reload=1 cross_rejected=1 finite_rejected=1\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

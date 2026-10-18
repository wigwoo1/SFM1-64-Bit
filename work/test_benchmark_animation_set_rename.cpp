#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_timeline_document_candidate.h"
#include <iostream>

static uint64_t mix(uint64_t h, const void* data, size_t n) {
    const auto* p = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < n; ++i) { h ^= p[i]; h *= 1099511628211ull; }
    return h;
}
static uint64_t graphAttributesDigest(const Graph& graph) {
    uint64_t h = 14695981039346656037ull;
    for (const auto& element : graph.elements) {
        h = mix(h, &element.type, sizeof(element.type));
        h = mix(h, element.id.data(), element.id.size());
        const uint64_t count = element.attributes.size(); h = mix(h, &count, sizeof(count));
        for (const auto& attribute : element.attributes) {
            h = mix(h, &attribute.name, sizeof(attribute.name)); h = mix(h, &attribute.type, sizeof(attribute.type));
            const uint64_t values = attribute.values.size(); h = mix(h, &values, sizeof(values));
            for (const auto& value : attribute.values) {
                h = mix(h, &value.reference, sizeof(value.reference));
                h = mix(h, &value.stringIndex, sizeof(value.stringIndex));
                const uint64_t wire = value.wire.size(); h = mix(h, &wire, sizeof(wire));
                if (wire) h = mix(h, value.wire.data(), value.wire.size());
                const uint64_t text = value.text.size(); h = mix(h, &text, sizeof(text));
                if (text) h = mix(h, value.text.data(), value.text.size());
            }
        }
    }
    return h;
}

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("usage: test_benchmark_animation_set_rename benchmark.dmx output.dmx");
        Graph source = loadGraph(argv[1]);
        uint32_t target = UINT32_MAX;
        for (uint32_t i = 0; i < source.elements.size(); ++i)
            if (source.strings.at(source.elements[i].type) == "DmeAnimationSet") { target = i; break; }
        if (target == UINT32_MAX) throw std::runtime_error("benchmark has no DmeAnimationSet");
        std::vector<std::string> names; names.reserve(source.elements.size());
        for (const auto& e : source.elements) names.push_back(source.strings.at(e.name));
        const auto digest = graphAttributesDigest(source);
        const auto original = names[target];
        const std::string renamed = "sfm64_rename_benchmark_test";
        if (std::find(names.begin(), names.end(), renamed) != names.end()) throw std::runtime_error("test name already exists");

        SessionTimelineDocumentCandidate document(std::move(source));
        bool rejected = false;
        try { document.renameElement(target, "invalid\nname"); } catch (const std::runtime_error&) { rejected = true; }
        if (!rejected || document.dirty() || document.undo()) throw std::runtime_error("invalid input changed history or document");
        if (!document.renameElement(target, renamed) || !document.dirty()) throw std::runtime_error("rename did not apply");
        if (graphAttributesDigest(document.graph()) != digest) throw std::runtime_error("rename changed attributes, references, or payloads");
        for (uint32_t i = 0; i < names.size(); ++i) {
            const auto expected = i == target ? renamed : names[i];
            if (document.graph().strings.at(document.graph().elements[i].name) != expected)
                throw std::runtime_error("rename changed an unexpected element name");
        }
        if (document.undo() != 1 || document.dirty() || document.graph().strings.at(document.graph().elements[target].name) != original)
            throw std::runtime_error("rename undo mismatch");
        if (document.redo() != 1 || !document.dirty() || document.graph().strings.at(document.graph().elements[target].name) != renamed)
            throw std::runtime_error("rename redo mismatch");
        document.saveAs(argv[2]);
        if (document.dirty()) throw std::runtime_error("saved rename remains dirty");
        Graph reopened = loadGraph(argv[2]);
        if (reopened.elements.size() != names.size() || graphAttributesDigest(reopened) != digest)
            throw std::runtime_error("saved benchmark changed graph attributes, references, or payloads");
        for (uint32_t i = 0; i < names.size(); ++i) {
            const auto expected = i == target ? renamed : names[i];
            if (reopened.strings.at(reopened.elements[i].name) != expected)
                throw std::runtime_error("saved benchmark has unexpected element name");
        }
        if (document.undo() != 1 || !document.dirty() || document.graph().strings.at(document.graph().elements[target].name) != original)
            throw std::runtime_error("undo after save mismatch");
        if (document.redo() != 1 || document.dirty()) throw std::runtime_error("redo to saved state mismatch");
        std::cout << "benchmark_animation_set_rename=passed elements=" << names.size()
                  << " target=" << target << " invalid_input=1 attributes_and_references_preserved=1"
                  << " only_target_name_changed=1 undo_redo=1 save_reopen=1 saved_state_history=1"
                  << " native_SFM_rename_parity=unverified\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

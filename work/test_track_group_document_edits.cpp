#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_timeline_document_candidate.h"
#include <chrono>
#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
    std::filesystem::path output;
    try {
        const char* source = argc > 1 ? argv[1] : "work/roundtrip-11.dmx";
        auto graph = loadGraph(source);
        uint32_t group = UINT32_MAX;
        bool initialVisible = false;
        std::string initialName;
        for (uint32_t i = 0; i < graph.elements.size() && group == UINT32_MAX; ++i) {
            const auto& element = graph.elements[i];
            if (graph.strings.at(element.type) != "DmeTrackGroup") continue;
            bool hasVisible = false;
            for (const auto& attribute : element.attributes) {
                const auto& name = graph.strings.at(attribute.name);
                if (name == "visible" && attribute.type == 4 && attribute.values.size() == 1 && attribute.values[0].wire.size() == 1) {
                    hasVisible = true;
                    initialVisible = attribute.values[0].wire[0] != 0;
                }
            }
            if (hasVisible) { group = i; initialName = graph.strings.at(element.name); }
        }
        if (group == UINT32_MAX) throw std::runtime_error("fixture has no DmeTrackGroup with scalar visible attribute");
        uint32_t stringElement = UINT32_MAX;
        std::string stringAttribute, initialString;
        for (uint32_t i = 0; i < graph.elements.size() && stringElement == UINT32_MAX; ++i)
            for (const auto& attribute : graph.elements[i].attributes)
                if (attribute.type == 5 && attribute.values.size() == 1 &&
                    graph.strings.at(attribute.name) != "name" &&
                    attribute.values[0].stringIndex < graph.strings.size()) {
                    stringElement = i;
                    stringAttribute = graph.strings.at(attribute.name);
                    initialString = attribute.values[0].text;
                    break;
                }
        if (stringElement == UINT32_MAX) throw std::runtime_error("fixture has no scalar string attribute for generic writeback regression");

        SessionTimelineDocumentCandidate document(std::move(graph));
        if (document.dirty()) throw std::runtime_error("new document starts dirty");
        if (!document.setBooleanAttribute(group, "visible", !initialVisible) || !document.dirty())
            throw std::runtime_error("visibility edit did not change document");
        if (document.setBooleanAttribute(group, "visible", !initialVisible))
            throw std::runtime_error("same visibility edit should be a no-op");
        const auto replacementName = initialName + "_edited";
        if (!document.renameElement(group, replacementName))
            throw std::runtime_error("track-group element-name edit did not change document");
        const auto replacementString = initialString + "_edited";
        if (!document.setStringAttribute(stringElement, stringAttribute, replacementString))
            throw std::runtime_error("generic string attribute edit did not change document");
        if (document.graph().elements[group].attributes.size() == 0) throw std::runtime_error("group attributes disappeared");

        auto readGroup = [&](const Graph& savedGraph) {
            bool visible = initialVisible;
            std::string name;
            for (const auto& attribute : savedGraph.elements.at(group).attributes) {
                const auto& key = savedGraph.strings.at(attribute.name);
                if (key == "visible") visible = attribute.values.at(0).wire.at(0) != 0;
            }
            name = savedGraph.strings.at(savedGraph.elements.at(group).name);
            return std::make_pair(visible, name);
        };
        if (readGroup(document.graph()) != std::make_pair(!initialVisible, replacementName))
            throw std::runtime_error("edited attribute values are wrong");

        if (document.graph().elements.at(stringElement).attributes.empty()) throw std::runtime_error("string attribute owner disappeared");
        bool hasReplacementString = false;
        for (const auto& attribute : document.graph().elements.at(stringElement).attributes)
            if (document.graph().strings.at(attribute.name) == stringAttribute)
                hasReplacementString = attribute.values.at(0).text == replacementString;
        if (!hasReplacementString) throw std::runtime_error("string attribute write failed");
        if (document.undo() != 1 || readGroup(document.graph()) != std::make_pair(!initialVisible, replacementName))
            throw std::runtime_error("string attribute undo failed");
        if (document.undo() != 1 || readGroup(document.graph()) != std::make_pair(!initialVisible, initialName))
            throw std::runtime_error("element-name undo failed");
        if (document.undo() != 1 || readGroup(document.graph()) != std::make_pair(initialVisible, initialName) || document.dirty())
            throw std::runtime_error("visibility undo or clean savepoint failed");
        if (document.redo() != 1 || document.redo() != 1 || document.redo() != 1 ||
            readGroup(document.graph()) != std::make_pair(!initialVisible, replacementName))
            throw std::runtime_error("attribute redo failed");

        output = std::filesystem::path("work") /
            ("track-group-edits-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".dmx");
        document.saveAs(output.string().c_str());
        const auto reloaded = loadGraph(output.string().c_str());
        bool persistedString = false;
        for (const auto& attribute : reloaded.elements.at(stringElement).attributes)
            if (reloaded.strings.at(attribute.name) == stringAttribute)
                persistedString = attribute.values.at(0).text == replacementString;
        if (readGroup(reloaded) != std::make_pair(!initialVisible, replacementName) || !persistedString || document.dirty())
            throw std::runtime_error("saved visibility/name did not round-trip");
        std::filesystem::remove(output);
        std::cout << "track-group document edits passed visibility toggle, element rename, generic string attribute edit, noop, undo/redo, dirty-state, save/reload fixture=" << source << '\n';
        return 0;
    } catch (const std::exception& error) {
        if (!output.empty()) { std::error_code ignored; std::filesystem::remove(output, ignored); }
        std::cerr << error.what() << '\n';
        return 1;
    }
}

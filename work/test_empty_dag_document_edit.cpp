#define NOMINMAX
#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_timeline_document_candidate.h"
#include "graph_exact_equal.h"
#include <chrono>
#include <iostream>

static const Attribute* findAttribute(const Graph& graph, uint32_t element, const char* name) {
    for (const auto& attribute : graph.elements.at(element).attributes)
        if (graph.strings.at(attribute.name) == name) return &attribute;
    return nullptr;
}

struct EmptyDagTarget { uint32_t dag, parent; size_t position; };

static size_t childCount(const Graph& graph, uint32_t element) {
    const auto* children = findAttribute(graph, element, "children");
    if (!children) return 0;
    if (children->type != 15) throw std::runtime_error("DAG children is not a reference array");
    size_t count = 0;
    for (const auto& value : children->values)
        if (value.reference != 0xffffffffu && value.reference != 0xfffffffeu) ++count;
    return count;
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("usage: test_empty_dag_document_edit SESSION.dmx");
        const Graph original = loadGraph(argv[1]);
        validateGraph(original);
        EmptyDagTarget target{};
        bool found = false;
        uint32_t childfulDag = UINT32_MAX;
        for (uint32_t dag = 0; dag < original.elements.size(); ++dag) {
            if (original.strings.at(original.elements[dag].type) != "DmeDag") continue;
            const auto count = childCount(original, dag);
            if (count && childfulDag == UINT32_MAX) childfulDag = dag;
            if (count) continue;
            for (uint32_t parent = 0; parent < original.elements.size() && !found; ++parent) {
                const auto* children = findAttribute(original, parent, "children");
                if (!children || children->type != 15) continue;
                for (size_t i = 0; i < children->values.size(); ++i) {
                    if (children->values[i].reference == dag && i + 1 < children->values.size()) {
                        target = {dag, parent, i}; found = true; break;
                    }
                }
            }
            if (found) break;
        }
        if (!found) throw std::runtime_error("benchmark has no childless DmeDag with a later sibling for order check");
        if (childfulDag == UINT32_MAX) throw std::runtime_error("benchmark has no childful DmeDag for rejection check");

        const auto* oldChildren = findAttribute(original, target.parent, "children");
        std::vector<uint32_t> expected;
        for (const auto& value : oldChildren->values) {
            if (value.reference == target.dag) continue;
            expected.push_back(value.reference > target.dag ? value.reference - 1 : value.reference);
        }
        if (expected.size() + 1 != oldChildren->values.size())
            throw std::runtime_error("target parent does not contain exactly one DAG reference");

        SessionTimelineDocumentCandidate document(original);
        if (!document.transact(
                [&](SessionTimelineDocumentCandidate& staged) { return staged.deleteEmptyDag(target.dag); },
                [](const Graph&) {}))
            throw std::runtime_error("empty DAG deletion was a no-op");
        validateGraph(document.graph());
        const uint32_t parentAfter = target.parent > target.dag ? target.parent - 1 : target.parent;
        const auto* afterChildren = findAttribute(document.graph(), parentAfter, "children");
        if (!afterChildren || afterChildren->values.size() != expected.size())
            throw std::runtime_error("empty DAG parent reference removal failed");
        for (size_t i = 0; i < expected.size(); ++i)
            if (afterChildren->values[i].reference != expected[i])
                throw std::runtime_error("sibling order/reference remap changed during deletion");
        const Graph deleted = document.graph();

        if (document.undo() != 1 || !graphExactEqualCandidate(document.graph(), original))
            throw std::runtime_error("empty DAG undo did not restore exact graph");
        if (document.redo() != 1 || !graphExactEqualCandidate(document.graph(), deleted))
            throw std::runtime_error("empty DAG redo did not restore exact deletion");

        SessionTimelineDocumentCandidate rejectDocument(original);
        bool rejected = false;
        try { rejectDocument.deleteEmptyDag(childfulDag); }
        catch (const std::exception&) { rejected = true; }
        if (!rejected || !graphExactEqualCandidate(rejectDocument.graph(), original))
            throw std::runtime_error("childful DAG rejection mutated the graph");

        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        const std::string output = "work\\empty-dag-document-edit-" + std::to_string(stamp) + ".dmx";
        document.saveAs(output.c_str());
        auto reopened = loadGraph(output.c_str()); validateGraph(reopened);
        if (!graphExactEqualCandidate(reopened, deleted)) throw std::runtime_error("saved deletion changed the graph");
        std::filesystem::remove(output);
        if (document.undo() != 1 || !document.dirty() || !graphExactEqualCandidate(document.graph(), original))
            throw std::runtime_error("undo after deletion save did not restore and dirty the graph");
        if (document.redo() != 1 || document.dirty() || !graphExactEqualCandidate(document.graph(), deleted))
            throw std::runtime_error("redo after deletion save did not restore the clean baseline");

        std::cout << "empty_dag_document_edit=passed elements=" << original.elements.size()
                  << " target=" << target.dag << " stable_sibling_order=1 exact_undo_redo=1"
                  << " childful_rejected=1 save_reopen=1 save_baseline=1 graph_input=read_only\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

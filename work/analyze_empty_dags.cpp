#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include <iomanip>
#include <map>
#include <sstream>

static std::string jsonString(const std::string& value) {
    std::ostringstream out;
    out << std::quoted(value);
    return out.str();
}

static bool isDagType(const std::string& type) {
    return type == "DmeDag" || type == "DmeGameModel" || type == "DmeCamera" ||
           type == "DmeLight" || type == "DmeParticleSystem";
}

int main(int argc, char** argv) {
    try {
        static_assert(sizeof(void*) == 8, "native x64 required");
        if (argc != 2) throw std::runtime_error("usage: analyze_empty_dags session.dmx");
        const auto graph = loadGraph(argv[1]);
        std::vector<uint32_t> dags;
        std::vector<size_t> childCounts(graph.elements.size(), 0);
        std::vector<std::vector<std::string>> incoming(graph.elements.size());
        for (uint32_t source = 0; source < graph.elements.size(); ++source) {
            const auto& element = graph.elements[source];
            const auto& sourceType = graph.strings.at(element.type);
            if (isDagType(sourceType)) dags.push_back(source);
            for (const auto& attribute : element.attributes) {
                const auto& attributeName = graph.strings.at(attribute.name);
                const bool references = attribute.type == 1 || attribute.type == 15;
                if (!references) continue;
                for (const auto& value : attribute.values) {
                    if (value.reference >= graph.elements.size()) continue;
                    if (isDagType(sourceType) && attributeName == "children")
                        ++childCounts[source];
                    incoming[value.reference].push_back(
                        sourceType + "." + attributeName + " from element " + std::to_string(source));
                }
            }
        }
        size_t empty = 0, emptyWithoutIncoming = 0, emptyWithNonChildrenIncoming = 0;
        for (uint32_t dag : dags) {
            if (childCounts[dag]) continue;
            ++empty;
            if (incoming[dag].empty()) ++emptyWithoutIncoming;
            if (std::any_of(incoming[dag].begin(), incoming[dag].end(), [](const auto& reference) {
                    return reference.find(".children from element ") == std::string::npos;
                })) ++emptyWithNonChildrenIncoming;
        }
        std::cout << "{\n  \"input\": " << jsonString(argv[1])
                  << ",\n  \"elements\": " << graph.elements.size()
                  << ",\n  \"dag_derived_element_count\": " << dags.size()
                  << ",\n  \"empty_dag_derived_element_count\": " << empty
                  << ",\n  \"empty_dag_derived_elements_without_incoming_references\": " << emptyWithoutIncoming
                  << ",\n  \"empty_dag_derived_elements_with_non_children_incoming_references\": " << emptyWithNonChildrenIncoming
                  << ",\n  \"samples\": [\n";
        size_t shown = 0;
        for (uint32_t dag : dags) {
            if (childCounts[dag] || shown == 24) continue;
            const auto& element = graph.elements[dag];
            if (shown) std::cout << ",\n";
            std::cout << "    {\"element\": " << dag
                      << ", \"type\": " << jsonString(graph.strings.at(element.type))
                      << ", \"name\": " << jsonString(graph.strings.at(element.name))
                      << ", \"incoming_reference_count\": " << incoming[dag].size()
                      << ", \"incoming_references\": [";
            for (size_t i = 0; i < incoming[dag].size(); ++i) {
                if (i) std::cout << ", ";
                std::cout << jsonString(incoming[dag][i]);
            }
            std::cout << "]}";
            ++shown;
        }
        std::cout << "\n  ],\n  \"non_children_reference_examples\": [\n";
        shown = 0;
        for (uint32_t dag : dags) {
            if (childCounts[dag] || shown == 16) continue;
            const auto& refs = incoming[dag];
            if (!std::any_of(refs.begin(), refs.end(), [](const auto& reference) {
                    return reference.find(".children from element ") == std::string::npos;
                })) continue;
            const auto& element = graph.elements[dag];
            if (shown) std::cout << ",\n";
            std::cout << "    {\"element\": " << dag
                      << ", \"type\": " << jsonString(graph.strings.at(element.type))
                      << ", \"name\": " << jsonString(graph.strings.at(element.name))
                      << ", \"incoming_references\": [";
            for (size_t i = 0; i < refs.size(); ++i) {
                if (i) std::cout << ", ";
                std::cout << jsonString(refs[i]);
            }
            std::cout << "]}";
            ++shown;
        }
        std::cout << "\n  ]\n}\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

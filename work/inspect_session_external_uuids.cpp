#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include <iomanip>
#include <set>

int main(int argc, char** argv) {
    try {
        static_assert(sizeof(void*) == 8, "x64 build required");
        if (argc != 2) throw std::runtime_error("usage: inspect_session_external_uuids INPUT.dmx");
        const auto graph = loadGraph(argv[1]);
        uint64_t total = 0;
        std::set<std::string> uuids;
        struct Location { uint32_t element; uint32_t attribute; std::string uuid; };
        std::vector<Location> locations;
        for (uint32_t ei = 0; ei < graph.elements.size(); ++ei) {
            const auto& element = graph.elements[ei];
            for (uint32_t ai = 0; ai < element.attributes.size(); ++ai) {
                const auto& attribute = element.attributes[ai];
                const auto scalarType = attribute.type >= 15 ? attribute.type - 14 : attribute.type;
                if (scalarType != 1) continue;
                for (const auto& value : attribute.values) {
                    if (value.reference == 0xfffffffeu) {
                        ++total;
                        uuids.insert(value.text);
                        locations.push_back({ei, ai, value.text});
                    }
                }
            }
        }
        std::cout << "{\"pointer_bits\":64,\"elements\":" << graph.elements.size()
                  << ",\"external_reference_values\":" << total
                  << ",\"unique_external_uuids\":" << uuids.size() << "}\n";
        size_t shown = 0;
        for (const auto& uuid : uuids) {
            if (shown++ == 24) break;
            std::cout << uuid << '\n';
        }
        for (const auto& location : locations) {
            const auto& element = graph.elements.at(location.element);
            const auto& attribute = element.attributes.at(location.attribute);
            std::cout << "owner_element_index=" << location.element
                      << " owner_type=" << graph.strings.at(element.type)
                      << " owner_name=" << graph.strings.at(element.name)
                      << " attribute=" << graph.strings.at(attribute.name)
                      << " uuid=" << location.uuid << '\n';
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

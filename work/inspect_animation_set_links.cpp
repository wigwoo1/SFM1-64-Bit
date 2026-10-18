#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include <iostream>

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("usage: inspect_animation_set_links session.dmx");
        const auto graph = loadGraph(argv[1]);
        size_t backingNameMatches = 0, backingReferences = 0;
        for (uint32_t i = 0; i < graph.elements.size(); ++i) {
            const auto& e = graph.elements[i];
            if (graph.strings.at(e.type) != "DmeAnimationSet") continue;
            std::cout << "set " << i << " name=\"" << graph.strings.at(e.name) << "\"\n";
            for (const auto& a : e.attributes) {
                const auto& name = graph.strings.at(a.name);
                std::cout << "  " << name << " type=" << unsigned(a.type) << " count=" << a.values.size();
                if ((a.type == 1 || a.type == 15) && !a.values.empty()) {
                    std::cout << " refs=";
                    for (const auto& v : a.values) {
                        std::cout << v.reference;
                        if (v.reference < graph.elements.size()) {
                            const auto& target = graph.elements[v.reference];
                            std::cout << "[" << graph.strings.at(target.type) << ":" << graph.strings.at(target.name) << "]";
                        }
                        std::cout << ";";
                        if ((name == "gameModel" || name == "camera") && v.reference < graph.elements.size()) {
                            ++backingReferences;
                            if (graph.strings.at(graph.elements[v.reference].name) == graph.strings.at(e.name)) ++backingNameMatches;
                        }
                    }
                } else if (a.type == 5 && !a.values.empty()) std::cout << " value=\"" << a.values.front().text << "\"";
                std::cout << '\n';
            }
            for (uint32_t owner = 0; owner < graph.elements.size(); ++owner) {
                const auto& o = graph.elements[owner];
                for (const auto& a : o.attributes) {
                    if (a.type != 1 && a.type != 15) continue;
                    for (const auto& v : a.values) if (v.reference == i)
                        std::cout << "  incoming owner=" << owner << " type=" << graph.strings.at(o.type)
                                  << " name=\"" << graph.strings.at(o.name) << "\" attr=" << graph.strings.at(a.name) << '\n';
                }
            }
        }
        std::cout << "summary animation_set_backing_refs=" << backingReferences
                  << " exact_set_name_matches=" << backingNameMatches << '\n';
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

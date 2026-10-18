#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include <iostream>

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("usage: inspect_animation_set_channel_links session.dmx");
        const auto graph = loadGraph(argv[1]);
        uint32_t setId = UINT32_MAX;
        for (uint32_t i = 0; i < graph.elements.size(); ++i)
            if (graph.strings.at(graph.elements[i].type) == "DmeAnimationSet") { setId = i; break; }
        if (setId == UINT32_MAX) throw std::runtime_error("no DmeAnimationSet in session");
        const auto& set = graph.elements[setId];
        std::cout << "animation_set=" << setId << " name=" << graph.strings.at(set.name) << '\n';
        size_t emitted = 0;
        for (const auto& attribute : set.attributes) {
            if (graph.strings.at(attribute.name) != "controls" || attribute.type != 15) continue;
            for (const auto& reference : attribute.values) {
                if (reference.reference >= graph.elements.size()) continue;
                const auto& control = graph.elements[reference.reference];
                if (graph.strings.at(control.type) != "DmeTransformControl") continue;
                std::cout << "control=" << reference.reference << " name=" << graph.strings.at(control.name) << '\n';
                for (const auto& field : control.attributes) {
                    const auto& fieldName = graph.strings.at(field.name);
                    if (fieldName.find("Channel") == std::string::npos && fieldName.find("channel") == std::string::npos) continue;
                    std::cout << "  " << fieldName << " type=" << unsigned(field.type) << " count=" << field.values.size();
                    for (const auto& value : field.values) if (field.type == 1 || field.type == 15) {
                        std::cout << " ref=" << value.reference;
                        if (value.reference < graph.elements.size()) {
                            const auto& target = graph.elements[value.reference];
                            std::cout << " target=" << graph.strings.at(target.type) << ':' << graph.strings.at(target.name);
                        }
                    }
                    std::cout << '\n';
                }
                if (++emitted == 12) break;
            }
            break;
        }
        std::cout << "transform_controls_reported=" << emitted << '\n';
        return emitted ? 0 : 2;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

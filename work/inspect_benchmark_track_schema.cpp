#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include <iostream>
#include <set>

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("usage: inspect_benchmark_track_schema session.dmx");
        const auto graph = loadGraph(argv[1]);
        auto attribute = [&](uint32_t element, const char* name) -> const Attribute* {
            for (const auto& candidate : graph.elements.at(element).attributes)
                if (graph.strings.at(candidate.name) == name) return &candidate;
            return nullptr;
        };
        auto reference = [&](uint32_t element, const char* name) -> uint32_t {
            const auto* found = attribute(element, name);
            return found && !found->values.empty() ? found->values.front().reference : UINT32_MAX;
        };
        auto refs = [&](uint32_t element, const char* name) {
            std::vector<uint32_t> result;
            const auto* found = attribute(element, name);
            if (found) for (const auto& value : found->values)
                if (value.reference < graph.elements.size()) result.push_back(value.reference);
            return result;
        };
        uint32_t root = UINT32_MAX;
        for (uint32_t i = 0; i < graph.elements.size(); ++i)
            if (graph.strings.at(graph.elements[i].type) == "DmeFilmClip" &&
                reference(i, "subClipTrackGroup") < graph.elements.size()) { root = i; break; }
        if (root == UINT32_MAX) throw std::runtime_error("root film clip with sub-clip track group not found");
        std::cout << "root DmeFilmClip " << graph.strings.at(graph.elements[root].name) << "\n";
        std::set<uint32_t> groups;
        const auto special = reference(root, "subClipTrackGroup");
        if (special < graph.elements.size()) groups.insert(special);
        for (auto id : refs(root, "trackGroups")) groups.insert(id);
        for (auto group : groups) {
            const auto& groupElement = graph.elements.at(group);
            std::cout << "track group " << graph.strings.at(groupElement.name) << " tracks=";
            const auto tracks = refs(group, "tracks");
            std::cout << tracks.size() << '\n';
            for (auto track : tracks) {
                const auto& trackElement = graph.elements.at(track);
                const auto children = refs(track, "children");
                const auto* clipType = attribute(track, "clipType");
                std::cout << "  DmeTrack " << graph.strings.at(trackElement.name)
                          << " clipType=" << (clipType && !clipType->values.empty() ? wireInt(clipType->values[0]) : -1)
                          << " children=" << children.size();
                std::set<std::string> childTypes;
                for (auto child : children) childTypes.insert(graph.strings.at(graph.elements.at(child).type));
                for (const auto& type : childTypes) std::cout << " " << type;
                std::cout << '\n';
            }
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

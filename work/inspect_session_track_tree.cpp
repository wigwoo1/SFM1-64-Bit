#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "animation_keys.h"
#include "session_render_root_candidate.h"
#include "animation_time_frame.h"
#include <iostream>

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("usage: inspect_session_track_tree session.dmx");
        const auto graph = loadGraph(argv[1]);
        const auto root = sessionRenderRootCandidate(graph);
        std::set<uint32_t> visited;
        size_t printed = 0;
        constexpr size_t limit = 1200;
        auto walk = [&](auto&& self, uint32_t id, unsigned depth) -> void {
            if (id >= graph.elements.size() || depth > 12 || printed >= limit || !visited.insert(id).second) return;
            const auto& element = graph.elements[id];
            const auto& type = graph.strings.at(element.type);
            std::cout << std::string(depth * 2, ' ') << id << ' ' << type << ' ' << graph.strings.at(element.name);
            if (type.find("Clip") != std::string::npos) {
                try {
                    auto refs = hierarchyRefs(graph, element, "timeFrame", 1);
                    if (refs.size() == 1) {
                        AnimationTimeFrame frame(graph, graph.elements.at(refs.front()));
                        std::cout << " start=" << frame.start << " duration=" << frame.duration;
                    }
                } catch (...) {}
            }
            std::cout << '\n';
            ++printed;
            auto follow = [&](const char* name, uint8_t dmxType) {
                try { for (auto child : hierarchyRefs(graph, element, name, dmxType)) self(self, child, depth + 1); }
                catch (const std::exception& e) { std::cout << std::string((depth + 1) * 2, ' ') << "! " << name << ": " << e.what() << '\n'; }
            };
            if (type == "DmeFilmClip") { follow("trackGroups", 15); follow("subClipTrackGroup", 1); }
            else if (type == "DmeTrackGroup") follow("tracks", 15);
            else if (type == "DmeTrack") {
                // Show only timeline/editor rows; large audio and particle clip
                // arrays are counted elsewhere and would mask the film track.
                try {
                    for (auto child : hierarchyRefs(graph, element, "children", 15)) {
                        if (child >= graph.elements.size()) continue;
                        const auto& childType = graph.strings.at(graph.elements[child].type);
                        if (childType == "DmeFilmClip" || childType == "DmeTrack" ||
                            childType == "DmeTrackGroup")
                            self(self, child, depth + 1);
                    }
                } catch (const std::exception& e) {
                    std::cout << std::string((depth + 1) * 2, ' ') << "! children: " << e.what() << '\n';
                }
            }
            // Channel clips can contain thousands of per-bone channels. They
            // are animation data, not timeline lanes, so stop at this level.
        };
        std::cout << "root=" << root << ' ' << graph.strings.at(graph.elements.at(root).type)
                  << ' ' << graph.strings.at(graph.elements.at(root).name) << '\n';
        walk(walk, root, 0);
        std::cout << "printed_nodes=" << printed << " visited_nodes=" << visited.size()
                  << " bounded_limit=" << limit << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

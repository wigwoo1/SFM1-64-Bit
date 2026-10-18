#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_render_root_candidate.h"
#include "session_timeline_lane_candidate.h"
#include <iostream>

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("usage: test_session_timeline_track_lanes session.dmx");
        const auto graph = loadGraph(argv[1]);
        const auto root = sessionRenderRootCandidate(graph);
        const auto frames = sessionTimelineFramesCandidate(graph, root);
        const auto lanes = sessionTimelineTrackLanesCandidate(graph, root, frames);
        const auto film = sessionTimelineLaneCandidate(graph, root, frames);
        if (lanes.size() != 7) throw std::runtime_error("benchmark should expose seven track lanes");
        if (lanes.front().groupName != "subClipTrackGroup" || lanes.front().trackName != "Film" || lanes.front().clipType != 3)
            throw std::runtime_error("film lane order or type changed");
        if (lanes.front().segments.size() != film.size()) throw std::runtime_error("film lane segment count differs from current shot lane");
        for (size_t i = 0; i < film.size(); ++i) {
            const auto& a = lanes.front().segments[i];
            const auto& b = film[i];
            if (a.startFrame != b.startFrame || a.endFrame != b.endFrame || a.element != b.element)
                throw std::runtime_error("film lane differs from current shot lane");
        }
        size_t soundLanes = 0, overlayLanes = 0, soundClipSegments = 0, overlayClipSegments = 0;
        for (const auto& lane : lanes) {
            for (const auto& segment : lane.segments)
                if (segment.startFrame >= segment.endFrame || segment.endFrame > frames.size())
                    throw std::runtime_error("invalid timeline lane segment bounds");
            if (lane.clipType == 1) { ++soundLanes; soundClipSegments += lane.segments.size(); }
            if (lane.clipType == 2) { ++overlayLanes; overlayClipSegments += lane.segments.size(); }
        }
        if (soundLanes != 5 || overlayLanes != 1 || soundClipSegments == 0 || overlayClipSegments == 0)
            throw std::runtime_error("benchmark sound or overlay lanes missing");
        std::cout << "track_lanes=" << lanes.size() << " film_segments=" << film.size()
                  << " sound_lanes=" << soundLanes << " sound_segments=" << soundClipSegments
                  << " overlay_lanes=" << overlayLanes << " overlay_segments=" << overlayClipSegments
                  << " film_lane_parity=passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

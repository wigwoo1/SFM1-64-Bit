#define NOMINMAX
#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "win32_sfm_editor_shell_candidate.h"
#include "session_bookmark_projection_candidate.h"
#include "session_render_root_candidate.h"
#include <chrono>
#include <iostream>

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("usage: benchmark_editor_hierarchy_mtt SESSION.dmx");
        auto graph = loadGraph(argv[1]);
        const auto root = sessionRenderRootCandidate(graph);
        const auto frames = sessionTimelineFramesCandidate(graph, root);
        const auto bookmarkFrames = sessionBookmarkFramesCandidate(graph, root, frames);
        const std::vector<size_t> expectedBookmarkFrames{
            86, 143, 186, 1043, 1121, 1353, 1422, 1484,
            1505, 1522, 1537, 1555, 1566, 1771, 1822, 2060};
        if (frames.size() != 2276 || bookmarkFrames != expectedBookmarkFrames)
            throw std::runtime_error("mtt_soldier bookmark-to-timeline projection changed");
        const auto begin = std::chrono::steady_clock::now();
        const auto rows = buildSfmEditorElementRowsCandidate(graph);
        const auto allRows = buildSfmEditorElementRowsCandidate(graph, true);
        const auto end = std::chrono::steady_clock::now();
        size_t multiParentRows = 0;
        uint32_t maxDepth = 0;
        for (const auto& row : rows) {
            multiParentRows += row.additionalParents != 0;
            maxDepth = std::max(maxDepth, row.hierarchyDepth);
        }
        const auto elapsed = std::chrono::duration<double, std::milli>(end - begin).count();
        if (allRows.size() != graph.elements.size())
            throw std::runtime_error("Element Viewer rows do not cover every DMX element");
        size_t summarizedAttributes = 0;
        for (size_t i = 0; i < allRows.size(); ++i) {
            if (allRows[i].index != i) throw std::runtime_error("Element Viewer row order lost DMX identity");
            if (allRows[i].attributeDetails.size() != graph.elements[i].attributes.size())
                throw std::runtime_error("Element Viewer attribute summary coverage mismatch");
            summarizedAttributes += allRows[i].attributeDetails.size();
            for (const auto& attribute : allRows[i].attributeDetails)
                if (attribute.empty()) throw std::runtime_error("empty Element Viewer attribute summary");
        }
        std::cout << "editor_hierarchy_benchmark passed elements=" << graph.elements.size()
                  << " animation_set_rows=" << rows.size() << " element_viewer_rows=" << allRows.size()
                  << " summarized_attributes=" << summarizedAttributes
                  << " multiparent_rows=" << multiParentRows
                  << " max_depth=" << maxDepth << " bookmark_markers=" << bookmarkFrames.size()
                  << " frame_count=" << frames.size() << " elapsed_ms=" << elapsed << '\n';
        std::cout << "bookmark_frames=";
        for (size_t i = 0; i < bookmarkFrames.size(); ++i) std::cout << (i ? "," : "") << bookmarkFrames[i];
        std::cout << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

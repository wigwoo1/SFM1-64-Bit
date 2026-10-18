#define NOMINMAX
#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "animation_keys.h"
#include "win32_sfm_editor_shell_candidate.h"
#include "session_graph_curve_projection_candidate.h"

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("session required");
        const Graph graph = loadGraph(argv[1]);
        const auto curves = buildSfmEditorGraphCurvesCandidate(graph);
        size_t scalarCount = 0, vectorCount = 0;
        std::map<uint32_t, std::array<const SfmEditorGraphCurveCandidate*, 3>> vectors;
        for (const auto& curve : curves) {
            if (curve.vectorComponent == 0xff) { ++scalarCount; continue; }
            if (curve.vectorComponent >= 3 || curve.element >= graph.elements.size())
                throw std::runtime_error("projected vector curve has invalid source metadata");
            auto& components = vectors[curve.element];
            if (components[curve.vectorComponent]) throw std::runtime_error("duplicate projected vector component");
            components[curve.vectorComponent] = &curve;
            ++vectorCount;
        }
        if (!scalarCount || !vectorCount || vectorCount % 3)
            throw std::runtime_error("scalar/vector curve projection incomplete");
        for (const auto& pair : vectors) {
            const auto& element = graph.elements.at(pair.first);
            KeyTrack track(graph, element);
            if (track.values->type != 24) throw std::runtime_error("vector projection points to non-vector layer");
            for (unsigned component = 0; component < 3; ++component) {
                const auto* curve = pair.second[component];
                if (!curve || curve->keys.size() != track.values->values.size())
                    throw std::runtime_error("missing projected axis");
                for (size_t key = 0; key < curve->keys.size(); ++key)
                    if (curve->keys[key].tick != wireInt(track.times->values[key]) ||
                        curve->keys[key].value != wireFloat(track.values->values[key], component))
                        throw std::runtime_error("projected axis differs from DMX source");
            }
        }
        std::cout << "graph_curve_projection=pass scalar_curves=" << scalarCount
                  << " vector_layers=" << vectors.size() << " vector_components=" << vectorCount << '\n';
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}

#pragma once
#include <cmath>
#include <optional>
#include "graph_float_curve_sampler.h"
#include "graph_vector_curve_sampler.h"

inline std::vector<SfmEditorGraphCurveCandidate> buildSfmEditorGraphCurvesCandidate(const Graph& graph) {
    std::vector<SfmEditorGraphCurveCandidate> curves;
    auto append = [&](uint32_t layer, const std::string& label, uint8_t component) {
        const auto& element = graph.elements.at(layer);
        KeyTrack track(graph, element);
        if (track.values->type != (component == 0xff ? 17 : 24) || track.values->values.empty()) return;
        SfmEditorGraphCurveCandidate curve;
        curve.element = layer;
        curve.vectorComponent = component;
        curve.label = label;
        curve.keys.reserve(track.values->values.size());
        for (size_t key = 0; key < track.values->values.size(); ++key) {
            const float value = component == 0xff ? wireFloat(track.values->values[key])
                                                    : wireFloat(track.values->values[key], component);
            if (!std::isfinite(value)) throw std::runtime_error("nonfinite graph key");
            curve.keys.push_back({wireInt(track.times->values[key]), value});
        }
        curves.push_back(std::move(curve));
    };
    for (uint32_t i = 0; i < graph.elements.size(); ++i) {
        const auto& element = graph.elements[i];
        const auto& type = graph.strings.at(element.type);
        if (type == "DmeFloatLogLayer") {
            try {
                for (uint32_t log = 0; log < graph.elements.size(); ++log) {
                    if (graph.strings.at(graph.elements[log].type) != "DmeFloatLog") continue;
                    const Attribute* layers = nullptr;
                    for (const auto& attribute : graph.elements[log].attributes)
                        if (graph.strings.at(attribute.name) == "layers") { layers = &attribute; break; }
                    if (!layers || layers->type != 15 || std::none_of(layers->values.begin(), layers->values.end(), [i](const Value& value) { return value.reference == i; })) continue;
                    append(i, graph.strings.at(element.name) + " [" + std::to_string(i) + "]", 0xff);
                }
            }
            catch (const std::runtime_error&) { continue; }
        } else if (type == "DmeVector3Log") {
            const Attribute* layers = nullptr;
            for (const auto& attribute : element.attributes)
                if (graph.strings.at(attribute.name) == "layers") { layers = &attribute; break; }
            if (!layers || layers->type != 15) continue;
            for (const auto& reference : layers->values) {
                if (reference.reference >= graph.elements.size() ||
                    graph.strings.at(graph.elements[reference.reference].type) != "DmeVector3LogLayer") continue;
                for (uint8_t component = 0; component < 3; ++component) {
                    try {
                        const auto layer = reference.reference;
                        append(layer, graph.strings.at(graph.elements[layer].name) + " / " +
                                      graph.strings.at(element.name) + " / " + "XYZ"[component], component);
                    } catch (const std::runtime_error&) { continue; }
                }
            }
        }
    }
    return curves;
}





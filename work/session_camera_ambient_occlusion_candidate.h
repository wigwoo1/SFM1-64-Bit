#pragma once
#include "clip_hierarchy.h"
#include <array>
#include <cmath>
#include <optional>

struct SessionCameraAmbientOcclusionCandidate {
    float bias, radius, inverseFalloff;
    std::optional<std::array<float,3>> shadingTint{};
};

inline std::optional<std::array<float,3>> readSessionCameraOcclusionTintCandidate(
        const Graph& graph, uint32_t camera, uint32_t renderSettings) {
    if (renderSettings == UINT32_MAX) return {};
    const auto& settings = graph.elements.at(renderSettings);
    const auto* mode = namedAttribute(graph, settings, "ambientOcclusionMode");
    const auto* show = namedAttribute(graph, settings, "showAmbientOcclusion");
    if (!mode || mode->type != 2 || mode->values.size() != 1 ||
        mode->values.front().wire.size() != 4 || wireInt(mode->values.front()) != 1)
        return {};
    if (show) {
        if (show->values.size() != 1) return {};
        const auto& value = show->values.front();
        const bool hidden = (show->type == 2 && value.wire.size() == 4 && wireInt(value) == 0) ||
                            (show->type == 4 && value.wire.size() == 1 && value.wire.front() == 0);
        if (!hidden) return {};
    }
    const auto* tint = namedAttribute(graph, graph.elements.at(camera), "SSAOTint");
    if (!tint) return std::array<float,3>{};
    if (tint->type != 8 || tint->values.size() != 1 || tint->values.front().wire.size() != 4)
        return {};
    const auto& rgba = tint->values.front().wire;
    constexpr float normalize = 1.f/255.f;
    return std::array<float,3>{uint8_t(rgba[0])*normalize,uint8_t(rgba[1])*normalize,uint8_t(rgba[2])*normalize};
}

// The native shader receives far-normalized depth. With linear view depth
// in the Vulkan buffer, the camera bias stays in depth-relative units and
// the falloff is 1 / (radius * strength) in linear view-depth units.
inline SessionCameraAmbientOcclusionCandidate readSessionCameraAmbientOcclusionCandidate(
        const Graph& graph, uint32_t camera) {
    const auto& element = graph.elements.at(camera);
    if (graph.strings.at(element.type) != "DmeCamera")
        throw std::runtime_error("ambient occlusion settings require a camera");
    const auto scalar = [&](const char* name, float fallback) {
        const auto* attribute = namedAttribute(graph, element, name);
        if (!attribute) return fallback;
        if (attribute->type != 3 || attribute->values.size() != 1 ||
            attribute->values.front().wire.size() != sizeof(float))
            throw std::runtime_error(std::string("ambient occlusion camera scalar schema: ") + name);
        const float value = wireFloat(attribute->values.front());
        if (!std::isfinite(value))
            throw std::runtime_error(std::string("nonfinite ambient occlusion camera scalar: ") + name);
        return value;
    };
    const float bias = scalar("SSAOBias", .0005f);
    const float radius = scalar("SSAORadius", 15.f);
    const float strength = scalar("SSAOStrength", 1.f);
    if (radius <= 0 || strength <= 0)
        throw std::runtime_error("ambient occlusion camera diagnostic requires positive radius and strength");
    const float falloff = radius * strength;
    if (!std::isfinite(falloff) || falloff == 0)
        throw std::runtime_error("ambient occlusion camera falloff outside the supported float range");
    const float inverse = 1.f / falloff;
    if (!std::isfinite(inverse))
        throw std::runtime_error("ambient occlusion camera falloff outside the supported float range");
    return {bias, radius, inverse};
}

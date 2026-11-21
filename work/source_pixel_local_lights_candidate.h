#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>

struct SourcePixelLocalLightCandidate {
    bool directional = false;
    std::array<float, 3> color{};
    std::array<float, 3> position{};
    std::array<float, 3> direction{};
};

// Convert an already selected world light to the inputs used by pixel packing.
// The scene supplies the current integer style value for the light's style ID.
inline std::optional<SourcePixelLocalLightCandidate> sourcePixelLocalLightFromWorldCandidate(
    uint32_t worldType, std::array<float, 3> intensity,
    const std::array<float, 3>& position, const std::array<float, 3>& emissionNormal,
    int32_t styleValue) {
    if (worldType >= 4) return {};
    const float scale = static_cast<float>(styleValue) * 0.0037878789007663727f;
    for (auto& component : intensity) component *= scale;
    return SourcePixelLocalLightCandidate{worldType == 3, intensity, position, emissionNormal};
}

// Six float4 constants used by the recovered model-pixel light upload.
// Lights 0–2 use RGB/XYZ pairs. Light 3 uses W in rows 0–2 for color
// and W in rows 3–5 for position.
inline std::array<std::array<float, 4>, 6> sourcePixelLocalLightsCandidate(
    const std::array<float, 3>& lightingOrigin,
    const std::array<SourcePixelLocalLightCandidate, 4>& lights, std::size_t count) {
    if (count > lights.size()) throw std::invalid_argument("pixel light count exceeds four");
    std::array<std::array<float, 4>, 6> constants{};
    for (std::size_t i = 0; i < count; ++i) {
        const auto& light = lights[i];
        auto position = light.position;
        if (light.directional) {
            for (std::size_t axis = 0; axis < 3; ++axis) {
                const float displacement = light.direction[axis] * 10000.f;
                position[axis] = lightingOrigin[axis] - displacement;
            }
        }
        if (i < 3) {
            for (std::size_t axis = 0; axis < 3; ++axis) {
                constants[2 * i][axis] = light.color[axis];
                constants[2 * i + 1][axis] = position[axis];
            }
        } else {
            for (std::size_t axis = 0; axis < 3; ++axis) {
                constants[axis][3] = light.color[axis];
                constants[axis + 3][3] = position[axis];
            }
        }
    }
    return constants;
}

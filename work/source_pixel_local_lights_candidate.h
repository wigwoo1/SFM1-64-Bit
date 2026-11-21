#pragma once

#include <array>
#include <cstddef>
#include <stdexcept>

struct SourcePixelLocalLightCandidate {
    bool directional = false;
    std::array<float, 3> color{};
    std::array<float, 3> position{};
    std::array<float, 3> direction{};
};

// Six float4 constants used by the recovered model-pixel light upload.
// Lights 0–2 use RGB/XYZ pairs. Light 3 uses those six registers' W lanes.
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
                constants[axis * 2][3] = light.color[axis];
                constants[axis * 2 + 1][3] = position[axis];
            }
        }
    }
    return constants;
}

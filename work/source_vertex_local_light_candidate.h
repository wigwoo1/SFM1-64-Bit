#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

struct SourceVertexLocalLightCandidate {
    uint32_t type; // Submitted light type. 2 is directional and 3 is spot.
    std::array<float, 3> color, position, direction;
    std::array<float, 3> attenuation; // Constant, linear, quadratic.
    float falloff, innerCosine, outerCosine, inverseConeWidth;
};

using SourceVertexLocalLightRowsCandidate = std::array<std::array<float, 4>, 5>;

inline SourceVertexLocalLightRowsCandidate sourceVertexLocalLightRowsCandidate(
    const SourceVertexLocalLightCandidate& l) {
    const bool spot = l.type == 3;
    return {{{l.color[0], l.color[1], l.color[2], float(l.type == 2)},
             {l.direction[0], l.direction[1], l.direction[2], float(spot)},
             {l.position[0], l.position[1], l.position[2], 1.f},
             spot ? std::array<float, 4>{l.falloff, l.innerCosine, l.outerCosine, l.inverseConeWidth}
                  : std::array<float, 4>{0.f, 1.f, 1.f, 1.f},
             {l.attenuation[0], l.attenuation[1], l.attenuation[2], 0.f}}};
}

// Attenuation varying in the tested Phong vertex program. Valid finite light
// data, nonzero light-to-vertex distance and positive denominator are required.
inline float sourceVertexLocalLightAttenuationCandidate(
    const SourceVertexLocalLightCandidate& light, const std::array<float, 3>& worldPosition) {
    if (light.type == 2) return 1.f;
    const float x = worldPosition[0] - light.position[0];
    const float y = worldPosition[1] - light.position[1];
    const float z = worldPosition[2] - light.position[2];
    const float squared = (x*x + y*y) + z*z;
    const float inverseLength = 1.f / std::sqrt(squared);
    const float distance = squared * inverseLength;
    const auto& a = light.attenuation;
    const float distanceWeight = 1.f / ((a[0] + a[1]*distance) + a[2]*squared);
    if (light.type != 3) return distanceWeight;

    const auto& d = light.direction;
    const float cosine = (d[0]*(x*inverseLength) + d[1]*(y*inverseLength)) + d[2]*(z*inverseLength);
    const float ramp = (cosine - light.outerCosine) * light.inverseConeWidth;
    const float spot = std::min(std::pow(std::max(ramp, 0.0001f), light.falloff), 1.f);
    return distanceWeight * spot;
}

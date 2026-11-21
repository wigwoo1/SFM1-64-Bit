#pragma once

#include <array>
#include <cmath>

// Resolve a missing radius from the world-light attenuation coefficients.
// The map loader supplies its HDR flag and already-initialized coefficients.
inline float sourceWorldLightFallbackRadiusCandidate(float radius, bool hdr,
    const std::array<float, 3>& intensity, float constant, float linear, float quadratic) {
    if (radius != 0) return radius;
    const double red = intensity[0], green = intensity[1], blue = intensity[2];
    const float magnitude = static_cast<float>(std::sqrt((red * red + green * green) + blue * blue));
    const float threshold = hdr ? 0.015f : 0.03f;
    if (quadratic == 0 && linear == 0) return 2000.f;

    const float target = magnitude / threshold;
    if (quadratic == 0) return (target - constant) / linear;

    const float discriminant = linear * linear - (constant - target) * (quadratic * 4.f);
    if (discriminant < 0) return 2000.f;
    const double root = (std::sqrt(static_cast<double>(discriminant)) - linear) / (2.0 * quadratic);
    return root < 0 ? 0.f : static_cast<float>(root);
}

#pragma once

#include "model_render_inputs_candidate.h"
#include "source_hdr_sky_lights_candidate.h"

enum class ModelWarpDrawRouteCandidate { Diffuse, Normal, Exponent };

struct ModelWarpDrawInputsCandidate {
    ModelWarpDrawRouteCandidate route = ModelWarpDrawRouteCandidate::Diffuse;
    std::array<float, 10> phong{};
    std::array<float, 24> light{};
};

// Current experimental renderer inputs. The availability flags describe views
// created on the logical device, not just textures named in a material.
inline std::optional<ModelWarpDrawInputsCandidate> prepareModelWarpDrawInputsCandidate(
    const ModelRenderMaterialCandidate& material, float alpha, bool warpAvailable, bool normalAvailable,
    bool exponentAvailable, const std::array<std::array<float, 3>, 6>& cube,
    const SourceSkyLightCandidate& sky, bool skyVisible, const std::array<float, 3>& camera,
    float exposure = 1.f) {
    if (alpha != 1 || material.translucent || material.alphaCutoff || material.selfIllum.enabled ||
        !warpAvailable) return {};

    ModelWarpDrawInputsCandidate result;
    const auto& inputs = material.lightwarpInputs;
    const auto exponent = inputs.phongExponentControl.value_or(0);
    const bool textureExponent = exponentAvailable && inputs.phongExponentControl == 0.f &&
        inputs.scalarSkyPhong && inputs.scalarSkyPhong->useBaseAlpha && !normalAvailable;
    if (normalAvailable) result.route = ModelWarpDrawRouteCandidate::Normal;
    if (textureExponent) result.route = ModelWarpDrawRouteCandidate::Exponent;

    if (inputs.scalarSkyPhong && (inputs.scalarSkyPhong->useBaseAlpha || normalAvailable) &&
        (textureExponent || exponent > 0)) {
        const auto& controls = *inputs.scalarSkyPhong;
        std::copy(controls.fresnelBoost.begin(), controls.fresnelBoost.end(), result.phong.begin());
        // The sign encodes the current shader's base-alpha mask selection.
        if (controls.useBaseAlpha) result.phong[3] = -result.phong[3];
        std::copy(camera.begin(), camera.end(), result.phong.begin() + 4);
        result.phong[7] = exponent;
        std::copy_n(controls.rim.begin(), 2, result.phong.begin() + 8);
    }
    if (!std::isfinite(exposure) || exposure <= 0.f)
        throw std::runtime_error("invalid model lightwarp exposure");
    // The current lightwarp shaders are homogeneous in ambient and sky RGB.
    // Scaling those inputs gives exposed linear RGB while preserving the push
    // constant layout, sky direction, material controls and output alpha.
    auto expose = [exposure](float energy) {
        if (exposure == 1.f) return energy;
        const float value = energy * exposure;
        if (!std::isfinite(value)) throw std::runtime_error("model lightwarp exposure overflow");
        return value;
    };
    for (size_t face = 0; face < cube.size(); ++face) {
        for (size_t channel = 0; channel < cube[face].size(); ++channel)
            result.light[4 * face + channel] = expose(cube[face][channel]);
        result.light[4 * face + 3] = face < 3 ? sky.towardLight[face]
            : (skyVisible ? expose(sky.intensity[face - 3]) : 0.f);
    }
    return result;
}

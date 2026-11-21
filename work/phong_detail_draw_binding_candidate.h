#pragma once

#include "material_typed_dispatch_candidate.h"
#include "phong_detail_uv_candidate.h"
#include "model_prepared_texture_candidate.h"
#include <cmath>
#include <cstring>

struct PhongDetailDrawBindingCandidate {
    MaterialTextureReferenceCandidate texture;
    uint32_t frame;
    float blendFactor;
    std::array<float, 8> uvRows;
};

// Project the ordinary mode-6 inputs from one resolved material snapshot.
// Shader variant selection and texture sampling remain the renderer's job.
inline std::optional<PhongDetailDrawBindingCandidate> resolvePhongDetailDrawBindingCandidate(
    const std::vector<MaterialTypedDispatchCandidate>& parameters) {
    const auto lookup = [&](const char* name) -> const MaterialTypedDispatchCandidate* {
        for (auto value = parameters.rbegin(); value != parameters.rend(); ++value)
            if (value->name == name) return &*value;
        return nullptr;
    };
    const auto* mode = lookup("$detailblendmode");
    const auto* frame = lookup("$detailframe");
    const auto* blend = lookup("$detailblendfactor");
    const auto* texture = lookup("$detail");
    if (!mode || !frame || !blend || !texture || !mode->created || !frame->created || !blend->created ||
        (mode->value.numeric.typeAndMetadata & 15u) != 4 || mode->value.numeric.integer != 6 ||
        (frame->value.numeric.typeAndMetadata & 15u) != 4 ||
        (blend->value.numeric.typeAndMetadata & 15u) != 0 ||
        !materialTypedTextureValidCandidate(*texture) || !texture->texture.resource()) return {};

    const auto frameIndex = frame->value.numeric.integer;
    const auto floatValue = [](uint32_t bits) {
        float value;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    };
    const auto factor = floatValue(blend->value.numeric.vector[0]);
    if (frameIndex >= texture->texture.resource()->layout().frameCount ||
        !std::isfinite(factor) || factor < 0 || factor > 1) return {};
    const auto* transform = lookup("$detailtexturetransform");
    if (!transform || !transform->created || (transform->value.numeric.typeAndMetadata & 15u) == 6)
        transform = lookup("$basetexturetransform");
    std::array<float, 8> matrix{1, 0, 0, 0, 0, 1, 0, 0};
    if (transform && (transform->value.numeric.typeAndMetadata & 15u) == 7) {
        if (!transform->created || !transform->matrix) return {};
        for (size_t i = 0; i < matrix.size(); ++i) matrix[i] = floatValue((*transform->matrix)[i]);
    }
    std::array<float, 2> scale{1, 1};
    const auto* scaleInput = lookup("$detailscale");
    if (scaleInput && scaleInput->created) {
        const auto type = scaleInput->value.numeric.typeAndMetadata & 15u;
        if (type != 6) scale.fill(floatValue(scaleInput->value.numeric.vector[0]));
        if (type == 2) scale[1] = floatValue(scaleInput->value.numeric.vector[1]);
    }
    const auto uvRows = scalePhongDetailUvCandidate(matrix, scale);
    if (!uvRows) return {};
    return PhongDetailDrawBindingCandidate{texture->texture, frameIndex, factor, *uvRows};
}

inline std::optional<ModelPreparedPhongDetailCandidate> preparePhongDetailDrawTextureCandidate(
    const PhongDetailDrawBindingCandidate& binding) {
    const auto& resource = binding.texture.resource();
    if (!resource || binding.frame >= resource->layout().frameCount) return {};
    auto prepared = prepareModelTextureSnapshotCandidate(resource, resource->name(), 0, binding.frame);
    if (!prepared.cpu) return {};
    return ModelPreparedPhongDetailCandidate{std::move(prepared), binding.uvRows, binding.blendFactor};
}

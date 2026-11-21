#pragma once

#include "material_animated_texture_frame_candidate.h"
#include "material_burnlevel_puppet_proxy_candidate.h"
#include <charconv>
#include <cstring>

class MaterialAnimatedTextureBindProxyCandidate final : public MaterialProxyInstanceCandidate {
    MaterialProxyBindingCandidate texture, result;
    float rate = 15.0f;
    bool noWrap = false;

    template<class T>
    static bool parseOptional(const SourceMaterialPatchNodeCandidate& declaration, const char* key, T& value) {
        const auto* field = sourceMaterialPatchFindConstCandidate(declaration, key);
        if (!field) return true;
        if (field->kind != 1 || field->text.empty()) return false;
        const auto* end = field->text.data() + field->text.size();
        const auto parsed = std::from_chars(field->text.data(), end, value);
        return parsed.ec == std::errc{} && parsed.ptr == end;
    }

    static float floatFromBits(uint32_t bits) noexcept {
        float value;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }

public:
    bool initialize(const MaterialProxyContextCandidate& context,
                    const SourceMaterialPatchNodeCandidate& declaration) override {
        const auto* source = sourceMaterialPatchFindConstCandidate(declaration, "animatedTextureVar");
        const auto* destination = sourceMaterialPatchFindConstCandidate(declaration, "animatedTextureFrameNumVar");
        if (!source || !destination || source->kind != 1 || destination->kind != 1 ||
            source->text.empty() || destination->text.empty()) return false;
        texture = context.find(source->text);
        result = context.find(destination->text);
        if (!texture.valid() || !result.valid()) return false;
        float configuredRate = 15.0f;
        int32_t configuredNoWrap = 0;
        if (!parseOptional(declaration, "animatedTextureFrameRate", configuredRate) ||
            !parseOptional(declaration, "animationNoWrap", configuredNoWrap) ||
            !std::isfinite(configuredRate) || configuredRate < 0) return false;
        rate = configuredRate;
        noWrap = configuredNoWrap != 0;
        return true;
    }

    bool evaluate(MaterialProxyEvaluationCandidate& bind) override {
        const auto time = bind.currentTime();
        const auto duration = bind.frameDuration();
        const auto source = bind.read(texture);
        if (!time || !duration || !source || !materialTypedTextureValidCandidate(*source) ||
            !source->texture.resource()) return false;
        const auto frame = selectMaterialAnimatedTextureFrameCandidate(
            floatFromBits(*time), floatFromBits(*duration), rate,
            source->texture.resource()->layout().frameCount, noWrap);
        // Ordinary AnimatedTexture has a no-op wrap callback.
        return frame && bind.setInt(result, static_cast<uint32_t>(frame->frame));
    }
};

// Opt in only when the draw producer supplies an explicit clock and retains
// the effective ordinary texture for the material binding.
struct MaterialPinnedAnimatedDetailProxyFactoryCandidate : MaterialPinnedPuppetDetailProxyFactoryCandidate {
    static MaterialPinnedProxyClassificationCandidate classification(const std::string& name) {
        if (admitted(name) && sourceMaterialPatchLowerCandidate(name) == "animatedtexture")
            return MaterialPinnedProxyClassificationCandidate::Implemented;
        return MaterialPinnedPuppetDetailProxyFactoryCandidate::classification(name);
    }

    MaterialProxyInstanceCandidate* create(const std::string& name) override {
        if (admitted(name) && sourceMaterialPatchLowerCandidate(name) == "animatedtexture")
            return new MaterialAnimatedTextureBindProxyCandidate;
        return MaterialPinnedPuppetDetailProxyFactoryCandidate::create(name);
    }
};

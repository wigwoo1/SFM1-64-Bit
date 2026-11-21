#pragma once
#include "material_pinned_client_ifm_proxy_factory_candidate.h"

// BurnLevel's ordinary IFM puppet branch writes zero to the detail blend
// factor because the renderable has no game entity.
class MaterialBurnLevelPuppetProxyCandidate final : public MaterialProxyInstanceCandidate {
    MaterialProxyBindingCandidate result;
public:
    bool initialize(const MaterialProxyContextCandidate& context,
                    const SourceMaterialPatchNodeCandidate& declaration) override {
        const auto* field = sourceMaterialPatchFindConstCandidate(declaration, "resultVar");
        if (!field || field->kind != 1 ||
            sourceMaterialPatchLowerCandidate(field->text) != "$detailblendfactor") return false;
        result = context.find(field->text);
        return result.valid();
    }

    bool evaluate(MaterialProxyEvaluationCandidate& frame) override {
        return frame.verifiedPuppetWithoutGameEntity() && frame.setFloat(result, 0);
    }
};

// Explicit profile for consumers with a checked puppet bind context.
struct MaterialPinnedPuppetDetailProxyFactoryCandidate : MaterialPinnedClientIfmProxyFactoryCandidate {
    static MaterialPinnedProxyClassificationCandidate classification(const std::string& name) {
        if (admitted(name) && sourceMaterialPatchLowerCandidate(name) == "burnlevel")
            return MaterialPinnedProxyClassificationCandidate::Implemented;
        return MaterialPinnedClientIfmProxyFactoryCandidate::classification(name);
    }

    MaterialProxyInstanceCandidate* create(const std::string& name) override {
        if (admitted(name) && sourceMaterialPatchLowerCandidate(name) == "burnlevel")
            return new MaterialBurnLevelPuppetProxyCandidate;
        return MaterialPinnedClientIfmProxyFactoryCandidate::create(name);
    }
};

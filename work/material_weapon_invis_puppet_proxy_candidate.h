#pragma once
#include "material_sine_proxy_candidate.h"

// Measured weapon_invis null-game-entity branch only. Other renderable/entity
// kinds remain unsupported; no cloak/limit/owner/viewer values are fabricated.
class MaterialWeaponInvisPuppetProxyCandidate:public MaterialProxyInstanceCandidate {
    MaterialProxyBindingCandidate result;
public:
    bool initialize(const MaterialProxyContextCandidate&context,const SourceMaterialPatchNodeCandidate&)override{
        result=context.find("$cloakfactor");return result.valid();
    }
    bool evaluate(MaterialProxyEvaluationCandidate&frame)override{
        // Confirm the binding belongs to this frame, including reinitialization
        // lifetime. This path intentionally makes no setter or journal entry.
        return frame.read(result).has_value()&&frame.verifiedPuppetWithoutGameEntity();
    }
};
// Separate explicit opt-in. The existing general factory and renderer admission
// policy are unchanged; accepting a declaration does not prove its whole shader.
struct MaterialPuppetFunctionProxyFactoryCandidate:MaterialFunctionProxyFactoryCandidate {
    MaterialProxyInstanceCandidate*create(const std::string&name)override{
        return name=="weapon_invis"?new MaterialWeaponInvisPuppetProxyCandidate:MaterialFunctionProxyFactoryCandidate::create(name);
    }
};

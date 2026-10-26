#pragma once
#include "source_material_patch_policy_candidate.h"

// Value-owned declaration passed by the original builtin builder to its
// material-creation service. This does not construct an IMaterial or texture.
struct BuiltinErrorMaterialSeedCandidate{
    std::string name;
    SourceMaterialPatchNodeCandidate source;
};
inline BuiltinErrorMaterialSeedCandidate builtinErrorMaterialSeedCandidate(){
    using Node=SourceMaterialPatchNodeCandidate;
    BuiltinErrorMaterialSeedCandidate result;result.name="___error.vmt";result.source.key="UnlitGeneric";
    auto integer=[&](const char*key,uint32_t value){Node n;n.key=key;n.kind=2;n.bits=value;result.source.children.push_back(std::move(n));};
    integer("$model",1);
    Node scale;scale.key="$decalscale";scale.kind=3;const float value=0.05f;std::memcpy(&scale.bits,&value,4);result.source.children.push_back(std::move(scale));
    Node texture;texture.key="$basetexture";texture.kind=1;texture.text="error";result.source.children.push_back(std::move(texture));
    integer("$gammacolorread",1);integer("$linearwrite",1);return result;
}

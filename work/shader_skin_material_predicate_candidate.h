#pragma once
#include <cstdint>
#include <optional>
// A snapshot of EFFECTIVE, TYPED native inputs, not raw VMT strings.
// optional absence means info field == -1 (undeclared), NOT unknown/unloaded.
// Original stdshader_dx9 10054420, with mat_phong explicitly supplied.
struct ShaderSkinMaterialInputsCandidate {
    int32_t matPhong=0;
    std::optional<int32_t>phong;
    std::optional<uint8_t>diffuseWarpTypeByte;
    std::optional<int32_t>baseAlphaPhongMask;
    std::optional<bool>bumpDefined;
};
struct ShaderSkinMaterialResultCandidate {bool wantsSkin=false,bumpQueried=false;};
inline ShaderSkinMaterialResultCandidate shaderSkinMaterialPredicateCandidate(const ShaderSkinMaterialInputsCandidate&in){
    if(!in.matPhong||!in.phong||!*in.phong)return {};
    if(in.diffuseWarpTypeByte&&((*in.diffuseWarpTypeByte&15)==3))return {true,false};
    if(!in.baseAlphaPhongMask||*in.baseAlphaPhongMask==1)return {true,false};
    if(!in.bumpDefined)return {};
    return {*in.bumpDefined,true};
}
// Checked caller-prefix rule only. Does not identify VS20/VS30/static/dynamic
// combos or execute the chosen draw routine. Hardware input is not inferred.
inline bool shaderSkinDrawGateCandidate(const ShaderSkinMaterialResultCandidate&predicate,int32_t hardwareLevel){
    return predicate.wantsSkin&&hardwareLevel>=92;
}

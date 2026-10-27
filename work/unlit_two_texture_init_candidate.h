#pragma once
#include "unlit_two_texture_parameter_schema_candidate.h"
#include "material_parameter_completion_candidate.h"
// Actual20-field UnlitTwoTexture_DX9 CPU InitParams. No texture/resource Init,
// cloak draw, additive blending or material proxy behavior is implemented here.
inline bool unlitTwoTextureCompletionCandidate(MaterialParameterCompletionCandidate&out,
        const std::vector<MaterialParameterCompletedValueCandidate>&input,const std::vector<uint32_t>&types){
    if(types!=unlitTwoTextureParameterTypesCandidate()||input.size()<20)return false;
    return materialParameterCompletionCandidate(out,input,types,[](auto&v){
        auto defined=[&](size_t i){return (v.at(i).value.value.numeric.typeAndMetadata&15)!=6;};
        auto integer=[&](size_t i){return v.at(i).value.value.numeric.integer;};
        auto set=[&](size_t i,uint32_t op,std::array<uint32_t,4>a={}){return materialParameterCompletionSetCandidate(v.at(i),op,a);};
        if(!set(2,0,{integer(2)|0x40000u,0,0,0}))return false;
        if(!defined(16))return set(16,0,{});
        if(integer(16)==0)return true;
        // The enabled cloak helper adds model/tangent requirements before
        // defaulting the three declared effect fields. Authored types survive.
        if(!set(2,0,{integer(2)|0x40000u,0,0,0})||!set(0,0,{integer(0)|0x800u,0,0,0})||!set(2,0,{integer(2)|0x40u,0,0,0}))return false;
        if(!defined(17)&&!set(17,1,{}))return false;
        if(!defined(19)&&!set(19,1,{0x3dcccccdu,0,0,0}))return false;
        if(!defined(18)&&!set(18,4,{0x3f800000,0x3f800000,0x3f800000,0x3f800000}))return false;
        return true;
    });
}

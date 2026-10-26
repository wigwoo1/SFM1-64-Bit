#pragma once
#include "unlit_generic_parameter_schema_candidate.h"
#include "material_parameter_completion_candidate.h"
#include "vertexlit_shared_init_candidate.h" // Existing shared settings carrier only.

// Actual UnlitGeneric mapping + concrete InitParams, before texture/resource
// Init. Its mapping declares no normal-map/Phong/light-warp initializer fields:
// similarly named extra variables do not turn those fields into declarations.
// This does not call the fixed125-field VertexLitGeneric implementation.
struct UnlitGenericCompletionCandidate {
    MaterialParameterCompletionCandidate completed;
    uint32_t warnings=0;
};
inline bool unlitGenericCompletionCandidate(UnlitGenericCompletionCandidate&out,
        const std::vector<MaterialParameterCompletedValueCandidate>&input,
        const std::vector<uint32_t>&types,const VertexLitInitSettingsCandidate&s,int32_t /*hardwareLevel*/){
    if(types!=unlitGenericParameterTypesCandidate()||input.size()<68)return false;
    UnlitGenericCompletionCandidate result;
    try{
        if(!materialParameterCompletionCandidate(result.completed,input,types,[&](auto&v){
            auto defined=[&](size_t i){return (v.at(i).value.value.numeric.typeAndMetadata&15)!=6;};
            auto integer=[&](size_t i){return v.at(i).value.value.numeric.integer;};
            auto set=[&](size_t i,uint32_t op,std::array<uint32_t,4>args){
                if(!materialParameterCompletionSetCandidate(v.at(i),op,args))throw std::runtime_error("Unlit initializer setter domain");
            };
            auto si=[&](size_t i,uint32_t n){set(i,0,{n,0,0,0});};
            auto di=[&](size_t i,uint32_t n){if(!defined(i))si(i,n);};
            auto df=[&](size_t i,float n){if(!defined(i)){uint32_t b;std::memcpy(&b,&n,4);set(i,1,{b,0,0,0});}};
            di(67,0);df(25,0);di(26,0);
            if(!defined(22))set(22,3,{0x3f800000,0x3f800000,0x3f800000,0});
            di(18,0);df(38,1);di(64,0);df(16,4);
            if(!(integer(1)&0x80000000)&&s.forceVertexFog)si(0,integer(0)|0x80000000);
            // info+A8 is absent, so the shared Phong predicate is false even
            // when the schema's separate $PHONG variable is explicitly set.
            auto flashlight=v.at(9);flashlight.value.matrix.reset();flashlight.value.matrixIdentity=false;flashlight.nullResource=0;
            flashlight.value.value.numeric.typeAndMetadata=(flashlight.value.value.numeric.typeAndMetadata&~15u)|6;
            if(!materialStringSetCandidate(flashlight.value.value,7,{},0,"effects/flashlight001"))return false;
            v.at(9)=std::move(flashlight);
            // No bump field: no albedo-to-base copy, skin path or bump warnings.
            si(2,integer(2)|0x40000);si(0,integer(0)&~0x40u);
            di(20,0);df(23,0);df(24,1);
            df(44,0.5f);df(45,0.5f);df(48,1);df(55,1);
            if(!defined(6)){si(0,integer(0)&~0x40u);si(0,integer(0)&~0x100000u);}
            if(integer(0)&0x10000)si(0,integer(0)|2u);
            si(0,integer(0)&~0x400000u);
            if((s.configFlagsAt34&0x80)&&defined(17)&&defined(6))set(17,6,{});
            df(27,1);di(63,0);di(62,0);
            return true;
        }))return false;
    }catch(const std::runtime_error&){return false;}
    out=std::move(result);return true;
}

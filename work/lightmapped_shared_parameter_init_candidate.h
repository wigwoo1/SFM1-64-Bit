#pragma once
#include "material_parameter_completion_candidate.h"
#include "vertexlit_shared_init_candidate.h"
#include "source_material_patch_policy_candidate.h"

// Independent CPU InitParams policy. Slot5/6 retain their measured service
// identities until the renderer-side interface is reconstructed; these are
// supplied observations, not inferred GPU capabilities. No resources are loaded.
struct LightmappedInitSettingsCandidate {
    int32_t forceVertexFog=0,disableLightwarp=0,disableFancyBlending=0;
    uint32_t shaderInitSlot5=0,shaderInitSlot6=0;
    bool paintEnabled=false;
};
struct LightmappedSharedCompletionCandidate {
    MaterialParameterCompletionCandidate completed;
    uint32_t warnings=0;
};
inline bool lightmappedSharedCompletionCandidate(LightmappedSharedCompletionCandidate&out,
        const std::vector<MaterialParameterCompletedValueCandidate>&input,const std::vector<uint32_t>&types,
        const VertexLitInitSettingsCandidate&common,const LightmappedInitSettingsCandidate&s,
        const std::array<int32_t,68>&mapping){
    if(input.size()<types.size())return false;
    for(auto i:mapping)if(i < -1 || (i>=0&&size_t(i)>=types.size()))return false;
    LightmappedSharedCompletionCandidate result;
    try{
        if(!materialParameterCompletionCandidate(result.completed,input,types,[&](auto&v){
            // Semantic field IDs refer to the first measured shared-helper mapping,
            // never an array padded to68. Each access reaches the actual family's
            // registered array; absent fields cannot capture dynamic extra vars.
            auto declared=[&](size_t i){return mapping.at(i)>=0;};
            auto at=[&](size_t i)->MaterialParameterCompletedValueCandidate&{if(!declared(i))throw std::runtime_error("Undeclared shared parameter access");return v.at(size_t(mapping.at(i)));};
            auto defined=[&](size_t i){return declared(i)&&(at(i).value.value.numeric.typeAndMetadata&15)!=6;};
            auto integer=[&](size_t i){return at(i).value.value.numeric.integer;};
            auto set=[&](size_t i,uint32_t op,std::array<uint32_t,4>a={}){if(!declared(i))return;if(!materialParameterCompletionSetCandidate(at(i),op,a))throw std::runtime_error("Lightmapped setter domain");};
            auto si=[&](size_t i,uint32_t n){set(i,0,{n,0,0,0});};
            auto di=[&](size_t i,uint32_t n){if(!defined(i))si(i,n);};
            auto df=[&](size_t i,uint32_t bits){if(!defined(i))set(i,1,{bits,0,0,0});};
            auto string=[&](size_t i){const auto&x=at(i).value.value;if((x.numeric.typeAndMetadata&15)!=1)throw std::runtime_error("Lightmapped GetString conversion unverified");return x.text;};
            auto ss=[&](size_t i,const std::string&text){auto n=at(i);n.value.matrix.reset();n.value.matrixIdentity=false;n.nullResource=0;n.value.value.numeric.typeAndMetadata=(n.value.value.numeric.typeAndMetadata&~15u)|6u;if(!materialStringSetCandidate(n.value.value,7,{},0,text))throw std::runtime_error("Lightmapped string setter domain");at(i)=std::move(n);};
            if(!(integer(1)&0x80000000u)&&s.forceVertexFog)si(0,integer(0)|0x80000000u);
            ss(9,"effects/flashlight001");
            if(!(common.configFlagsAt34&0x100)&&defined(28)&&defined(13)&&defined(6)&&(!defined(34)||integer(34)==0))ss(6,string(13));
            if((s.shaderInitSlot5&255u)&&defined(22)&&!(s.shaderInitSlot6&255u)&&sourceMaterialPatchLowerCandidate(string(22))=="env_cubemap"){
                ++result.warnings;set(22,6);
            }
            if(s.disableLightwarp)set(44,6);
            if(s.disableFancyBlending)set(45,6);
            if(!defined(27))set(27,3,{0x3f800000,0x3f800000,0x3f800000,0});
            di(34,0);
            if(!defined(14))set(14,3,{0x3f800000,0x3f800000,0x3f800000,0});
            df(17,0x40800000);
            if(!defined(21))set(21,4,{0x3f800000,0x3f800000,0x3f800000,0x3f800000});
            df(20,0x3f800000);di(19,0);df(33,0x3f800000);
            di(25,0);di(23,0);di(29,0);di(16,0);df(31,0);df(32,0x3f800000);df(50,0);
            if(!defined(6)){si(0,integer(0)&~0x40u);si(0,integer(0)&~0x100000u);}
            if(defined(28))set(24,6);
            if(integer(0)&0x10000)si(0,integer(0)|2u);
            si(2,integer(2)|4u);
            if(!(common.configFlagsAt34&0x100)&&defined(28)&&integer(34)==0)si(2,integer(2)|8u);
            if((common.configFlagsAt34&0x80)&&defined(22)&&defined(6))set(22,6);
            di(41,0);di(42,0);di(48,0);df(52,0x3f000000);df(53,0x3f000000);
            df(56,0x3f800000);df(62,0x3dcccccd);di(63,0);df(64,0);
            if(s.paintEnabled){if(declared(66)&&!defined(66))ss(66,"paint/splatnormal_default");if(declared(67)&&!defined(67))ss(67,defined(22)?string(22):std::string("env_cubemap"));}
            else{set(66,6);set(67,6);}
            return true;
        }))return false;
    }catch(const std::runtime_error&){return false;}
    out=std::move(result);return true;
}

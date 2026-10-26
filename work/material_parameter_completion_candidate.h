#pragma once
#include "material_parameter_collection_candidate.h"

// Material-system InitParams surrounding stage only. The caller must supply
// the actual shader initializer separately; descriptor default TEXT is not used
// by this stage. No texture, nonnull material or arbitrary FourCC resource.
struct MaterialParameterCompletedValueCandidate {
    MaterialTypedDispatchCandidate value;
    uint32_t nullResource=0; // 0 ordinary, 1 FourCC(0,null), 2 material(null)
};
struct MaterialParameterCompletionCandidate {
    std::vector<MaterialParameterCompletedValueCandidate>values;
    std::vector<uint32_t>descriptorLookups;
};
inline bool materialParameterCompletionValueValidCandidate(const MaterialParameterCompletedValueCandidate&v){
    const auto&t=v.value;const auto type=t.value.numeric.typeAndMetadata&15;
    if(!t.created||t.name.size()>1023||t.value.text.size()>4095)return false;
    if(type==5||type==8)return v.nullResource==(type==5?1u:2u)&&!t.matrix&&t.value.text.empty();
    return !v.nullResource&&(materialStringDomainCandidate(type)||type==7)&&
        (bool(t.matrix)==(type==7))&&(type==1||t.value.text.empty());
}
// Used by explicit initializer callbacks too. Unsupported operations preserve
// the entire value; clearing a matrix/null resource preserves numeric caches.
inline bool materialParameterCompletionSetCandidate(MaterialParameterCompletedValueCandidate&out,
        uint32_t operation,const std::array<uint32_t,4>&arguments={}){
    if(!materialParameterCompletionValueValidCandidate(out)||operation>6||operation==5)return false;
    auto next=out;
    if(next.value.matrix||next.nullResource){
        next.value.matrix.reset();next.value.matrixIdentity=false;next.nullResource=0;
        next.value.value.numeric.typeAndMetadata=(next.value.value.numeric.typeAndMetadata&~15u)|6;
    }
    if(!materialStringSetCandidate(next.value.value,operation,arguments))return false;
    out=std::move(next);return true;
}
template<class Initializer>
bool materialParameterCompletionCandidate(MaterialParameterCompletionCandidate&out,
        const std::vector<MaterialParameterCompletedValueCandidate>&input,
        const std::vector<uint32_t>&types,Initializer initialize){
    if(types.size()<6||types.size()>256||input.size()<types.size()||input.size()>256)return false;
    for(const auto&v:input)if(!materialParameterCompletionValueValidCandidate(v))return false;
    MaterialParameterCompletionCandidate result;result.values=input;
    if(!initialize(result.values)||result.values.size()!=input.size())return false;
    for(size_t i=0;i<input.size();++i)if(!materialParameterCompletionValueValidCandidate(result.values[i])||result.values[i].value.name!=input[i].value.name)return false;
    auto undefined=[](const auto&v){return (v.value.value.numeric.typeAndMetadata&15)==6;};
    const std::array<uint32_t,4>white{0x3f800000,0x3f800000,0x3f800000,0};
    if(undefined(result.values[4])&&!materialParameterCompletionSetCandidate(result.values[4],3,white))return false;
    if(undefined(result.values[5])&&!materialParameterCompletionSetCandidate(result.values[5],1,white))return false;
    for(size_t i=types.size();i-->0;){
        auto&v=result.values[i];if(!undefined(v))continue;
        result.descriptorLookups.push_back(uint32_t(i));
        uint32_t op=6;std::array<uint32_t,4>args{};
        switch(types[i]){
        case 1:case 8:op=0;break;
        case 2:op=3;args=white;break;
        case 3:op=2;break;
        case 4:op=3;break;
        case 5:op=4;break;
        case 7:op=1;break;
        case 9:case 11:
            v.nullResource=types[i]==9?1u:2u;
            v.value.value.numeric.typeAndMetadata=(v.value.value.numeric.typeAndMetadata&~15u)|(types[i]==9?5u:8u);
            v.value.value.numeric.integer=0;v.value.value.numeric.vector.fill(0);continue;
        case 10:
            v.value.matrix.emplace();v.value.matrix->fill(0);
            for(size_t d=0;d<4;++d)(*v.value.matrix)[5*d]=0x3f800000;
            v.value.matrixIdentity=true;v.value.value.numeric.typeAndMetadata=(v.value.value.numeric.typeAndMetadata&~15u)|7;
            v.value.value.numeric.integer=0;v.value.value.numeric.vector.fill(0);continue;
        default:continue; // Texture0, envmap6 and unknown types stay Undefined.
        }
        if(!materialParameterCompletionSetCandidate(v,op,args))return false;
    }
    out=std::move(result);return true;
}

#pragma once
#include "material_typed_dispatch_candidate.h"
#include "material_vector_component_candidate.h"

inline bool materialMatrixIdentityValueCandidate(const MaterialMatrixMathCandidate&matrix){
    for(size_t i=0;i<16;++i)if(!materialNumericOrderedEqualCandidate(matrix[i],i%5==0?0x3f800000u:0u))return false;
    return true;
}
// Owned value storage, not allocator/resource/notification ABI. The original
// setter always replaces its65-byte matrix backing, including equal inputs.
inline bool materialTypedMatrixSetCandidate(MaterialTypedDispatchCandidate&value,const MaterialMatrixMathCandidate&matrix){
    const auto type=value.value.numeric.typeAndMetadata&15u;
    if(!materialStringDomainCandidate(type)&&type!=7)return false;
    if(type==7&&!value.matrix)return false;
    auto next=value;next.matrix=matrix;next.matrixIdentity=materialMatrixIdentityValueCandidate(matrix);next.value.text.clear();
    next.value.numeric.typeAndMetadata=(next.value.numeric.typeAndMetadata&~15u)|7u;
    next.value.numeric.integer=0;next.value.numeric.vector.fill(0);value=std::move(next);return true;
}
inline bool materialTypedNumericWriteCandidate(MaterialTypedDispatchCandidate&value,uint32_t operation,
        const std::array<uint32_t,4>&arguments={},uint32_t count=0,std::string_view text={}){
    if(operation==8)return materialVectorComponentCandidate(value.value.numeric,count,arguments[0]);
    if((value.value.numeric.typeAndMetadata&15u)!=7)return materialStringSetCandidate(value.value,operation,arguments,count,text);
    // Detach a value-owned matrix before ordinary numeric/string writes. Stage
    // everything so invalid operations and throwing allocations retain value.
    auto next=value;
    if((next.value.numeric.typeAndMetadata&15u)==7){
        if(!next.matrix)return false;next.matrix.reset();next.matrixIdentity=false;
        next.value.numeric.typeAndMetadata=(next.value.numeric.typeAndMetadata&~15u)|6u;
    }
    if(!materialStringSetCandidate(next.value,operation,arguments,count,text))return false;
    value=std::move(next);return true;
}
inline std::optional<MaterialMatrixMathCandidate>materialTypedMatrixReadCandidate(const MaterialTypedDispatchCandidate&value){
    if((value.value.numeric.typeAndMetadata&15u)==7)return value.matrix;
    if(!materialStringDomainCandidate(value.value.numeric.typeAndMetadata))return {};
    MaterialMatrixMathCandidate identity{};identity[0]=identity[5]=identity[10]=identity[15]=0x3f800000;return identity;
}
inline bool materialTypedMatrixIsIdentityCandidate(const MaterialTypedDispatchCandidate&value){
    return (value.value.numeric.typeAndMetadata&15u)!=7||value.matrixIdentity;
}

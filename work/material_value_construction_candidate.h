#pragma once
#include "material_constructor_decimal_candidate.h"
#include <cmath>

// Standalone constructor candidate; production integration is a separate gate.
// Value construction is distinct from subsequent assignments. Owner/name
// registration and other resource constructors are separate, unverified work.
enum class MaterialValueConstructionKindCandidate:uint32_t{Integer,Float,Vector,String,Undefined};
inline uint32_t materialConstructorStringIntegerCandidate(double v,bool sse2){
    if(sse2){
        if(!std::isfinite(v)||v<-2147483648.0||v>=2147483648.0)return 0x80000000u;
        return uint32_t(int32_t(v));
    }
    // Original non-SSE _ftol2 path returns EDX:EAX; constructor retains EAX.
    if(!std::isfinite(v)||v< -9223372036854775808.0||v>=9223372036854775808.0)return 0;
    return uint32_t(int64_t(v));
}
inline bool materialValueConstructCandidate(MaterialStringValueCandidate&out,
        MaterialValueConstructionKindCandidate kind,const std::array<uint32_t,4>&arguments={},
        uint32_t count=0,std::string_view input={},bool sse2=true){
    const auto k=uint32_t(kind);if(k>4)return false;
    if(k==2){if(count<1||count>4)return false;}else if(count)return false;
    if(k!=3&&!input.empty())return false;
    if(input.size()>4095)return false;
    MaterialStringValueCandidate value;
    // Packed test/value metadata: no name symbol, no temporary queue index,
    // native constructor default component count4. No x86 pointer identity.
    value.numeric.typeAndMetadata=0xffffff46;
    if(k==3){
        if(!materialStringSetCandidate(value,7,{},0,input))return false;
        const double parsed=materialConstructorDoubleCandidate(value.text);
        value.numeric.integer=materialConstructorStringIntegerCandidate(parsed,sse2);
        const float scalar=static_cast<float>(parsed);uint32_t bits=0;std::memcpy(&bits,&scalar,4);
        value.numeric.vector.fill(bits);
    }else if(k!=4){
        const auto op=k==0?MaterialNumericOpCandidate::SetInt:
            k==1?MaterialNumericOpCandidate::SetFloat:MaterialNumericOpCandidate::SetVectorArray;
        if(!materialNumericSetCandidate(value.numeric,op,arguments,count))return false;
    }
    out=std::move(value);return true;
}

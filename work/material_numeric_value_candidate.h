#pragma once
#include <array>
#include <cstdint>

// Owned VALUE behavior of installed immediate, ownerless, resource-free
// CMaterialVar setters. Not queued updates, resource ownership or notifications.
struct MaterialNumericValueCandidate{
    uint32_t typeAndMetadata=6,integer=0;
    std::array<uint32_t,4>vector{}; // Exact float32 bits, including NaN payloads.
};
enum class MaterialNumericOpCandidate:uint32_t{
    SetInt=0,SetFloat=1,SetVector2=2,SetVector3=3,SetVector4=4,SetVectorArray=5,SetUndefined=6
};
inline bool materialNumericResourceFreeCandidate(uint32_t typeAndMetadata){
    const auto t=typeAndMetadata&15;return t==0||t==2||t==4||t==6;
}
// CVTTSS2SI: truncation, and integer-indefinite on NaN/infinity/overflow.
inline uint32_t materialNumericTruncateCandidate(uint32_t bits){
    const unsigned exponent=(bits>>23)&255;const bool negative=(bits>>31)!=0;
    if(exponent==255)return 0x80000000u;
    const int e=int(exponent)-127;if(e<0)return 0;if(e>31)return 0x80000000u;
    const uint64_t significand=(bits&0x7fffffu)|0x800000u;
    const uint64_t magnitude=e>=23?significand<<unsigned(e-23):significand>>unsigned(23-e);
    if(magnitude>(negative?0x80000000ull:0x7fffffffull))return 0x80000000u;
    return negative?0u-uint32_t(magnitude):uint32_t(magnitude);
}
// CVTSI2SS with round-to-nearest-even, independent of host FP environment.
inline uint32_t materialNumericIntFloatCandidate(uint32_t integer){
    if(!integer)return 0;
    const uint32_t sign=integer&0x80000000u,magnitude=sign?0u-integer:integer;
    unsigned top=31;while(!(magnitude&(uint32_t(1)<<top)))--top;
    uint32_t significand=0;
    if(top<=23)significand=magnitude<<(23-top);
    else{
        const unsigned shift=top-23;significand=magnitude>>shift;
        const uint32_t remainder=magnitude&((uint32_t(1)<<shift)-1),half=uint32_t(1)<<(shift-1);
        if(remainder>half||(remainder==half&&(significand&1)))++significand;
        if(significand==0x1000000u){significand>>=1;++top;}
    }
    return sign|((top+127)<<23)|(significand&0x7fffffu);
}
inline bool materialNumericOrderedEqualCandidate(uint32_t a,uint32_t b){
    const auto nan=[](uint32_t n){return (n&0x7f800000u)==0x7f800000u&&(n&0x7fffffu);};
    if(nan(a)||nan(b))return false;
    return a==b||(!(a&0x7fffffffu)&&!(b&0x7fffffffu));
}
// False means unsupported domain, with no mutation. Valid array count is1..4.
inline bool materialNumericSetCandidate(MaterialNumericValueCandidate&v,MaterialNumericOpCandidate op,
        const std::array<uint32_t,4>&arguments,uint32_t count=0){
    const auto operation=uint32_t(op),oldType=v.typeAndMetadata&15;
    if(!materialNumericResourceFreeCandidate(v.typeAndMetadata)||operation>6)return false;
    if(op==MaterialNumericOpCandidate::SetVectorArray){if(count<1||count>4)return false;}
    else if(count)return false;
    if(op==MaterialNumericOpCandidate::SetInt){
        if(oldType==4&&v.integer==arguments[0])return true;
        v.integer=arguments[0];v.vector.fill(materialNumericIntFloatCandidate(arguments[0]));
        v.typeAndMetadata=(v.typeAndMetadata&~15u)|4;return true;
    }
    if(op==MaterialNumericOpCandidate::SetFloat){
        if(oldType==0&&materialNumericOrderedEqualCandidate(v.vector[0],arguments[0]))return true;
        v.vector.fill(arguments[0]);v.integer=materialNumericTruncateCandidate(arguments[0]);
        v.typeAndMetadata&=~15u;return true;
    }
    if(op==MaterialNumericOpCandidate::SetUndefined){
        v.typeAndMetadata=(v.typeAndMetadata&~15u)|6;return true;
    }
    const unsigned components=op==MaterialNumericOpCandidate::SetVectorArray?count:operation;
    std::array<uint32_t,4>values{};for(unsigned i=0;i<components;++i)values[i]=arguments[i];
    bool equal=oldType==2;
    for(unsigned i=0;i<4;++i)equal=equal&&materialNumericOrderedEqualCandidate(v.vector[i],values[i]);
    // Native no-op checks all four values, not the stored component count.
    if(equal)return true;
    v.vector=values;v.integer=materialNumericTruncateCandidate(values[0]);
    v.typeAndMetadata=(v.typeAndMetadata&~0x7fu)|2|(components<<4);return true;
}

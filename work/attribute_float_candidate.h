#pragma once
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
#include <emmintrin.h>
inline bool attributeFloatTypeCandidate(unsigned flags) {
    const auto t=flags&31;return t==2 || t==3 || t==4 || t==7;
}
template<class Attribute,class Adapter>
inline float attributeFloatCandidate(Attribute* attribute,Adapter& a) {
    a.initializeFloatDefault();
    if((a.attributeFlags(attribute)&31)==3)return a.directFloat(attribute);
    if(!attributeFloatTypeCandidate(a.attributeFlags(attribute)))return a.floatDefault();
    const auto type=a.attributeFlags(attribute)&31;
    if(type==2 || type==7) {
        auto value=_mm_cvtepi32_ps(_mm_set_epi32(0,0,0,a.signedValue(attribute)));
        if(type==7) {
            const std::uint32_t bits=0x38d1b717;float scale;std::memcpy(&scale,&bits,4);
            value=_mm_mul_ss(value,_mm_set_ss(scale));
        }
        a.setSharedFloat(_mm_cvtss_f32(value));
    } else if(type==4)a.setSharedFloat(a.booleanByte(attribute)?1.0f:0.0f);
    else if(type==3)a.setSharedFloat(a.copyFloatX87(attribute));
    return a.sharedFloat();
}


#pragma once
#include <array>
#include "material_cosine_double_candidate.h"
#include "material_sine_double_candidate.h"
// Checked numerical prerequisite only. Live input bindings/Init are separate.
inline double materialTextureScrollMultiplyCandidate(double a,double b){
    uint64_t left,right;std::memcpy(&left,&a,8);std::memcpy(&right,&b,8);
    // The observed MULSD destination wins when both operands are NaNs.
    // Preserve operand selection rather than letting register allocation swap
    // them. This does not cover floating exception flags or alternate modes.
    if((left&0x7fffffffffffffffull)>0x7ff0000000000000ull){left|=0x8000000000000ull;std::memcpy(&a,&left,8);return a;}
    if((right&0x7fffffffffffffffull)>0x7ff0000000000000ull){right|=0x8000000000000ull;std::memcpy(&b,&right,8);return b;}
    volatile double result=a*b;return result;
}
inline float materialTextureScrollWrapCandidate(float x){
    const auto trunc=[](float v){return _mm_cvtsi32_ss(_mm_setzero_ps(),_mm_cvttss_si32(_mm_set_ss(v)));};
    __m128 value=_mm_set_ss(x);
    if(x<0.0f)value=_mm_add_ss(value,_mm_sub_ss(_mm_set_ss(1.0f),trunc(x)));
    return _mm_cvtss_f32(_mm_sub_ss(value,trunc(_mm_cvtss_f32(value))));
}
// time, rate, angle in degrees, matrix scale. A raw cached GetFloat read
// quiets signaling NaNs before its f32 store in the observed x87 path.
inline std::array<uint32_t,17> materialTextureScrollValueCandidate(const std::array<uint32_t,5>&q){
    std::array<float,4> v{};for(size_t i=0;i<4;++i){auto b=q[i];if(i&&(b&0x7fffffffu)>0x7f800000u)b|=0x400000u;std::memcpy(&v[i],&b,4);}
    volatile double angle=double(v[2])*0x1.1df46a2529d39p-6;
    volatile double x=materialCosineDoubleCandidate(angle);x=materialTextureScrollMultiplyCandidate(x,double(v[0]));x=materialTextureScrollMultiplyCandidate(x,double(v[1]));
    volatile float roundedX=float(x);
    volatile double y=materialSineDoubleCandidate(angle);y=materialTextureScrollMultiplyCandidate(y,double(v[0]));y=materialTextureScrollMultiplyCandidate(y,double(v[1]));
    volatile float roundedY=float(y);
    const float a=materialTextureScrollWrapCandidate(roundedX),b=materialTextureScrollWrapCandidate(roundedY);
    std::array<uint32_t,17>out{};out[0]=(q[4]&15u)==7?9u:3u;
    if(out[0]==9){out[1]=q[3];if((out[1]&0x7fffffffu)>0x7f800000u)out[1]|=0x400000u;out[6]=out[1];out[11]=out[16]=0x3f800000u;std::memcpy(&out[4],&a,4);std::memcpy(&out[8],&b,4);}
    else{std::memcpy(&out[1],&a,4);std::memcpy(&out[2],&b,4);}
    return out;
}

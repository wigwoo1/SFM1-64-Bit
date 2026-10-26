#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
#include "material_trig_fallback_candidate.h"
using MaterialMatrixMathCandidate=std::array<uint32_t,16>;
inline float materialMatrixFloatCandidate(uint32_t b){float v=0;std::memcpy(&v,&b,4);return v;}
inline uint32_t materialMatrixBitsCandidate(float v){uint32_t b=0;std::memcpy(&b,&v,4);return b;}
inline uint32_t materialMatrixBinaryCandidate(uint32_t a,uint32_t b,bool multiply){
    auto nan=[](uint32_t x){return (x&0x7fffffffu)>0x7f800000u;};
    // Preserve SSE destination/source NaN selection independently of compiler
    // register allocation. Floating exception flags are a separate contract.
    if(nan(a))return a|0x400000u;if(nan(b))return b|0x400000u;
    float x=materialMatrixFloatCandidate(a),y=materialMatrixFloatCandidate(b),z=0;
    _mm_store_ss(&z,multiply?_mm_mul_ss(_mm_set_ss(x),_mm_set_ss(y)):_mm_add_ss(_mm_set_ss(x),_mm_set_ss(y)));
    return materialMatrixBitsCandidate(z);
}
inline void materialMatrixMultiplyCandidate(const MaterialMatrixMathCandidate&a,const MaterialMatrixMathCandidate&b,MaterialMatrixMathCandidate&out){
    MaterialMatrixMathCandidate z{};
    for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col){
        const unsigned index=row*4+col;
        const uint32_t first=materialMatrixBinaryCandidate(a[4*row],b[col],true),second=materialMatrixBinaryCandidate(a[4*row+1],b[4+col],true);
        uint32_t v=(0xf7e0u&(1u<<index))?materialMatrixBinaryCandidate(second,first,false):materialMatrixBinaryCandidate(first,second,false);
        v=materialMatrixBinaryCandidate(v,materialMatrixBinaryCandidate(b[8+col],a[4*row+2],true),false);
        const auto last=(col==3&&row)?materialMatrixBinaryCandidate(a[4*row+3],b[12+col],true):materialMatrixBinaryCandidate(b[12+col],a[4*row+3],true);
        z[index]=materialMatrixBinaryCandidate(v,last,false);
    }
    out=z;
}
inline MaterialMatrixMathCandidate materialMatrixTranslateCandidate(uint32_t x,uint32_t y,uint32_t z){
    MaterialMatrixMathCandidate out{};out[0]=out[5]=out[10]=out[15]=0x3f800000;out[3]=x;out[7]=y;out[11]=z;return out;
}
inline MaterialMatrixMathCandidate materialMatrixScaleMathCandidate(uint32_t x,uint32_t y,uint32_t z){
    MaterialMatrixMathCandidate out{};out[0]=x;out[5]=y;out[10]=z;out[15]=0x3f800000;return out;
}
// Isolated numerical contract; full parser/factory integration remains gated.
inline MaterialMatrixMathCandidate materialMatrixRotateCandidate(uint32_t degrees,bool sse2=false){
    const float radians=static_cast<float>(double(materialMatrixFloatCandidate(degrees))*0.017453292519943295);
    const float s=materialTrigCandidate(radians,false,sse2),c=materialTrigCandidate(radians,true,sse2);
    MaterialMatrixMathCandidate out{};out[0]=out[5]=materialMatrixBitsCandidate(c);out[4]=materialMatrixBitsCandidate(s);out[1]=out[4]^0x80000000u;out[10]=out[15]=0x3f800000;return out;
}
inline MaterialMatrixMathCandidate materialMatrixTransformCandidate(const std::array<uint32_t,7>&p,bool sse2=false){
    auto out=materialMatrixTranslateCandidate(p[0]^0x80000000u,p[1]^0x80000000u,0);
    auto stage=materialMatrixScaleMathCandidate(p[2],p[3],0x3f800000);materialMatrixMultiplyCandidate(stage,out,out);
    stage=materialMatrixRotateCandidate(p[4],sse2);materialMatrixMultiplyCandidate(stage,out,out);
    float cx=materialMatrixFloatCandidate(p[0]),cy=materialMatrixFloatCandidate(p[1]);float tx=materialMatrixFloatCandidate(p[5]),ty=materialMatrixFloatCandidate(p[6]);
    float x=0,y=0;_mm_store_ss(&x,_mm_add_ss(_mm_set_ss(tx),_mm_set_ss(cx)));_mm_store_ss(&y,_mm_add_ss(_mm_set_ss(ty),_mm_set_ss(cy)));
    stage=materialMatrixTranslateCandidate(materialMatrixBitsCandidate(x),materialMatrixBitsCandidate(y),0);materialMatrixMultiplyCandidate(stage,out,out);return out;
}

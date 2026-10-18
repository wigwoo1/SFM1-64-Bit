#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
// Owned float3x4 output. Input must be a copied snapshot, with no output aliasing.
// Instruction-derived ordering; full original execution parity remains unverified.
inline std::array<uint32_t,12> quaternionPositionMatrixCandidate(const std::array<uint32_t,4>& quaternion,const std::array<uint32_t,3>& position){
    float q[4];std::memcpy(q,quaternion.data(),16);
    const auto product=[&](unsigned a,unsigned b){return _mm_mul_ss(_mm_mul_ss(_mm_set_ss(q[a]),_mm_set_ss(2.0f)),_mm_set_ss(q[b]));};
    std::array<uint32_t,12> result{};
    const auto store=[&](unsigned index,__m128 value){float f;_mm_store_ss(&f,value);std::memcpy(&result[index],&f,4);};
    store(0,_mm_sub_ss(_mm_sub_ss(_mm_set_ss(1.0f),product(1,1)),product(2,2)));
    store(4,_mm_add_ss(product(0,1),product(3,2)));
    store(8,_mm_sub_ss(product(0,2),product(3,1)));
    store(1,_mm_sub_ss(product(0,1),product(3,2)));
    store(5,_mm_sub_ss(_mm_sub_ss(_mm_set_ss(1.0f),product(0,0)),product(2,2)));
    store(9,_mm_add_ss(product(1,2),product(3,0)));
    store(2,_mm_add_ss(product(0,2),product(3,1)));
    store(6,_mm_sub_ss(product(1,2),product(3,0)));
    store(10,_mm_sub_ss(_mm_sub_ss(_mm_set_ss(1.0f),product(0,0)),product(1,1)));
    for(unsigned i=0;i<3;++i){auto word=position[i];if((word&0x7f800000u)==0x7f800000u&&(word&0x007fffffu))word|=0x00400000u;result[i*4+3]=word;}
    return result;
}

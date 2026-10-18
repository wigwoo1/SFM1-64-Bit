#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
inline std::array<uint32_t,3> vectorTransformCandidate(const std::array<uint32_t,3>& position,const std::array<uint32_t,12>& matrix){
    float p[3],m[12];std::memcpy(p,position.data(),12);std::memcpy(m,matrix.data(),48);
    std::array<uint32_t,3> result{};
    for(unsigned row=0;row<3;++row){
        const auto i=row*4;
        auto value=_mm_mul_ss(_mm_set_ss(m[i+1]),_mm_set_ss(p[1]));
        value=_mm_add_ss(value,_mm_mul_ss(_mm_set_ss(m[i]),_mm_set_ss(p[0])));
        value=_mm_add_ss(value,_mm_mul_ss(_mm_set_ss(m[i+2]),_mm_set_ss(p[2])));
        value=_mm_add_ss(value,_mm_set_ss(m[i+3]));
        float f;_mm_store_ss(&f,value);std::memcpy(&result[row],&f,4);
    }
    return result;
}

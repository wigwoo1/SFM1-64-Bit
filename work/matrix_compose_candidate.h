#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <emmintrin.h>
inline std::array<uint32_t,12> matrixComposeCandidate(const std::array<uint32_t,12>& left,const std::array<uint32_t,12>& right){
    float a[12],b[12],out[12];std::memcpy(a,left.data(),48);std::memcpy(b,right.data(),48);
    const __m128 b0=_mm_loadu_ps(b),b1=_mm_loadu_ps(b+4),b2=_mm_loadu_ps(b+8);
    const __m128 mask=_mm_castsi128_ps(_mm_set_epi32(-1,0,0,0));
    for(unsigned row=0;row<3;++row){
        const auto ar=_mm_loadu_ps(a+row*4);
        auto value=_mm_mul_ps(_mm_shuffle_ps(ar,ar,0xaa),b2);
        value=_mm_add_ps(value,_mm_mul_ps(_mm_shuffle_ps(ar,ar,0x55),b1));
        value=_mm_add_ps(value,_mm_mul_ps(_mm_shuffle_ps(ar,ar,0),b0));
        value=_mm_add_ps(value,_mm_and_ps(ar,mask));
        _mm_storeu_ps(out+row*4,value);
    }
    std::array<uint32_t,12> result{};std::memcpy(result.data(),out,48);return result;
}

#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
// Caller supplies recovered right/up basis. Quaternion->angles->trig is separate.
inline std::array<uint32_t,3> projectedLightJitterDisplacementCandidate(const std::array<uint32_t,3>& origin,const std::array<uint32_t,6>& rightUp,const std::array<uint32_t,2>& jitter,uint32_t radiusWord){
    float j[2],radius,basis[6],position[3];std::memcpy(j,jitter.data(),8);std::memcpy(&radius,&radiusWord,4);std::memcpy(basis,rightUp.data(),24);std::memcpy(position,origin.data(),12);
    if(j[0]==0.0f&&j[1]==0.0f)return origin;
    for(unsigned stage=0;stage<2;++stage)for(unsigned component=0;component<3;++component){
        auto offset=_mm_mul_ss(_mm_set_ss(basis[stage*3+component]),_mm_set_ss(j[stage]));
        offset=_mm_mul_ss(offset,_mm_set_ss(radius));
        position[component]=_mm_cvtss_f32(_mm_add_ss(_mm_set_ss(position[component]),offset));
    }
    std::array<uint32_t,3> result{};std::memcpy(result.data(),position,12);return result;
}

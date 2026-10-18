#pragma once
#include "angle_basis_products_candidate.h"
#include <stdexcept>
extern "C" void sfm64OriginalStyleSinCos(const float* radians,float* sine,float* cosine);
inline std::array<uint32_t,9> angleBasisCandidate(const std::array<uint32_t,3>& angleWords){
    float angles[3];std::memcpy(angles,angleWords.data(),12);
    for(auto angle:angles)if(!(angle>=-36000.0f&&angle<=36000.0f))throw std::invalid_argument("angle basis verified domain");
    uint32_t scaleWord=0x3c8efa35u;float scale;std::memcpy(&scale,&scaleWord,4);
    std::array<uint32_t,6> trig{};
    for(unsigned pair=0;pair<3;++pair){const unsigned component=pair==0?1:pair==1?0:2;
        float radians=_mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(angles[component]),_mm_set_ss(scale)));float sine,cosine;
        sfm64OriginalStyleSinCos(&radians,&sine,&cosine);
        std::memcpy(&trig[pair*2],&sine,4);std::memcpy(&trig[pair*2+1],&cosine,4);
    }
    return angleBasisProductsCandidate(trig);
}

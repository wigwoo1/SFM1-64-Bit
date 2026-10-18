#pragma once
#include "quaternion_position_matrix_candidate.h"
#include <cmath>
#include <stdexcept>
inline std::array<uint32_t,3> quaternionAnglesCandidate(const std::array<uint32_t,4>& quaternion){
    float q[4];std::memcpy(q,quaternion.data(),16);
    for(auto value:q)if(!(value>=-1.0f&&value<=1.0f))throw std::invalid_argument("quaternion angle candidate domain");
    const auto words=quaternionPositionMatrixCandidate(quaternion,{{0,0,0}});float m[12];std::memcpy(m,words.data(),48);
    auto squared=_mm_add_ss(_mm_mul_ss(_mm_set_ss(m[4]),_mm_set_ss(m[4])),_mm_mul_ss(_mm_set_ss(m[0]),_mm_set_ss(m[0])));
    const float horizontal=_mm_cvtss_f32(_mm_sqrt_ss(squared));
    uint32_t thresholdWord=0x3a83126f,scaleWord=0x42652ee1;float threshold,scale;std::memcpy(&threshold,&thresholdWord,4);std::memcpy(&scale,&scaleWord,4);
    const auto angle=[&](float y,float x){const float radians=static_cast<float>(std::atan2(static_cast<double>(y),static_cast<double>(x)));return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(radians),_mm_set_ss(scale)));};
    float angles[3];angles[0]=angle(-m[8],horizontal);
    if(horizontal>threshold){angles[1]=angle(m[4],m[0]);angles[2]=angle(m[9],m[10]);}
    else{angles[1]=angle(-m[1],m[5]);angles[2]=0.0f;}
    std::array<uint32_t,3> result{};std::memcpy(result.data(),angles,12);return result;
}

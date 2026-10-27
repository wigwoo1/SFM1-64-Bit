#pragma once
#include <array>
#include "material_sine_double_candidate.h"
// time, period, minimum, maximum, offset: exact f32 cached input values.
inline uint32_t materialSineValueCandidate(const std::array<uint32_t,5>&bits){
    std::array<float,5>v{};std::memcpy(v.data(),bits.data(),20);if(v[1]==0)v[1]=1;
    volatile float delta=v[0]-v[4];volatile double angle=double(delta)*0x1.921fb54442d18p2;
    angle=angle/double(v[1]);volatile double wave=materialSineDoubleCandidate(angle);
    volatile float range=v[3]-v[2];volatile double normalized=wave*.5;normalized=normalized+.5;
    volatile float weight=static_cast<float>(normalized);volatile float product=weight*range;float result=product+v[2];uint32_t resultBits;std::memcpy(&resultBits,&result,4);return resultBits;
}

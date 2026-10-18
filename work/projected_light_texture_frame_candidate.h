#pragma once
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <emmintrin.h>
inline int32_t projectedLightTextureFrameCandidate(uint32_t timeWord,uint32_t frameRateWord,bool hasTexture,int32_t frameCount){
    if(!hasTexture)return 0;
    if(frameCount<=0)throw std::invalid_argument("projected texture frame count");
    int32_t time;float rate,scale;uint32_t scaleWord=0x38d1b717;
    std::memcpy(&time,&timeWord,4);std::memcpy(&rate,&frameRateWord,4);std::memcpy(&scale,&scaleWord,4);
    auto value=_mm_cvtepi32_ps(_mm_cvtsi32_si128(time));
    value=_mm_mul_ss(value,_mm_set_ss(scale));value=_mm_mul_ss(value,_mm_set_ss(rate));
    const int32_t frame=_mm_cvttss_si32(value);
    return frame%frameCount; // Native signed remainder, including negative time.
}

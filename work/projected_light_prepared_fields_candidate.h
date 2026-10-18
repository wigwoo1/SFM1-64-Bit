#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
#include "projected_light_texture_frame_candidate.h"
// Named owned data, not the original x86 output structure or GPU constants.
struct ProjectedLightPreparedFieldsCandidate {
    std::array<uint32_t,7> quaternionPositionWords{};
    std::array<uint32_t,31> controlWords{}; // Same named order as saved float fields.
    std::array<uint32_t,3> rgbWords{};
    std::array<uint8_t,4> booleanBytes{};
    std::string textureName;
    uint32_t animationTimeWord{},numPlanesWord{};
};
inline ProjectedLightPreparedFieldsCandidate projectedLightPreparedFieldsCandidate(const ProjectedLightSavedFieldsCandidate& saved,const std::array<uint32_t,7>& jitteredWorldPose){
    ProjectedLightPreparedFieldsCandidate result;
    result.quaternionPositionWords=jitteredWorldPose;result.controlWords=saved.floatWords;
    // Recovered x87 copies quiet signaling NaNs; retain sign/payload.
    for(unsigned index:{0u,3u,5u,6u,7u,8u,9u,11u,12u,13u,14u,15u,16u,17u,18u,19u,20u,21u,22u,23u,24u,25u,27u,28u,29u,30u}){
        auto& word=result.controlWords[index];if((word&0x7f800000u)==0x7f800000u&&(word&0x007fffffu))word|=0x00400000u;
    }
    float horizontal,vertical;std::memcpy(&horizontal,&saved.floatWords[1],4);std::memcpy(&vertical,&saved.floatWords[2],4);
    const auto selected=horizontal>vertical?saved.floatWords[1]:saved.floatWords[2];
    result.controlWords[1]=selected;result.controlWords[2]=selected;
    float intensity,scale;const uint32_t scaleWord=0x3b808081;
    std::memcpy(&intensity,&saved.floatWords[26],4);std::memcpy(&scale,&scaleWord,4);
    for(unsigned channel=0;channel<3;++channel){
        const float color=float((saved.colorWord>>(channel*8))&255u);
        auto value=_mm_mul_ss(_mm_set_ss(color),_mm_set_ss(intensity));
        value=_mm_mul_ss(value,_mm_set_ss(scale));float output;_mm_store_ss(&output,value);std::memcpy(&result.rgbWords[channel],&output,4);
    }
    result.booleanBytes=saved.booleanBytes;result.textureName=saved.textureName;
    result.animationTimeWord=saved.animationTimeWord;result.numPlanesWord=saved.numPlanesWord;
    return result;
}
// Actual loaded texture presence/frame count must come from renderer ownership.
inline int32_t projectedLightPreparedTextureFrameCandidate(const ProjectedLightPreparedFieldsCandidate& prepared,bool hasTexture,int32_t frameCount){
    return projectedLightTextureFrameCandidate(prepared.animationTimeWord,prepared.controlWords[10],hasTexture,frameCount);
}

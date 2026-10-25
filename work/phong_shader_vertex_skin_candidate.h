#pragma once
#include "generic_shader_skin_blend_candidate.h"
#include "generic_compressed_weight_input_candidate.h"
#include "generic_packed_normal_candidate.h"
#include <cstring>

struct PhongShaderVertexFrameCandidate {
    std::array<float,3> position{},normal{},tangent{},binormal{};
};
struct PhongPackedFrameCandidate {
    std::array<float,3> normal{},tangent{};
    float handedness=1;
};
// Installed phong_vs20/vs30 compressed stream-input programs. This decodes
// supplied shader bytes; it does NOT implement the native tangent packer.
inline PhongPackedFrameCandidate phongDecodePackedFrameCandidate(const std::array<uint8_t,4>&bytes){
    return {genericDecodeNormalCandidate({bytes[0],bytes[1]}),
            genericDecodeNormalCandidate({bytes[2],bytes[3]}),bytes[3]<128?-1.0f:1.0f};
}
// CPU reference for static0/dynamic2,3 of the installed phong_vs20/vs30.
// Three matrices are already selected WORLD-space skin matrices, not local
// transforms. Input conversions/strip selection happen before this function.
// The same direction delta modifies N and T. Cross products use RAW world N/T,
// then handedness and normalization; normalizing before crossing changes bits.
// Separate binary32 MAD/DP3/DP4 + float sqrt/reciprocal is an explicit CPU
// convention, not native GPU precision parity. No texture-morph path/shading.
inline PhongShaderVertexFrameCandidate phongShaderSkinVertexCandidate(
    const std::array<GenericShaderSkinMatrixCandidate,3>&matrices,bool compressed,
    const std::array<uint8_t,8>&weightData,const std::array<uint8_t,12>&normalData,
    const std::array<uint8_t,16>&tangentData,const std::array<float,3>&position,
    const std::array<float,3>&positionDelta,const std::array<float,3>&directionDelta,float flexScale){
    if(!std::isfinite(flexScale))throw std::invalid_argument("nonfinite Phong flex scale");
    std::array<float,2>weights{};std::array<float,3>normal{},tangent{};float handedness=0;
    if(compressed){std::array<uint8_t,4>w{};std::memcpy(w.data(),weightData.data(),4);
        weights=genericCompressedWeightInputCandidate(w);
        const auto frame=phongDecodePackedFrameCandidate({normalData[0],normalData[1],normalData[2],normalData[3]});
        normal=frame.normal;tangent=frame.tangent;handedness=frame.handedness;
    }else{std::memcpy(weights.data(),weightData.data(),8);std::memcpy(normal.data(),normalData.data(),12);
        std::memcpy(tangent.data(),tangentData.data(),12);std::memcpy(&handedness,tangentData.data()+12,4);}
    // Restrict the owned API to the meaningful tangent-frame sign domain.
    if(handedness!=1.0f&&handedness!=-1.0f)throw std::invalid_argument("Phong tangent handedness must be +1 or -1");
    std::array<float,4>p{};std::array<float,3>n{},t{};
    for(unsigned i=0;i<3;++i){
        if(!std::isfinite(position[i])||!std::isfinite(positionDelta[i])||!std::isfinite(normal[i])||!std::isfinite(tangent[i])||!std::isfinite(directionDelta[i]))throw std::invalid_argument("nonfinite Phong vertex input");
        const float dp=positionDelta[i]*flexScale,dd=directionDelta[i]*flexScale;
        p[i]=dp+position[i];n[i]=dd+normal[i];t[i]=dd+tangent[i];
    }
    p[3]=1.0f;const auto blended=genericShaderSkinBlendCandidate(matrices,weights);
    PhongShaderVertexFrameCandidate out;std::array<float,3>rawNormal{},rawTangent{},cross{};
    for(unsigned row=0;row<3;++row){
        float pv=p[0]*blended[row][0],nv=n[0]*blended[row][0],tv=t[0]*blended[row][0];
        for(unsigned i=1;i<3;++i){const float np=n[i]*blended[row][i],tp=t[i]*blended[row][i],pp=p[i]*blended[row][i];nv=nv+np;tv=tv+tp;pv=pv+pp;}
        const float translation=p[3]*blended[row][3];out.position[row]=pv+translation;
        if(!std::isfinite(out.position[row]))throw std::overflow_error("Phong world position overflow");
        rawNormal[row]=nv;rawTangent[row]=tv;
    }
    for(unsigned i=0;i<3;++i){const unsigned y=(i+1)%3,z=(i+2)%3;
        const float negative=rawNormal[z]*rawTangent[y],positive=rawNormal[y]*rawTangent[z];
        const float difference=positive+(-negative);cross[i]=difference*handedness;}
    out.normal=genericNormalizeReferenceCandidate(rawNormal);
    out.tangent=genericNormalizeReferenceCandidate(rawTangent);
    out.binormal=genericNormalizeReferenceCandidate(cross);return out;
}

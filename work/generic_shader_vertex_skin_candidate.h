#pragma once
#include "generic_packed_normal_candidate.h"
#include "generic_compressed_weight_input_candidate.h"
#include "generic_shader_skin_blend_candidate.h"
#include <cstring>
struct GenericShaderSkinnedVertexCandidate {std::array<float,3>position{},normal{};};
// Assembled static0/dynamic4..7 world-position/world-normal CPU reference.
// Matrices are already selected for the three hardware slots and WORLD-space.
// Tangents, material shading, GPU rounding and live variant choice are outside
// this API; it does not load sessions or apply a second model placement.
inline GenericShaderSkinnedVertexCandidate genericShaderSkinVertexCandidate(
    const std::array<GenericShaderSkinMatrixCandidate,3>&matrices,bool compressed,
    const std::array<uint8_t,8>&weightData,const std::array<uint8_t,12>&normalData,
    const std::array<float,3>&position,const std::array<float,3>&positionDelta,
    const std::array<float,3>&normalDelta,float flexScale,bool dynamicLighting,float staticLightEnabled){
    if(!std::isfinite(flexScale)||!std::isfinite(staticLightEnabled))throw std::invalid_argument("nonfinite skin constant");
    const float staticSquare=staticLightEnabled*staticLightEnabled;if(!std::isfinite(staticSquare))throw std::overflow_error("static light predicate overflow");
    const float baseGate=(-staticSquare<staticSquare)?1.0f:0.0f;
    std::array<float,2>weights{};std::array<float,3>normal{};
    if(compressed){std::array<uint8_t,4>packed{};std::memcpy(packed.data(),weightData.data(),4);weights=genericCompressedWeightInputCandidate(packed);normal=genericDecodeNormalCandidate({normalData[0],normalData[1]});}
    else{std::memcpy(weights.data(),weightData.data(),8);std::memcpy(normal.data(),normalData.data(),12);}
    std::array<float,4>p{};std::array<float,3>n{};
    for(unsigned i=0;i<3;++i){if(!std::isfinite(position[i])||!std::isfinite(positionDelta[i])||!std::isfinite(normal[i])||!std::isfinite(normalDelta[i]))throw std::invalid_argument("nonfinite shader vertex");
        const float dp=positionDelta[i]*flexScale,dn=normalDelta[i]*flexScale;p[i]=dp+position[i];
        if(dynamicLighting)n[i]=dn+normal[i];else{const float base=baseGate*normal[i];n[i]=base+dn;}}
    p[3]=1;const auto blended=genericShaderSkinBlendCandidate(matrices,weights);GenericShaderSkinnedVertexCandidate out;
    for(unsigned row=0;row<3;++row){float v=p[0]*blended[row][0];for(unsigned i=1;i<4;++i){const float product=p[i]*blended[row][i];v=v+product;}out.position[row]=v;
        float w=n[0]*blended[row][0];for(unsigned i=1;i<3;++i){const float product=n[i]*blended[row][i];w=w+product;}out.normal[row]=w;
        if(!std::isfinite(v))throw std::overflow_error("shader position overflow");}
    out.normal=genericNormalizeReferenceCandidate(out.normal);return out;
}

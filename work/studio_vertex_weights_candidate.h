#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>

struct StudioVertexWeightsCandidate {
    std::array<uint8_t,8> weights{};
    std::array<uint8_t,4> indices{};
    bool valid=true;
};
// Installed studiorender 10055AA0/100560C0: bounded bone-bearing VVD3/VTX9
// branch only. Does not select vertex declarations, map strip hardware slots
// to MDL bones, unpack shader inputs, or reproduce normal/tangent packing.
inline StudioVertexWeightsCandidate studioVertexWeightsCandidate(
    const std::array<uint8_t,48>&vertex,const std::array<uint8_t,9>&optimized,
    bool compressed,unsigned storedWeights,std::array<uint8_t,8> initial){
    if(!vertex[15]||vertex[15]>3||optimized[3]>3||storedWeights<1||storedWeights>2)
        throw std::invalid_argument("unsupported studio vertex weight count");
    std::array<float,3> raw{},selected{};std::memcpy(raw.data(),vertex.data(),12);
    for(float w:raw)if(!std::isfinite(w)||w<0||w>1)throw std::invalid_argument("studio vertex weight domain");
    StudioVertexWeightsCandidate out;out.weights=initial;
    float sum=0;
    for(unsigned i=0;i<optimized[3];++i){
        if(optimized[i]>=3||(optimized[6+i]>=53&&optimized[6+i]!=255))throw std::invalid_argument("studio vertex weight/index span");
        sum=raw[optimized[i]]+sum;
    }
    const float delta=sum-1.0f;
    if(optimized[3]&&double(std::abs(delta))>0.001){out.valid=false;sum=1.0f;}
    else if(sum==0)sum=1.0f;
    const float reciprocal=1.0f/sum;
    for(unsigned i=0;i<optimized[3];++i){
        selected[i]=raw[optimized[i]]*reciprocal;
        // Native movzx makes an earlier compare against -1 unreachable.
        // 255 is remapped to zero without discarding its selected weight.
        out.indices[i]=optimized[6+i]==255?0:optimized[6+i];
    }
    if(optimized[3]){
        if(compressed){
            const int a=int(selected[0]*32768.0f),b=int(selected[1]*32768.0f),c=int(selected[2]*32768.0f);
            uint32_t packed=uint32_t(a-1)&0xffffu;
            if(storedWeights>1)packed|=uint32_t(32767-c-a)<<16;
            (void)b; // Original truncates this lane but residual uses a and c.
            std::memcpy(out.weights.data(),&packed,4);
        }else std::memcpy(out.weights.data(),selected.data(),8);
    }
    return out;
}

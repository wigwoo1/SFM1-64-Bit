#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
// Defaults from complete installed constructors. Fields 08/0C intentionally
// keep offset names: no unsupported semantic name is inferred for them.
inline std::array<uint32_t,4>phongVertexStaticDefaultsCandidate(unsigned version,int32_t firstHardwareLevel,int32_t secondHardwareLevel,uint32_t materialFlags){
    if(version!=20&&version!=30)throw std::invalid_argument("Phong vertex version");
    return {0,0,(firstHardwareLevel>=92&&!(materialFlags&0x80000000u))?1u:0u,version==20&&secondHardwareLevel<92?1u:0u};
}
// Original 10054370 queries the API's raw low byte, then the fog mode. This
// captures defaults only; draw-time setters can subsequently replace fields.
inline std::array<uint32_t,5>phongVs30DynamicDefaultsCandidate(uint32_t rawMorphResult,int32_t fogMode){
    return {0,0,rawMorphResult&255u,0,fogMode==2?1u:0u};
}
inline void phongVertexStaticFieldsCheckCandidate(const std::array<uint32_t,4>&fields){
    if(fields[2]>1||fields[3]>1)throw std::invalid_argument("Phong static field domain");
}
// Arguments passed to the shader-cache API are staticIndex*dynamicCount.
// These formulas are byte-checked at their installed inline call sites.
inline uint32_t phongVs20StaticArgumentCandidate(const std::array<uint32_t,4>&fields,bool useStaticControlFlow){
    phongVertexStaticFieldsCheckCandidate(fields);return (uint32_t(useStaticControlFlow)+2*fields[2]+4*fields[3])*24;
}
inline uint32_t phongVs30StaticArgumentCandidate(const std::array<uint32_t,4>&fields,bool decal,bool worldNormal){
    phongVertexStaticFieldsCheckCandidate(fields);if(fields[3])throw std::invalid_argument("VS30 native field0C must be zero");
    return (uint32_t(decal)+2*uint32_t(worldNormal)+4*fields[2])*48;
}
inline uint32_t phongVs20DynamicIndexCandidate(unsigned compression,int32_t numBones,uint32_t lightCount,bool useStaticControlFlow,int32_t fogMode){
    if(compression>1)throw std::invalid_argument("Phong compression enum");const uint32_t lights=useStaticControlFlow?0:lightCount;
    if(lights>2)throw std::invalid_argument("VS20 compiled light-count domain");
    return compression+2*uint32_t(numBones>0)+4*lights+12*uint32_t(fogMode==2);
}
inline uint32_t phongVs30DynamicIndexCandidate(unsigned compression,int32_t numBones,bool morphing,unsigned tessellationMode,int32_t fogMode){
    if(compression>1||tessellationMode>2)throw std::invalid_argument("VS30 compiled enum domain");
    return uint32_t(compression&&tessellationMode==0)+2*uint32_t(numBones>0&&tessellationMode==0)+4*uint32_t(morphing)+8*tessellationMode+24*uint32_t(fogMode==2);
}
// A valid index does not prove a compiled record exists. The caller must look
// up that exact cache slot and separately admit its input/shader contract.
// No fallback to static0, no runtime/live hardware/material state inference.

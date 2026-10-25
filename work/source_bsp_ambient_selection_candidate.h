#pragma once
#include "source_bsp_leaf_ambient_candidate.h"
#include <utility>

enum class SourceBspAmbientStorageCandidate : uint32_t {LegacyLeafV0=0,SplitLdr=1,SplitHdr=2};
// Shaderapidx9 hardware-config013 slot34. Forced GL emulation enables the
// GLOBAL hardware object, which must not silently be assumed to alias this
// query's object. Capture the local enabled byte BEFORE querying DX level.
// Services expose stable explicit configuration; actual device setup is separate.
template<class Hardware>
int32_t sourceHardwareHdrTypeCandidate(int32_t emulateGl,Hardware&hardware){
    if(emulateGl!=0)hardware.enableGlobalHdr();
    const auto enabled=hardware.hdrEnabled();
    const auto dxLevel=hardware.dxSupportLevel();
    return enabled!=0&&dxLevel>=90?hardware.hardwareHdrType():0;
}
// Original BSP lump helper: an active override wins, including empty or
// negative lengths. Validation/decompression are separate loader contracts.
inline int32_t sourceEffectiveBspLumpLengthCandidate(bool overrideActive,int32_t headerLength,int32_t overrideLength){
    return overrideActive?overrideLength:headerLength;
}
// Engine100E49D0..100E4A1C chooses the format/ambient tables. The callbacks are
// explicit current hardware-query/effective lump55 length inputs, NOT assumed
// console defaults. Hardware config013 slot34 is a current HDR-type query;
// it is not just the object's enabled byte and can have side effects.
// Legacy V0 is a separate storage layout, not a fallback to split LDR.
template<class HdrTypeQuery,class HdrSampleLengthQuery>
SourceBspAmbientStorageCandidate sourceBspAmbientSelectionCandidate(
    uint32_t leafLumpVersion,HdrTypeQuery&&hdrType,HdrSampleLengthQuery&&hdrSampleLength){
    if(leafLumpVersion==0)return SourceBspAmbientStorageCandidate::LegacyLeafV0;
    if(leafLumpVersion!=1)throw std::invalid_argument("unsupported BSP leaf version for ambient selection");
    if(hdrType()!=0&&hdrSampleLength()>0)return SourceBspAmbientStorageCandidate::SplitHdr;
    return SourceBspAmbientStorageCandidate::SplitLdr;
}
inline SourceBspAmbientEncodingCandidate sourceBspSplitAmbientEncodingCandidate(SourceBspAmbientStorageCandidate storage){
    if(storage==SourceBspAmbientStorageCandidate::SplitHdr)return SourceBspAmbientEncodingCandidate::Hdr;
    if(storage==SourceBspAmbientStorageCandidate::SplitLdr)return SourceBspAmbientEncodingCandidate::Ldr;
    throw std::invalid_argument("legacy/unknown ambient storage requires a separate reader");
}

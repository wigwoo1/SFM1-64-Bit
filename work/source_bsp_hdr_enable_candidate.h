#pragma once
#include <cstdint>

struct SourceBspHdrEnableRequestCandidate {bool mapHasHdr=false,enableRequested=false;};
// Engine100E34A0 decision ONLY, after map-header initialization. Length callbacks
// supply effective (external override or header) lengths. Signed tests differ:
// lumps53/54 must be positive; lump55 for BSP>=20 must merely be nonzero.
// This does not initialize hardware, commit enabled state, notify materials,
// validate/decompress lumps, or discover the user's current console settings.
template<class EffectiveLength,class HdrLevel,class HardwareHdrType>
SourceBspHdrEnableRequestCandidate sourceBspHdrEnableRequestCandidate(
    int32_t bspVersion,EffectiveLength&&length,HdrLevel&&level,HardwareHdrType&&type){
    bool hasHdr=length(53)>0&&length(54)>0;
    if(bspVersion>=20&&length(55)==0)hasHdr=false;
    const bool request=hasHdr&&level()>=2&&type()!=0;
    return {hasHdr,request};
}

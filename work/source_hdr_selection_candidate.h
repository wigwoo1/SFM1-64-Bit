#pragma once
// Independently authored installed-ABI policy, expressed as owned scene inputs.
// This is not live hardware negotiation or an implicit default rendering mode.
#include <cstdint>

struct SourceHdrHardwareStateCandidate {
    std::int32_t dxSupportLevel=0,configuredType=0;
    std::uint8_t enabled=0;
};
inline std::int32_t selectSourceHdrTypeCandidate(SourceHdrHardwareStateCandidate& queried,
    SourceHdrHardwareStateCandidate& global,std::int32_t emulateGl)noexcept{
    // The native side effect targets the global object, which need not alias
    // the queried object. Unknown configured integers are returned unchanged.
    if(emulateGl!=0)global.enabled=1;
    return queried.enabled!=0&&queried.dxSupportLevel>=90?queried.configuredType:0;
}

struct SourceLightmapCreationRequestCandidate {
    std::uint32_t imageFormat=0,extraCreationFlags=0;
};
inline bool sourceLightmapCreationRequestCandidate(SourceLightmapCreationRequestCandidate& out,
    std::int32_t selectedType)noexcept{
    SourceLightmapCreationRequestCandidate next;
    switch(selectedType){
        case 0:next.imageFormat=0;next.extraCreationFlags=0x20000;break;
        case 1:next.imageFormat=25;break;
        case 2:next.imageFormat=24;break;
        // The original dispatch falls through to format0 for unknown words.
        // This checked API refuses those unverified modes without committing.
        default:return false;
    }
    out=next;return true;
}

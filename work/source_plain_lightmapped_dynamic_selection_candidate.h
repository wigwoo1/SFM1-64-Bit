#pragma once
#include <cstdint>

// Independent desired dynamic indices from explicit measured state. This does
// not build native cached material flags, issue API queries/commands, choose a
// static program, or establish original live configuration. Plain/no-outline
// path only. Fog query results are separate observations, never one cached value.
struct SourcePlainLightmappedDynamicStateCandidate {
    int32_t dxLevel=0;
    uint32_t modeQuery=0,fogForAlpha=0,fogForVertex=0,fogForPixel=0;
    uint32_t depthAlphaReturn=0,envmapContrastBits=0;
    bool cachedVertexFast=false,cachedPixelFast=false,opaqueAlphaEligible=false;
    bool hasOutline=false;
};
struct SourcePlainLightmappedDynamicSelectionCandidate {
    uint32_t vertex=0,pixel=0;
    bool ps20b=false;
};
inline bool sourcePlainLightmappedDynamicSelectionCandidate(
        SourcePlainLightmappedDynamicSelectionCandidate&out,
        const SourcePlainLightmappedDynamicStateCandidate&s) noexcept {
    // Mode3 has additional color/tone-map state not covered by this contract;
    // outline and pre-DX9 material routes have not been reconstructed here.
    if(s.dxLevel<90||s.modeQuery>2||s.hasOutline)return false;
    const uint32_t vertexFast=s.cachedVertexFast&&s.modeQuery==0?1u:0u;
    const uint32_t pixelFast=s.cachedPixelFast&&s.modeQuery==0?1u:0u;
    // Exact +1.0 comparison for IEEE binary32 bits. No floating arithmetic or
    // NaN payload/trap emulation is needed to select this boolean branch.
    const uint32_t contrast=pixelFast&&s.envmapContrastBits==0x3f800000u?1u:0u;
    const uint32_t water=s.opaqueAlphaEligible&&s.fogForAlpha==2?1u:0u;
    const uint32_t depth=s.opaqueAlphaEligible&&s.depthAlphaReturn!=0?1u:0u;
    const uint32_t pixelFog=s.fogForPixel==2?1u:0u;
    SourcePlainLightmappedDynamicSelectionCandidate next;
    next.vertex=vertexFast+2u*(s.fogForVertex==2?1u:0u);
    next.ps20b=s.dxLevel>=92;
    next.pixel=contrast+2u*pixelFast+4u*water+
        (next.ps20b?8u*depth+16u*pixelFog:8u*pixelFog);
    out=next;return true;
}

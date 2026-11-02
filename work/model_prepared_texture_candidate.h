#pragma once
#include "shader_texture_cpu_preparation_candidate.h"
#include <cmath>

// CPU-only renderer snapshot. No native texture-cache hit or GPU-ready promise.
struct ModelPreparedTextureCandidate {
    std::shared_ptr<const ShaderTextureCpuPreparationCandidate>cpu;
    uint32_t frame=0;
    // Historical name: selected/GPU-relative mip, NOT the authored file level.
    // image()->plan()[11] and lease.sourceLevel() retain the authored identity.
    uint32_t sourceMip=0;
    bool fullMipSampling=false;
};
inline ModelPreparedTextureCandidate prepareModelTextureSnapshotCandidate(const std::string&name,
    const std::vector<uint8_t>&bytes,uint32_t requestedMip,uint32_t frame){
    ModelPreparedTextureCandidate result;result.frame=frame;result.sourceMip=requestedMip;
    auto resource=MaterialTextureResourceCandidate::fromVtf(name,bytes);const auto&layout=resource->layout();
    if(frame>=layout.frameCount)throw std::runtime_error("prepared model texture frame");
    const auto fmt=layout.format;const auto power=[](uint32_t n){return n&&n<=32768&&!(n&(n-1));};
    // Preserve the existing decoded-image route outside the measured ordinary
    // complete 2D-chain domain. No fabricated mips or silently changed format.
    if((fmt!=0&&fmt!=3&&fmt!=12&&fmt!=13&&fmt!=15)||layout.mips.empty()||
       !power(layout.mips[0].width)||!power(layout.mips[0].height)||(layout.flags&0x4000u))return result;
    uint32_t minor=0;std::memcpy(&minor,bytes.data()+8,4);
    if(minor>1){uint16_t depth=0;std::memcpy(&depth,bytes.data()+63,2);if(depth!=1)return result;}
    if(layout.lodControl&&((*layout.lodControl)[0]>30||(*layout.lodControl)[1]>30))return result;
    uint32_t levels=1;for(auto n=std::max(layout.mips[0].width,layout.mips[0].height);n>1;n>>=1)++levels;
    if(layout.flags&0x100u){if(requestedMip)return result;levels=1;}
    if(levels>layout.mips.size())return result;
    // Explicit owned-renderer policy: retain all declared ordinary levels,
    // preserve the authored format, no external override table. Actual physical
    // device format/extent support is checked later by the Vulkan owner.
    MaterialTextureCpuImageSettingsCandidate settings;settings.maxWidth=settings.maxHeight=32768;
    settings.forceFullMipChain=settings.hardwareKnown=settings.overridesKnownAbsent=true;
    TextureFormatPolicyServicesCandidate services;services.enabled=[]()->std::optional<uint32_t>{return 0;};
    MaterialShaderTextureRequestCandidate request;request.name=name;request.group="Owned model renderer snapshots";
    if(!ShaderTextureCpuPreparationCandidate::prepareColdNormalMiss(request,MaterialTextureReferenceCandidate::ordinary(resource),settings,1,services,result.cpu)){
        // A declared NOMIP file may lack the level requested by its LOD clamp.
        // Preserve the preceding decoded fallback; never invent missing pixels
        // or abort a model load merely by opting into this new prepared route.
        if(layout.lodControl)return result;
        throw std::runtime_error("prepared model texture snapshot refused");
    }
    if(layout.lodControl){
        const auto&plan=result.cpu->image()->plan();
        // Existing callers request an authored mip and clamp their decoded
        // fallback to the last authored level. Discarded levels select the new
        // root; retained levels subtract its source skip exactly once.
        const auto authored=std::min(requestedMip,uint32_t(layout.mips.size()-1));
        result.sourceMip=std::min(authored>plan[11]?authored-plan[11]:0u,plan[9]-1);
    }
    // Reference preset verified for force-anisotropy1, filtering1, mipmapping1:
    // ordinary textures use trilinear sampling across the retained authored chain.
    // This is not live engine CVar/hardware negotiation. Anisotropic requests
    // retain the preceding policy until their backend contract is proven.
    result.fullMipSampling=(layout.flags&0x10u)==0;
    return result;
}

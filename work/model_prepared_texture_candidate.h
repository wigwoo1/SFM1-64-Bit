#pragma once
#include "shader_texture_cpu_preparation_candidate.h"
#include <cmath>

// CPU-only renderer snapshot. No native texture-cache hit or GPU-ready promise.
struct ModelPreparedTextureCandidate {
    std::shared_ptr<const ShaderTextureCpuPreparationCandidate>cpu;
    uint32_t frame=0;
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
    if(minor>=3){uint32_t count=0;std::memcpy(&count,bytes.data()+68,4);for(uint32_t i=0;i<count;++i){uint32_t tag=0;std::memcpy(&tag,bytes.data()+80+size_t(i)*8,4);if((tag&0xffffffu)==0x444f4cu)return result;}}
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
    if(!ShaderTextureCpuPreparationCandidate::prepareColdNormalMiss(request,MaterialTextureReferenceCandidate::ordinary(resource),settings,1,services,result.cpu))throw std::runtime_error("prepared model texture snapshot refused");
    return result;
}

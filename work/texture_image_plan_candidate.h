#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include "vtf_effective_flags_candidate.h"

// Opt-in CPU plan for measured fresh 2D textures. This does not load an image,
// allocate GPU handles, determine active native hardware or admit unknown LOD
// resources/overrides. Preserve raw, VTF-effective and requested flags separately.
struct TextureImagePlanInputCandidate {
    uint32_t major=7,minor=2,rawFlags=0,requestedFlags=0;
    uint32_t width=0,height=0,frames=0,format=0;
    std::array<uint32_t,3>reflectivityBits{};
    uint32_t picmip=0,maxWidth=0,maxHeight=0,dimensionClamp=0;
    bool forceFullMipChain=false,graffitiPolicy=false;
    bool lodResourceKnownAbsent=false,overridesKnownAbsent=false,hardwareKnown=false;
};
// Native CTexture fields, ordered like the independent reference record.
using TextureImagePlanCandidate=std::array<uint32_t,16>;
inline bool textureImagePlanCandidate(const TextureImagePlanInputCandidate&v,
    TextureImagePlanCandidate&output)noexcept{
    const auto power=[](uint32_t n){return n&&n<=32768&&!(n&(n-1));};
    uint32_t flags=0;
    for(auto bits:v.reflectivityBits)if((bits&0x7f800000u)==0x7f800000u)return false;
    if(!v.lodResourceKnownAbsent||!v.overridesKnownAbsent||!v.hardwareKnown||
        !power(v.width)||!power(v.height)||!v.frames||v.frames>2048||
        (v.format!=0&&v.format!=12&&v.format!=13)||v.picmip>3||
        v.maxWidth<4||v.maxHeight<4||v.maxWidth>32768||v.maxHeight>32768||
        (v.dimensionClamp&&!power(v.dimensionClamp))||
        !vtfEffectiveFlagsCandidate(v.major,v.minor,v.rawFlags,flags))return false;
    flags|=v.requestedFlags|(v.graffitiPolicy?0x200u:0u);
    if(flags&0x4000u)return false; // Cubemap hardware/storage remains separate.
    uint32_t tw=v.width,th=v.height;
    if(v.dimensionClamp)while(tw>v.dimensionClamp||th>v.dimensionClamp){tw>>=1;th>>=1;}
    if(!(flags&0x200u))for(uint32_t i=0;i<v.picmip;++i)if(tw>4&&th>4){tw>>=1;th>>=1;}
    tw=std::min(tw,v.maxWidth);th=std::min(th,v.maxHeight);
    uint32_t aw=v.width,ah=v.height,skip=0;
    while(aw>tw||ah>th){aw>>=1;ah>>=1;++skip;if(!aw||!ah)return false;}
    uint32_t levels=1;
    if(!(flags&0x100u)){
        uint32_t mw=aw,mh=ah;
        if(v.forceFullMipChain||(flags&0x400u)){
            auto n=std::max(mw,mh);while(n>1){++levels;n>>=1;}
        }else while(std::min(mw,mh)>32){++levels;mw>>=1;mh>>=1;}
    }
    TextureImagePlanCandidate next{flags,0,v.format,v.width,v.height,1,
        aw,ah,1,levels,v.frames,skip,v.reflectivityBits[0],
        v.reflectivityBits[1],v.reflectivityBits[2],1};
    output=next;return true;
}

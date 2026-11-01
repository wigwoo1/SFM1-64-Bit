#pragma once
#include "material_texture_resource_candidate.h"
#include "texture_image_plan_candidate.h"

struct MaterialTextureCpuImageSettingsCandidate {
    uint32_t requestedFlags=0,picmip=0,maxWidth=0,maxHeight=0,dimensionClamp=0;
    bool forceFullMipChain=false,graffitiPolicy=false;
    bool overridesKnownAbsent=false,hardwareKnown=false;
};

// A lease keeps immutable file storage alive after its image, resolver, material
// value or snapshot is released. data() is borrowed for this lease's lifetime.
class MaterialTextureMipLeaseCandidate {
    std::shared_ptr<const MaterialTextureResourceCandidate>storage;
    size_t start=0,length=0;
    uint32_t w=0,h=0,sourceMip=0,frameNumber=0;
    friend class MaterialTextureCpuImageCandidate;
    MaterialTextureMipLeaseCandidate(std::shared_ptr<const MaterialTextureResourceCandidate>p,
        size_t o,size_t n,uint32_t width,uint32_t height,uint32_t mip,uint32_t frame)noexcept:
        storage(std::move(p)),start(o),length(n),w(width),h(height),sourceMip(mip),frameNumber(frame){}
public:
    const uint8_t*data()const noexcept{return storage?storage->bytes().data()+start:nullptr;}
    size_t size()const noexcept{return storage?length:0;}
    uint32_t width()const noexcept{return w;}
    uint32_t height()const noexcept{return h;}
    uint32_t sourceLevel()const noexcept{return sourceMip;}
    uint32_t frame()const noexcept{return frameNumber;}
};

// Immutable selected CPU pixels and the measured CTexture plan. No file IO,
// conversion, error-image fabrication, GPU objects or production publication.
class MaterialTextureCpuImageCandidate {
    std::shared_ptr<const MaterialTextureResourceCandidate>storage;
    TextureImagePlanCandidate description{};
    uint32_t fileFlags=0,vtfFlags=0,requestFlags=0;
    MaterialTextureCpuImageCandidate(std::shared_ptr<const MaterialTextureResourceCandidate>p,
        const TextureImagePlanCandidate&plan,uint32_t raw,uint32_t effective,uint32_t requested)noexcept:
        storage(std::move(p)),description(plan),fileFlags(raw),vtfFlags(effective),requestFlags(requested){}
public:
    static bool prepare(const MaterialTextureReferenceCandidate&input,
        MaterialTextureCpuImageSettingsCandidate settings,
        std::shared_ptr<const MaterialTextureCpuImageCandidate>&output)noexcept{
        try{
            if(!input.valid()||input.isEnvironment()||!input.resource())return false;
            const auto retained=input.resource();const auto&bytes=retained->bytes();const auto&layout=retained->layout();
            const auto word=[&](size_t at){uint32_t v;std::memcpy(&v,bytes.data()+at,4);return v;};
            const auto half=[&](size_t at){uint16_t v;std::memcpy(&v,bytes.data()+at,2);return uint32_t(v);};
            // Resource validation has established header/resource table spans.
            // The old 2D layout admits nonzero depth; this image stage requires1.
            const auto minor=word(8);if(minor>1&&half(63)!=1)return false;
            if(minor>=3)for(uint32_t i=0;i<word(68);++i)
                if((word(80+size_t(i)*8)&0xffffffu)==0x444f4cu)return false;
            if(layout.mips.empty())return false;
            TextureImagePlanInputCandidate v;v.major=word(4);v.minor=minor;v.rawFlags=layout.flags;
            v.requestedFlags=settings.requestedFlags;v.width=layout.mips[0].width;v.height=layout.mips[0].height;
            v.frames=layout.frameCount;v.format=layout.format;v.reflectivityBits={word(32),word(36),word(40)};
            v.picmip=settings.picmip;v.maxWidth=settings.maxWidth;v.maxHeight=settings.maxHeight;
            v.dimensionClamp=settings.dimensionClamp;v.forceFullMipChain=settings.forceFullMipChain;
            v.graffitiPolicy=settings.graffitiPolicy;v.lodResourceKnownAbsent=true;
            v.overridesKnownAbsent=settings.overridesKnownAbsent;v.hardwareKnown=settings.hardwareKnown;
            TextureImagePlanCandidate plan;
            if(!textureImagePlanCandidate(v,plan))return false;
            const auto skip=plan[11],levels=plan[9];
            // Select only declared, bounds-checked levels. Never expose native
            // unfilled allocation bytes or synthesize missing mips/pixels.
            if(skip>=layout.mips.size()||levels>layout.mips.size()-skip)return false;
            for(uint32_t i=0;i<levels;++i){const auto&m=layout.mips[skip+i];
                if(m.width!=std::max(1u,plan[6]>>i)||m.height!=std::max(1u,plan[7]>>i)||
                    m.offset>bytes.size()||m.size>(bytes.size()-m.offset)/layout.frameCount)return false;}
            uint32_t normalized=0;if(!vtfEffectiveFlagsCandidate(v.major,v.minor,v.rawFlags,normalized))return false;
            auto next=std::shared_ptr<const MaterialTextureCpuImageCandidate>(new MaterialTextureCpuImageCandidate(retained,plan,layout.flags,normalized,settings.requestedFlags));
            output=std::move(next);return true;
        }catch(...){return false;}
    }
    const TextureImagePlanCandidate&plan()const noexcept{return description;}
    uint32_t rawFlags()const noexcept{return fileFlags;}
    uint32_t effectiveVtfFlags()const noexcept{return vtfFlags;}
    uint32_t requestedFlags()const noexcept{return requestFlags;}
    std::optional<MaterialTextureMipLeaseCandidate>lease(uint32_t frame,uint32_t selectedMip)const noexcept{
        if(frame>=description[10]||selectedMip>=description[9])return {};
        const auto level=description[11]+selectedMip;const auto&m=storage->layout().mips[level];
        return MaterialTextureMipLeaseCandidate(storage,m.offset+size_t(frame)*m.size,m.size,m.width,m.height,level,frame);
    }
};

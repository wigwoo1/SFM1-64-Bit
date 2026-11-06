#pragma once
#include "source_bsp_base_lightmap_candidate.h"
#include "source_lightmap_accumulation_candidate.h"

inline std::vector<SourceLightmapLinearPixelCandidate> accumulateSourceBspPlaneCandidate(
    const SourceBspLightmapDataCandidate& data,size_t slot,uint32_t plane,uint32_t width,uint32_t height,
    float styleScale,SourceLightmapAccumulationUnitsCandidate units){
        std::vector<SourceLightmapRawSampleCandidate> raw;raw.reserve(size_t(width)*height);
        for(uint32_t y=0;y<uint32_t(height);++y)for(uint32_t x=0;x<uint32_t(width);++x)raw.push_back(data.sample(slot,0,plane,x,y));
        std::vector<SourceLightmapLinearPixelCandidate> staged(size_t(width)*height,{0,0,0,1});
        if(!sourceLightmapAccumulationCandidate(staged,raw,styleScale,units))
            throw std::runtime_error("accumulated lightmap arithmetic/control refused");
        return staged;
}

// Immutable CPU plane, not renderer/material admission or original live state.
// styleScale is an explicit caller multiplier, separate from HDR/exposure.
// Alpha1 is a declared opaque-plane policy; RGBExp32's fourth byte is exponent.
// Preserve the older double-based preview decoder and normalized pass unchanged.
class SourceBspAccumulatedLightmapCandidate {
    std::shared_ptr<const SourceBspLightmapSnapshotCandidate> source_;
    uint32_t face_=0,width_=0,height_=0;
    float styleScale_=0;
    SourceLightmapAccumulationUnitsCandidate units_=SourceLightmapAccumulationUnitsCandidate::TexLightLinear;
    std::vector<SourceLightmapLinearPixelCandidate> pixels_;
    SourceBspAccumulatedLightmapCandidate()=default;
public:
    const auto& source()const noexcept{return source_;}
    uint32_t faceIndex()const noexcept{return face_;}
    uint32_t width()const noexcept{return width_;}
    uint32_t height()const noexcept{return height_;}
    float styleScale()const noexcept{return styleScale_;}
    SourceLightmapAccumulationUnitsCandidate units()const noexcept{return units_;}
    const auto& pixels()const noexcept{return pixels_;}
    static std::shared_ptr<const SourceBspAccumulatedLightmapCandidate> prepare(
            std::shared_ptr<const SourceBspLightmapSnapshotCandidate> source,uint32_t face,
            const VmtNodeCandidate& material,float styleScale,SourceLightmapAccumulationUnitsCandidate units){
        validateSourceBaseLightmapMaterialCandidate(material);
        if(!source||units!=SourceLightmapAccumulationUnitsCandidate::TexLightLinear)
            throw std::runtime_error("accumulated lightmap explicit units/snapshot required");
        validateSourceBspLightmapSnapshotCandidate(*source);
        if(source->state!=SourceBspLightmapSnapshotStateCandidate::Available||!source->lightmaps)
            throw std::runtime_error("accumulated lightmap metadata unavailable");
        const auto& data=*source->lightmaps;
        if(!data.pairedFaceLightingTables()||data.faceLump()!=58||data.lightingLump()!=53||
           face<data.firstFace()||uint64_t(face)-data.firstFace()>=data.faces().size())
            throw std::runtime_error("accumulated lightmap requires paired HDR face identity");
        const size_t slot=size_t(face-data.firstFace());const auto& f=data.face(slot);
        if(f.availability!=SourceBspLightmapAvailabilityCandidate::Samples||f.styleCount!=1||
           f.styles[0]!=0||f.planesPerStyle!=1||f.flags!=0||f.displacementIndex!=-1)
            throw std::runtime_error("accumulated lightmap unsupported face/style/plane/flags");
        if(std::find(source->orderedTriangleFaces.begin(),source->orderedTriangleFaces.end(),face)==source->orderedTriangleFaces.end())
            throw std::runtime_error("accumulated lightmap face absent from retained geometry");
        if(f.size[0]<0||f.size[1]<0)throw std::runtime_error("accumulated lightmap negative dimensions");
        const uint64_t width=uint64_t(f.size[0])+1,height=uint64_t(f.size[1])+1,count=width*height;
        if(width>UINT32_MAX||height>UINT32_MAX||!count||count!=f.samplesPerPlane||count>16384||width*8>65535)
            throw std::runtime_error("accumulated lightmap dimensions/stride unsupported");
        auto staged=accumulateSourceBspPlaneCandidate(data,slot,0,uint32_t(width),uint32_t(height),styleScale,units);
        auto next=std::shared_ptr<SourceBspAccumulatedLightmapCandidate>(new SourceBspAccumulatedLightmapCandidate);
        next->source_=std::move(source);next->face_=face;next->width_=uint32_t(width);next->height_=uint32_t(height);
        next->styleScale_=styleScale;next->units_=units;next->pixels_=std::move(staged);return next;
    }
};

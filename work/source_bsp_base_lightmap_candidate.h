#pragma once
// Independently authored normalized TexLightLinear operator. Not SFM exposure,
// packed-lightmap, lightstyle or tone-map emulation. See the private contract.
#include "source_bsp_lightmap_snapshot_candidate.h"
#include "vmt_keyvalues_candidate.h"
#include "material_uv_transform_candidate.h"
#include <algorithm>

enum class SourceBaseLightmapUnitsCandidate {TexLightLinear};
// Explicit metadata subset for this normalized draw operator only. Authored
// nodes remain in the material; this does not implement Hammer filtering or
// physics surface-name resolution, or exempt arbitrary '%' compiler switches.
inline bool sourceBaseLightmapMetadataKeyCandidate(const std::string& name){
    const auto key=vmtAsciiLowerCandidate(name);return key=="%keywords"||key=="$surfaceprop";
}
inline bool sourceBaseLightmapMetadataLeafCandidate(const VmtNodeCandidate& node){
    if(!sourceBaseLightmapMetadataKeyCandidate(node.key)||node.block||!node.children.empty()||node.value.empty()||node.value.size()>4095)return false;
    for(unsigned char c:node.value)if(c<32||c>126)return false;
    return true;
}
inline void validateSourceBaseLightmapMaterialCandidate(const VmtNodeCandidate& material){
    if(vmtAsciiLowerCandidate(material.key)!="lightmappedgeneric"||!material.block||material.children.empty()||material.children.size()>4)
        throw std::runtime_error("base lightmap requires plain LightmappedGeneric");
    size_t bases=0,transforms=0,keywords=0,surfaces=0;
    for(const auto& node:material.children){const auto key=vmtAsciiLowerCandidate(node.key);
        if(node.block||node.value.empty()||!node.children.empty())throw std::runtime_error("base lightmap material leaf required");
        if(key=="$basetexture")++bases;else if(key=="$basetexturetransform")++transforms;
        else if(sourceBaseLightmapMetadataKeyCandidate(key)){
            if(!sourceBaseLightmapMetadataLeafCandidate(node))throw std::runtime_error("base lightmap metadata scalar required");
            if(key=="%keywords")++keywords;else ++surfaces;
        }else throw std::runtime_error("base lightmap unsupported material parameter");
    }
    if(bases!=1||transforms>1)throw std::runtime_error("base lightmap basetexture/transform count");
    if(keywords>1||surfaces>1)throw std::runtime_error("base lightmap duplicate metadata");
    (void)materialUvTransformCandidate(material);
}
inline std::array<float,4> decodeSourceTexLightLinearCandidate(const std::array<uint8_t,4>& raw){
    // Convert the signed byte without implementation-defined unsigned->signed cast.
    const int exponent=raw[3]<128?int(raw[3]):int(raw[3])-256;
    const double scale=std::ldexp(1.,exponent)/255.;
    return {float(raw[0]*scale),float(raw[1]*scale),float(raw[2]*scale),1.f};
}
class SourceBspBaseLightmapCandidate {
    std::shared_ptr<const SourceBspLightmapSnapshotCandidate> source_;
    uint32_t face_=0,width_=0,height_=0;
    std::vector<std::array<float,4>> pixels_;
    SourceBspBaseLightmapCandidate()=default;
public:
    const auto& source()const noexcept{return source_;}
    uint32_t faceIndex()const noexcept{return face_;}
    uint32_t width()const noexcept{return width_;}
    uint32_t height()const noexcept{return height_;}
    const auto& pixels()const noexcept{return pixels_;}
    static std::shared_ptr<const SourceBspBaseLightmapCandidate> prepare(
        std::shared_ptr<const SourceBspLightmapSnapshotCandidate> source,uint32_t face,
        const VmtNodeCandidate& material,SourceBaseLightmapUnitsCandidate units){
        validateSourceBaseLightmapMaterialCandidate(material);
        if(units!=SourceBaseLightmapUnitsCandidate::TexLightLinear||!source)
            throw std::runtime_error("explicit lightmap units and snapshot required");
        validateSourceBspLightmapSnapshotCandidate(*source);
        if(source->state!=SourceBspLightmapSnapshotStateCandidate::Available||!source->lightmaps)
            throw std::runtime_error("base lightmap metadata unavailable");
        const auto& data=*source->lightmaps;
        if(!data.pairedFaceLightingTables()||data.faceLump()!=58||data.lightingLump()!=53||face<data.firstFace()||uint64_t(face)-data.firstFace()>=data.faces().size())
            throw std::runtime_error("base lightmap requires paired HDR face identity");
        const size_t slot=size_t(face-data.firstFace());const auto& f=data.face(slot);
        if(f.availability!=SourceBspLightmapAvailabilityCandidate::Samples||f.styleCount!=1||f.styles[0]!=0||f.planesPerStyle!=1||f.flags!=0||f.displacementIndex!=-1)
            throw std::runtime_error("base lightmap unsupported face/style/plane/flags");
        if(std::find(source->orderedTriangleFaces.begin(),source->orderedTriangleFaces.end(),face)==source->orderedTriangleFaces.end())
            throw std::runtime_error("base lightmap face absent from retained geometry");
        auto next=std::shared_ptr<SourceBspBaseLightmapCandidate>(new SourceBspBaseLightmapCandidate);
        next->source_=std::move(source);next->face_=face;next->width_=uint32_t(f.size[0])+1;next->height_=uint32_t(f.size[1])+1;
        next->pixels_.reserve(size_t(f.samplesPerPlane));
        for(uint32_t y=0;y<next->height_;++y)for(uint32_t x=0;x<next->width_;++x)
            next->pixels_.push_back(decodeSourceTexLightLinearCandidate(data.sample(slot,0,0,x,y)));
        return next;
    }
};

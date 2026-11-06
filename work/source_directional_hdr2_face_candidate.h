#pragma once
#include "source_bsp_accumulated_lightmap_candidate.h"
#include "native_lightmap_hdr2_region_candidate.h"

// Encodes four side-by-side planes for a single static HDR brush face.
inline bool encodeSourceDirectionalHdr2FaceCandidate(const SourceBspLightmapDataCandidate& data,
    size_t slot,float scale,uint32_t encoding,const NativeLightmapHdr2RegionCandidate& region,
    std::vector<uint8_t>& bytes){
    const auto& face=data.face(slot);
    if(!data.pairedFaceLightingTables()||data.faceLump()!=58||data.lightingLump()!=53||
       face.availability!=SourceBspLightmapAvailabilityCandidate::Samples||face.flags!=0x800||
       face.displacementIndex!=-1||face.styleCount!=1||face.styles[0]!=0||face.planesPerStyle!=4||
       face.size[0]<0||face.size[1]<0||region.width!=uint64_t(face.size[0])+1||
       region.height!=uint64_t(face.size[1])+1||face.samplesPerPlane>16384||
       uint64_t(region.width)*region.height!=face.samplesPerPlane)return false;
    std::array<std::vector<NativeLightmapHdr2BitsCandidate>,4> storage;
    for(unsigned plane=0;plane<4;++plane){
        auto pixels=accumulateSourceBspPlaneCandidate(data,slot,plane,region.width,region.height,scale,
            SourceLightmapAccumulationUnitsCandidate::TexLightLinear);
        storage[plane].resize(pixels.size());
        for(size_t i=0;i<pixels.size();++i)std::memcpy(storage[plane][i].data(),pixels[i].data(),16);
    }
    return nativeLightmapHdr2RegionCandidate(encoding,{&storage[0],&storage[1],&storage[2],&storage[3]},region,bytes);
}

#pragma once
#include "source_bsp_lightmap_snapshot_candidate.h"
#include "scene_shading_attributes_candidate.h"

struct SceneBspLightmapAssociationCandidate {
    size_t model=0;
    uint32_t firstVertex=0,vertexCount=0,firstIndex=0,indexCount=0;
    std::shared_ptr<const SourceBspLightmapSnapshotCandidate>snapshot;
};
template<class Upload>void validateSceneBspLightmapsCandidate(const Upload& upload){
    for(size_t n=0;n<upload.bspLightmaps.size();++n){
        const auto&a=upload.bspLightmaps[n];
        if(!a.snapshot||a.firstVertex>upload.vertices.size()||a.vertexCount>upload.vertices.size()-a.firstVertex||a.firstIndex>upload.indices.size()||a.indexCount>upload.indices.size()-a.firstIndex||a.indexCount%3)throw std::runtime_error("BSP scene association buffer coverage");
        validateSourceBspLightmapSnapshotCandidate(*a.snapshot);
        if(a.vertexCount!=a.snapshot->vertexCount||a.indexCount!=a.snapshot->localIndices.size())throw std::runtime_error("BSP scene association snapshot coverage");
        for(size_t j=0;j<a.indexCount;++j)if(uint64_t(upload.indices[size_t(a.firstIndex)+j])!=uint64_t(a.firstVertex)+a.snapshot->localIndices[j])throw std::runtime_error("BSP scene association global/local index mismatch");
        for(size_t j=0;j<n;++j){const auto&b=upload.bspLightmaps[j];
            const bool verticesOverlap=uint64_t(a.firstVertex)<uint64_t(b.firstVertex)+b.vertexCount&&uint64_t(b.firstVertex)<uint64_t(a.firstVertex)+a.vertexCount;
            const bool indicesOverlap=a.indexCount&&b.indexCount&&uint64_t(a.firstIndex)<uint64_t(b.firstIndex)+b.indexCount&&uint64_t(b.firstIndex)<uint64_t(a.firstIndex)+a.indexCount;
            if(a.model==b.model||verticesOverlap||indicesOverlap)throw std::runtime_error("overlapping/duplicate BSP scene association");
        }
        auto draws=[&](const auto& list){for(const auto&d:list){
            const uint64_t end=uint64_t(d.firstIndex)+d.indexCount,associationEnd=uint64_t(a.firstIndex)+a.indexCount;
            if(d.indexCount&&d.model==a.model&&(d.firstIndex<a.firstIndex||end>associationEnd||d.indexCount%3)){
                // Generated ribbons can reuse the world's material group. They
                // do not acquire source lightmaps merely by sharing that group.
                bool generated=d.indexCount%3==0&&d.firstIndex<=upload.indices.size()&&d.indexCount<=upload.indices.size()-d.firstIndex&&upload.vertexShading.size()==upload.vertices.size();
                if(generated)for(size_t k=0;k<d.indexCount;++k){const auto index=upload.indices[size_t(d.firstIndex)+k];if(index>=upload.vertexShading.size()||upload.vertexShading[index].origin!=SceneShadingOriginCandidate::GeneratedGeometry||upload.vertexShading[index].model!=a.model){generated=false;break;}}
                if(!generated)throw std::runtime_error("BSP source draw outside association");
            }
            if(d.indexCount&&uint64_t(d.firstIndex)<associationEnd&&uint64_t(a.firstIndex)<end)
                if(d.firstIndex<a.firstIndex||end>associationEnd||d.model!=a.model||(d.firstIndex-a.firstIndex)%3)throw std::runtime_error("BSP draw/association identity mismatch");
        }};draws(upload.opaque);draws(upload.translucent);
    }
}

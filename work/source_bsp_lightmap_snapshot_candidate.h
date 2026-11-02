#pragma once
#include "source_bsp_lightmap_binding_candidate.h"
#include <map>
#include <string>

enum class SourceBspLightmapSnapshotStateCandidate {Available,UnavailableLegacyPreview};
// Shared CPU metadata only. No extracted geometry, renderer atlas or decoded
// light colors are duplicated here. Publish through shared_ptr<const ...>.
struct SourceBspLightmapSnapshotCandidate {
    SourceBspLightmapSnapshotStateCandidate state=SourceBspLightmapSnapshotStateCandidate::UnavailableLegacyPreview;
    std::string unavailableReason;
    uint32_t vertexCount=0;
    std::vector<uint32_t>localIndices;
    std::shared_ptr<const SourceBspLightmapDataCandidate>lightmaps;
    std::vector<SourceBspVertexLuxelCandidate>vertexLuxels;
    std::vector<uint32_t>orderedTriangleFaces;
};
inline void validateSourceBspLightmapSnapshotCandidate(const SourceBspLightmapSnapshotCandidate& s){
    if(!s.vertexCount||s.localIndices.size()>UINT32_MAX||s.localIndices.size()%3)throw std::runtime_error("BSP lightmap snapshot geometry coverage");
    for(auto index:s.localIndices)if(index>=s.vertexCount)throw std::runtime_error("BSP lightmap snapshot vertex index");
    if(s.state==SourceBspLightmapSnapshotStateCandidate::UnavailableLegacyPreview){
        if(s.unavailableReason.empty()||s.lightmaps||!s.vertexLuxels.empty()||!s.orderedTriangleFaces.empty())throw std::runtime_error("ambiguous unavailable BSP lightmap snapshot");
        return;
    }
    if(s.state!=SourceBspLightmapSnapshotStateCandidate::Available||!s.unavailableReason.empty()||!s.lightmaps||s.vertexLuxels.size()!=s.vertexCount||s.orderedTriangleFaces.size()!=s.localIndices.size()/3)throw std::runtime_error("BSP lightmap snapshot metadata coverage");
    const auto first=s.lightmaps->firstFace();
    for(const auto& v:s.vertexLuxels){
        if(v.faceIndex<first||uint64_t(v.faceIndex)-first>=s.lightmaps->faces().size())throw std::runtime_error("BSP lightmap snapshot face outside owner");
        for(double value:v.luxel)if(!std::isfinite(value))throw std::runtime_error("nonfinite BSP snapshot luxel");
    }
    for(size_t n=0;n<s.orderedTriangleFaces.size();++n)for(size_t k=0;k<3;++k)
        if(s.vertexLuxels[s.localIndices[n*3+k]].faceIndex!=s.orderedTriangleFaces[n])throw std::runtime_error("BSP reordered triangle/source face mismatch");
}
inline std::shared_ptr<const SourceBspLightmapSnapshotCandidate> prepareSourceBspLightmapSnapshotCandidate(
    SourceBspLightmappedBrushCandidate&& brush,const std::vector<uint32_t>& indices){
    validateSourceBspLightmapBindingCandidate(brush);
    if(brush.geometry.positions.size()>UINT32_MAX||brush.geometry.triangles.size()>UINT32_MAX/3||indices.size()!=brush.geometry.triangles.size()*3)throw std::runtime_error("BSP regrouped triangle coverage");
    std::map<std::array<uint32_t,3>,size_t>remaining;
    for(const auto&t:brush.geometry.triangles)++remaining[t.vertices];
    SourceBspLightmapSnapshotCandidate next;next.state=SourceBspLightmapSnapshotStateCandidate::Available;next.vertexCount=uint32_t(brush.geometry.positions.size());next.localIndices=indices;
    next.orderedTriangleFaces.reserve(indices.size()/3);
    for(size_t n=0;n<indices.size();n+=3){
        const std::array<uint32_t,3> triangle{indices[n],indices[n+1],indices[n+2]};
        auto found=remaining.find(triangle);if(found==remaining.end()||!found->second)throw std::runtime_error("regrouped BSP triangle absent/duplicated");--found->second;
        const auto face=brush.vertexLuxels.at(triangle[0]).faceIndex;
        for(auto vertex:triangle)if(brush.vertexLuxels.at(vertex).faceIndex!=face)throw std::runtime_error("regrouped BSP triangle crosses source faces");
        next.orderedTriangleFaces.push_back(face);
    }
    for(const auto& item:remaining)if(item.second)throw std::runtime_error("regrouping omitted BSP source triangle");
    next.lightmaps=std::move(brush.lightmaps);next.vertexLuxels=std::move(brush.vertexLuxels);
    validateSourceBspLightmapSnapshotCandidate(next);
    return std::make_shared<const SourceBspLightmapSnapshotCandidate>(std::move(next));
}
inline std::shared_ptr<const SourceBspLightmapSnapshotCandidate> unavailableSourceBspLightmapSnapshotCandidate(
    size_t vertexCount,const std::vector<uint32_t>& indices,std::string reason){
    if(vertexCount>UINT32_MAX)throw std::runtime_error("BSP unavailable snapshot vertex overflow");
    SourceBspLightmapSnapshotCandidate next;next.vertexCount=uint32_t(vertexCount);next.localIndices=indices;next.unavailableReason=std::move(reason);
    validateSourceBspLightmapSnapshotCandidate(next);
    return std::make_shared<const SourceBspLightmapSnapshotCandidate>(std::move(next));
}

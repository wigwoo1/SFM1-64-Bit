#pragma once
#include "source_bsp_inline_brush_candidate.h"
#include "source_bsp_lightmap_data_candidate.h"

struct SourceBspVertexLuxelCandidate {
    uint32_t faceIndex=0;
    std::array<double,2> luxel{};
};
struct SourceBspLightmappedBrushCandidate {
    SourceBspBrushGeometryCandidate geometry;
    std::shared_ptr<const SourceBspLightmapDataCandidate> lightmaps;
    std::vector<SourceBspVertexLuxelCandidate> vertexLuxels;
    std::vector<uint32_t> triangleFaces;
};

inline void validateSourceBspLightmapBindingCandidate(const SourceBspLightmappedBrushCandidate& result){
    if(!result.lightmaps||result.vertexLuxels.size()!=result.geometry.positions.size()||
       result.triangleFaces.size()!=result.geometry.triangles.size())throw std::runtime_error("BSP lightmap binding coverage");
    auto faceSlot=[&](uint32_t face){
        const auto first=result.lightmaps->firstFace();
        if(face<first||uint64_t(face)-first>=result.lightmaps->faces().size())throw std::runtime_error("BSP lightmap binding face outside owner");
        return size_t(face-first);
    };
    for(const auto& v:result.vertexLuxels){
        (void)faceSlot(v.faceIndex);
        for(double value:v.luxel)if(!std::isfinite(value))throw std::runtime_error("nonfinite BSP lightmap binding");
    }
    for(size_t n=0;n<result.triangleFaces.size();++n){
        const auto face=result.triangleFaces[n];(void)faceSlot(face);
        for(auto vertex:result.geometry.triangles[n].vertices)
            if(vertex>=result.vertexLuxels.size()||result.vertexLuxels[vertex].faceIndex!=face)throw std::runtime_error("BSP triangle/lightmap face mismatch");
    }
}

// The same explicit dataset is used for geometry and raw lightmap metadata.
// Omitted selection preserves the historical LegacyPreview behavior.
inline SourceBspLightmappedBrushCandidate readSourceBspLightmappedBrushCandidate(
    const std::vector<uint8_t>& bytes,uint32_t modelIndex,
    SourceBspLightmapSelectionCandidate selection=SourceBspLightmapSelectionCandidate::LegacyPreview){
    SourceBspLightmappedBrushCandidate next;
    next.lightmaps=readSourceBspLightmapDataCandidate(bytes,modelIndex,selection);
    SourceBspBrushEmissionTraceCandidate trace;
    trace.vertex=[&](uint32_t face,const std::array<float,3>& sourceBasePosition){
        const auto first=next.lightmaps->firstFace();
        if(face<first||uint64_t(face)-first>=next.lightmaps->faces().size())throw std::runtime_error("BSP emitted face outside lightmap owner");
        next.vertexLuxels.push_back({face,next.lightmaps->luxel(size_t(face-first),sourceBasePosition)});
    };
    trace.triangle=[&](uint32_t face){next.triangleFaces.push_back(face);};
    next.geometry=readSourceBspInlineBrushCandidate(bytes,modelIndex,&trace,selection);
    validateSourceBspLightmapBindingCandidate(next);
    return next; // No callback retains a pointer to this local result.
}

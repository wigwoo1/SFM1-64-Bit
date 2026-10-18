#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>

// IFM draw branch 10243967..10243A56, after model/draw-category gating.
// A missing override means the downstream engine chooses its usual model origin.
// Renderable bounds are supplied by the caller. Their computation, renderable
// flag20 provenance and live cached studio-header binding are separate contracts.
template<class AttachmentIndex,class DagPosition,class RenderableBounds>
std::optional<std::array<float,3>> ifmModelLightingOriginCandidate(
    bool attributePresent,std::optional<uint32_t> selectedDag,bool computeBounds,
    uint8_t renderableFlags,AttachmentIndex attachmentIndex,DagPosition dagPosition,
    RenderableBounds renderableBounds){
    auto checked=[](const std::array<float,3>&point){
        for(float v:point)if(!std::isfinite(v))throw std::runtime_error("nonfinite explicit IFM lighting origin");
        return point;
    };
    if(attributePresent){
        if(!selectedDag)return {};
        return checked(dagPosition(*selectedDag));
    }
    // Do not query the header on the present-attribute path, even when its
    // reference and child-zero fallback fail to produce a DAG.
    if(attachmentIndex()>0||(!computeBounds&&!(renderableFlags&0x20)))return {};
    const auto bounds=renderableBounds();
    checked(bounds[0]);checked(bounds[1]);std::array<float,3>point{};
    for(unsigned axis=0;axis<3;++axis){const float sum=bounds[1][axis]+bounds[0][axis];point[axis]=sum*0.5f;}
    return checked(point);
}

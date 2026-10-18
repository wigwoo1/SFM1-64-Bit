#pragma once
#include "session_model_illumination_dag_candidate.h"
#include "ifm_model_lighting_origin_candidate.h"
#include "ifm_renderable_bounds_candidate.h"
// Requires the existing Graph and namedAttribute declarations. Inputs are a
// stable draw snapshot, not live renderable synchronization or a native ABI.
// AttachmentIndex returns optional<int32_t>: missing header differs from index0.
struct SessionIfmLightingOriginCandidate {
    bool attributePresent=false,headerQueried=false,boundsQueried=false;
    std::optional<uint32_t>selectedDag;
    std::optional<std::array<float,3>>point;
};
template<class IsDag,class AttachmentIndex,class WorldMatrix,class BoundsServices>
SessionIfmLightingOriginCandidate sessionIfmLightingOriginCandidate(
    const Graph&graph,uint32_t model,const IfmRenderableBoundsStateCandidate&state,
    IsDag isDag,AttachmentIndex attachmentIndex,WorldMatrix worldMatrix,BoundsServices&boundsServices){
    const auto selection=sessionModelIlluminationDagCandidate(graph,model,isDag);
    SessionIfmLightingOriginCandidate result;result.attributePresent=selection.attributePresent;
    result.point=ifmModelLightingOriginCandidate(selection.attributePresent,selection.dag,state.computeBounds,state.renderableFlags,
        [&]{result.headerQueried=true;const auto index=attachmentIndex();if(!index)throw std::runtime_error("lighting origin requires a resolved studio header");return *index;},
        [&](uint32_t id){result.selectedDag=id;const auto m=worldMatrix(id);return std::array<float,3>{m[0][3],m[1][3],m[2][3]};},
        [&]{result.boundsQueried=true;return ifmRenderableBoundsWrapperCandidate(state,boundsServices);});
    return result;
}

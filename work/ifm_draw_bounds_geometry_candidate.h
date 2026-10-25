#pragma once
#include "ifm_draw_frames_candidate.h"
#include "ifm_bounds_geometry_services_candidate.h"
struct IfmDrawBoundsGeometryCandidate {
    IfmBoundsWrapperFramesCandidate frames{},framesAtBounds{};
    bool boundsQueried=false;IfmBoundsCandidate innerBounds{};
};
// Connect full draw-frame selection to the INNER bounds dispatcher. Providers
// supply a stable resolved model/DAG/studio/pose snapshot, not a precomputed
// final center. Native cache synchronization/lifetime stays a caller contract.
template<class Providers>
IfmDrawBoundsGeometryCandidate ifmDrawBoundsGeometryCandidate(
    const IfmRenderableBoundsStateCandidate&state,Providers&providers){
    IfmDrawBoundsGeometryCandidate result;
    struct FramesServices {
        const IfmRenderableBoundsStateCandidate&state;Providers&providers;IfmDrawBoundsGeometryCandidate&result;
        auto resolveDag(){return providers.resolveDag();}auto world(uint32_t id){return providers.world(id);}
        auto childCount(uint32_t id){return providers.childCount(id);}auto typedChildZero(uint32_t id){return providers.typedChildZero(id);}
        auto innerBounds(const IfmBoundsWrapperFramesCandidate&frames){
            result.boundsQueried=true;result.framesAtBounds=frames;providers.acceptIntermediateFrames(frames);
            IfmBoundsGeometryServicesCandidate<Providers>bounds{providers,state.runtimeMode};
            result.innerBounds=ifmRenderableBoundsCandidate(state,bounds);return result.innerBounds;
        }
    }services{state,providers,result};
    result.frames=ifmDrawFramesCandidate(state.runtimeMode,services);return result;
}

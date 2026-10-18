#pragma once
#include "ifm_renderable_bounds_candidate.h"
#include "ifm_bounds_frames_candidate.h"
// Connect recovered dispatcher, animated hitboxes and frame selection without
// substituting already-computed animated bounds or child/wrapper matrices.
// Providers own the stable snapshot/cache and bind engine/model/DAG inputs.
// This does not supply live synchronization, bone evaluation or an engine ABI.
template<class Providers>
struct IfmBoundsGeometryServicesCandidate {
    Providers&providers;int32_t runtimeMode;
    void synchronize(){providers.synchronize();}
    auto studio(){return providers.studio();}
    auto modelBounds(){return providers.modelBounds();}
    auto firstBonePose(){return providers.firstBonePose();}
    std::optional<IfmBoundsCandidate>animatedBounds(){
        // OriginalEF60..EF86 attempts lazy acquisition here, before testing the
        // first set. Providers retain the GetModel availability gate. A failed
        // acquisition may be retried by the later studio path.
        const auto*boxes=providers.animatedHitboxes();
        // Header/empty-set gate precedes clock and bone setup.
        if(!boxes||boxes->empty())return {};
        const bool clock=providers.hasClock();const int32_t tick=clock?providers.clockTick():0;
        return ifmAnimatedHitboxBoundsCandidate(boxes,clock,tick,
            [&](auto&palette,uint32_t maximum,uint32_t mask,float time){return providers.setupBones(palette,maximum,mask,time);},
            [&]{return providers.modelWorld();},[&]{return providers.localScale();});
    }
    auto childTransform(){return ifmBoundsChildMatrixCandidate(runtimeMode,providers);}
    auto wrapperTransform(){return ifmBoundsWrapperMatrixCandidate(providers.wrapperFrames());}
};

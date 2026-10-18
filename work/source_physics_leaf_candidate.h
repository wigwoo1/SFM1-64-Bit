#pragma once
#include "source_physics_search_candidate.h"
#include "source_physics_tree_traversal_candidate.h"
struct SourcePhysicsLeafStateCandidate {
    SourcePhysicsSearchStateCandidate search;
    SourcePhysicsSearchTraceCandidate trace;
    std::array<float,16>ray{};
    SourcePhysicsTreeStateCandidate tree;
    std::array<float,3>localDelta{};
    float nearest=0,tolerance=0;
};
// Original2A389..2A4DF accepted-hit policy. Codes3=not closer,4=accepted,
// 5=accepted and ray shortened. The original external plane distance and all
// unrelated trace fields remain intact until full TraceBox finalization.
inline uint32_t sourcePhysicsLeafHitCandidate(SourcePhysicsLeafStateCandidate&state,uint32_t contents){
    auto finite=[](float x){if(!std::isfinite(x))throw std::runtime_error("nonfinite leaf hit arithmetic");return x;};
    if(!(finite(state.nearest)>finite(state.search.distance)))return 3;
    auto next=state;for(float x:next.search.trace.plane.normal)finite(x);next.trace.plane.normal=next.search.trace.plane.normal;next.trace.startSolid=next.search.trace.startSolid;next.trace.allSolid=next.search.trace.allSolid;next.nearest=next.search.distance;
    next.trace.fraction=finite(finite(next.ray[14])*next.search.distance);
    const float gain=finite(finite(finite(finite(next.tolerance)*2.f)+next.search.distance)*next.ray[14]);uint32_t code=4;
    if(1.f>gain){
        next.ray[12]=finite(finite(next.ray[13])*gain);
        for(unsigned j=0;j<3;++j)next.ray[3+j]=finite(finite(next.ray[j])+finite(finite(next.ray[6+j])*gain));
        next.ray[15]=0;next.tree.length=finite(next.ray[12]*0.02539999969303608f);
        const float half=finite(gain*.5f);for(unsigned j=0;j<3;++j)next.tree.midpoint[j]=finite(finite(next.localDelta[j])*half+finite(next.tree.start[j]));code=5;
    }
    next.trace.contents=contents;state=next;return code;
}
// Complete2A340 control/search/hit policy given a bound owned support context.
// Binding294F0/metadata and cache lifetime are explicit caller responsibilities.
// Codes1=filtered,2=search miss,3=hit not closer,4/5=accepted as above.
// Ordinary misses retain original search/cache changes. Exceptions roll back
// both state and cache. Filtered leaves do not inspect unused geometry/state.
inline uint32_t sourcePhysicsLeafCandidate(SourcePhysicsLeafStateCandidate&state,
    uint32_t contents,uint32_t mask,const SourcePhysicsBoxSupportCandidate&box,
    const SourcePhysicsSupportContextCandidate&shape,SourcePhysicsSupportCacheCandidate&cache,
    SourcePhysicsSearchStatisticsCandidate*statistics=nullptr){
    if(!(contents&mask)){if(statistics)*statistics={};return 1;}
    auto next=state;auto nextCache=cache;SourcePhysicsSearchStatisticsCandidate stats;
    const bool hit=sourcePhysicsSearchCandidate(next.search,next.ray,box,shape,next.tolerance,nextCache,&stats);
    const uint32_t code=hit?sourcePhysicsLeafHitCandidate(next,contents):2;state=next;cache=nextCache;if(statistics)*statistics=stats;return code;
}

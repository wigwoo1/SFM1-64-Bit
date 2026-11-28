#pragma once
#include "source_engine_collideable_clip_dispatch_candidate.h"
#include "source_phy_hull_trace_candidate.h"
#include "source_studio_physics_contents_candidate.h"
enum class SourceEngineStudioHullCacheCandidate {Available,KnownMissing,KnownEmpty};
struct SourceEngineStudioHullClipParametersCandidate {
    std::array<float,3> origin{},angles{};
    uint32_t mask=0,solidFlags=0,referenceHandle=0;
    uint16_t surfaceProperty=0;
    uint64_t worldEntityId=0;
    SourceEngineStudioHullCacheCandidate cache=SourceEngineStudioHullCacheCandidate::Available;
    // Required evidence from the host if FSOLID_ROOT_PARENT_ALIGNED is set.
    bool identityRootEstablished=false;
};
struct SourceEngineStudioHullClipResultCandidate {
    SourcePhysicsSearchTraceCandidate trace;
    bool physicsDispatched=false;
};
// Static studio solid6 composition. Geometry/contents are computed from actual
// supported PHY/MDL bytes. Supplied surface/world identities are semantic IDs.
// Null shape means an EXPLICIT known missing/empty cache, never an unresolved
// asset. Transformed boxes, hitboxes, custom collision and other solids remain
// separate scopes. The observed TraceBox ignores opaque Ray_t transform tokens.
template<class ContentsObserver>
SourceEngineStudioHullClipResultCandidate sourceEngineStudioHullClipCandidate(
    const SourcePhyHullTraceCandidate* shape,const SourceStudioPhysicsContentsCandidate& studio,
    const SourceServerHullRayCandidate& ray,const SourceEngineStudioHullClipParametersCandidate& p,
    ContentsObserver&& observeContents){
    const uint32_t customBit=ray.isRay?1u:2u;
    if((p.solidFlags&customBit)||(p.mask&0x40000000u))throw std::runtime_error("custom/hitbox studio hull clip unsupported");
    const uint32_t allowedFlags=4u|128u|256u|(ray.isRay?2u:1u);
    if(p.solidFlags&~allowedFlags)throw std::runtime_error("studio hull clip flag outside verified domain");
    if((p.solidFlags&256u)&&!p.identityRootEstablished)throw std::runtime_error("studio hull clip root identity unverified");
    if((p.referenceHandle&0xffff0000u)!=0x40000000u)throw std::runtime_error("studio hull clip requires static prop handle");
    if(p.cache!=SourceEngineStudioHullCacheCandidate::Available&&p.cache!=SourceEngineStudioHullCacheCandidate::KnownMissing&&p.cache!=SourceEngineStudioHullCacheCandidate::KnownEmpty)throw std::runtime_error("studio hull cache state unsupported");
    if((p.cache==SourceEngineStudioHullCacheCandidate::Available)!=bool(shape))throw std::runtime_error("studio hull shape/cache binding inconsistent");
    SourceEngineStudioHullClipResultCandidate result;
    struct Host {
        const SourcePhyHullTraceCandidate* shape;
        const SourceStudioPhysicsContentsCandidate& studioData;
        const SourceEngineStudioHullClipParametersCandidate& parameters;
        ContentsObserver& observer;
        bool& dispatched;
        SourceEngineCollideableClipModelCandidate model(const SourceEngineCollideableClipRayCandidate&){return {true,3};}
        SourceEngineCollideableClipStudioCandidate studio(const SourceEngineCollideableClipRayCandidate&){return {studioData.modelContents(),parameters.surfaceProperty};}
        uint32_t flags(const SourceEngineCollideableClipRayCandidate&){return parameters.solidFlags;}
        uint64_t rootTransform(const SourceEngineCollideableClipRayCandidate&){return 0;}
        uint32_t solidType(const SourceEngineCollideableClipRayCandidate&){return 6;}
        bool physics(const SourceEngineCollideableClipRayCandidate& value,uint32_t mask,SourcePhysicsSearchTraceCandidate& trace){
            if(!shape)return false;
            trace=shape->traceHullWithContents(value.ray,parameters.origin,parameters.angles,mask,[&](uint32_t gameData){const uint32_t contents=studioData.forConvex(gameData);observer(gameData,contents);return contents;});
            dispatched=true;return true;
        }
        bool custom(const SourceEngineCollideableClipRayCandidate&,uint32_t,SourcePhysicsSearchTraceCandidate&){throw std::runtime_error("unsupported studio custom dispatch");}
        bool brush(const SourceEngineCollideableClipRayCandidate&,uint32_t,SourcePhysicsSearchTraceCandidate&){throw std::runtime_error("unsupported studio brush dispatch");}
        bool hitbox(const SourceEngineCollideableClipRayCandidate&,uint32_t,SourcePhysicsSearchTraceCandidate&){throw std::runtime_error("unsupported studio hitbox dispatch");}
        // Original wrappers require solid3/solid2 respectively. With solid6,
        // both return false without modifying the initialized trace.
        bool obb(const SourceEngineCollideableClipRayCandidate&,uint32_t,SourcePhysicsSearchTraceCandidate&){return false;}
        bool bounds(const SourceEngineCollideableClipRayCandidate&,uint32_t,SourcePhysicsSearchTraceCandidate&){return false;}
        void annotate(const SourceEngineCollideableClipRayCandidate&,SourcePhysicsSearchTraceCandidate& trace){trace.entityId=parameters.worldEntityId;trace.hitBox=(parameters.referenceHandle&0xffffu)+1;}
    }host{shape,studio,p,observeContents,result.physicsDispatched};
    result.trace=sourceEngineCollideableClipDispatchCandidate({ray,0},p.mask,host);
    return result;
}
inline SourceEngineStudioHullClipResultCandidate sourceEngineStudioHullClipCandidate(
    const SourcePhyHullTraceCandidate* shape,const SourceStudioPhysicsContentsCandidate& studio,
    const SourceServerHullRayCandidate& ray,const SourceEngineStudioHullClipParametersCandidate& p){
    return sourceEngineStudioHullClipCandidate(shape,studio,ray,p,[](uint32_t,uint32_t){});
}

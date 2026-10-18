#pragma once
#include "source_phy_point_trace_candidate.h"
#include "source_studio_physics_contents_candidate.h"
#include "source_world_light_visibility_candidate.h"
// Named engine surface token.0 remains **empty**. This reserved semantic ID
// is not the address of original **studio** and is never dereferenced.
inline constexpr uint64_t sourceEngineStudioSurfaceIdCandidate=UINT64_MAX;
struct SourceEngineStudioPointClipParametersCandidate {
    std::array<float,3>origin{},angles{};
    uint32_t mask=0,solidFlags=0,referenceHandle=0;
    uint16_t surfaceProperty=0;
    uint64_t worldEntityId=0;
};
struct SourceEngineStudioPointClipResultCandidate {
    SourcePhysicsSearchTraceCandidate trace;
    bool physicsDispatched=false;
};
// Original1016F300 ->1016E170 ->physics ->1016D7B0 for point/studio/solid6
// static props, no custom collision or hitbox mask. Caller supplies resolved
// surface-property and world-entity IDs, plus KNOWN cache absence as nullptr.
// Unresolved assets must not be translated into known absent physics caches.
// Brush/hull/custom/hitbox dispatch remain separate explicit required scopes.
inline SourceEngineStudioPointClipResultCandidate sourceEngineStudioPointClipCandidate(
    const SourcePhyPointTraceCandidate*shape,const SourceStudioPhysicsContentsCandidate&studio,
    const SourceWorldLightRayCandidate&ray,const SourceEngineStudioPointClipParametersCandidate&p,
    SourcePhysicsSupportCacheCandidate*scratch=nullptr){
    if((p.solidFlags&1)||(p.mask&0x40000000u))throw std::runtime_error("custom/hitbox studio clip not implemented");
    if((p.referenceHandle&0xffff0000u)!=0x40000000u)throw std::runtime_error("studio point clip requires static-prop handle");
    auto finite=[](float x){if(!std::isfinite(x))throw std::runtime_error("nonfinite studio clip arithmetic");return x;};
    for(const auto&v:{ray.start,ray.delta})for(float x:v)finite(x);SourceEngineStudioPointClipResultCandidate result;
    for(unsigned j=0;j<3;++j){result.trace.start[j]=finite(ray.start[j]+0.f);result.trace.end[j]=finite(ray.delta[j]+result.trace.start[j]);}
    if(!(studio.modelContents()&p.mask)||!shape)return result;
    result.trace=shape->tracePointWithContents(ray.start,ray.delta,p.origin,p.angles,p.mask,[&](uint32_t data){return studio.forConvex(data);},scratch);result.physicsDispatched=true;
    if(result.trace.fraction<1||result.trace.allSolid||result.trace.startSolid){result.trace.contents=studio.modelContents();result.trace.surfaceId=sourceEngineStudioSurfaceIdCandidate;result.trace.surfaceMetadata=p.surfaceProperty;result.trace.entityId=p.worldEntityId;result.trace.hitBox=(p.referenceHandle&0xffffu)+1;}
    return result;
}

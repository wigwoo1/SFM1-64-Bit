#pragma once
#include "source_server_hull_ray_candidate.h"
#include "source_physics_search_candidate.h"

// Pointer-free orchestration only. The opaque transform identifier is passed
// to supplied collision services; this helper never dereferences it. Geometry,
// asset resolution and annotation are separate, explicitly required services.
struct SourceEngineCollideableClipRayCandidate {
    SourceServerHullRayCandidate ray;
    uint64_t transformId=0;
};
struct SourceEngineCollideableClipModelCandidate {
    bool present=false;
    uint32_t type=0;
};
struct SourceEngineCollideableClipStudioCandidate {
    uint32_t contents=0;
    uint16_t surfaceProperty=0;
};
inline constexpr uint64_t sourceEngineCollideableStudioSurfaceIdCandidate=UINT64_MAX;

// Complete original engine1016F300 dispatch policy, finite axis-aligned ray
// fields and supplied leaf services. False leaf returns may still alter trace.
// Custom collision return values are intentionally ignored by the caller.
template<class Services>
SourcePhysicsSearchTraceCandidate sourceEngineCollideableClipDispatchCandidate(
    const SourceEngineCollideableClipRayCandidate& input,uint32_t mask,Services& services){
    auto finite=[](float value){if(!std::isfinite(value))throw std::runtime_error("nonfinite collideable clip ray");return value;};
    for(const auto& values:{input.ray.center,input.ray.delta,input.ray.offset,input.ray.extents})for(float value:values)finite(value);
    SourcePhysicsSearchTraceCandidate trace;
    for(unsigned axis=0;axis<3;++axis){trace.start[axis]=finite(input.ray.offset[axis]+input.ray.center[axis]);trace.end[axis]=finite(input.ray.delta[axis]+trace.start[axis]);}
    auto ray=input;
    const auto model=services.model(ray);
    const bool studioModel=model.present&&model.type==3;
    SourceEngineCollideableClipStudioCandidate studio;
    if(studioModel){studio=services.studio(ray);if(!(mask&0x40000000u)&&!(studio.contents&mask))return trace;}
    if(services.flags(ray)&0x100u)ray.transformId=services.rootTransform(ray);
    const auto solid=services.solidType(ray);
    const bool custom=solid==5||bool(services.flags(ray)&(ray.ray.isRay?1u:2u));
    bool handled=false;
    if(custom){(void)services.custom(ray,mask,trace);handled=true;}
    else {
        handled=services.physics(ray,mask,trace);
        if(!handled&&model.present&&model.type==1)handled=services.brush(ray,mask,trace);
        if(!handled)handled=services.obb(ray,mask,trace);
    }
    bool handledHitboxes=false;
    if(studioModel&&(mask&0x40000000u)&&!custom){handledHitboxes=services.hitbox(ray,mask,trace);handled=handledHitboxes;}
    if(!handled)(void)services.bounds(ray,mask,trace);
    const auto hit=[&](){return trace.fraction<1||trace.allSolid||trace.startSolid;};
    if(studioModel&&!handledHitboxes&&hit()&&(!custom||uint16_t(trace.surfaceMetadata)==0)){
        trace.contents=studio.contents;trace.surfaceId=sourceEngineCollideableStudioSurfaceIdCandidate;trace.surfaceMetadata=studio.surfaceProperty;
    }
    if(!trace.entityId&&hit())services.annotate(ray,trace);
    return trace;
}

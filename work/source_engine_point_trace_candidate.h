#pragma once
#include "source_world_light_entity_filter_candidate.h"
#include "source_physics_search_candidate.h"
// Complete original1016EBD0 semantic merge. Metadata/64bit IDs follow the
// selected hit. Existing start-solid state retains its farther exit/start.
inline bool sourceEngineMergeTraceCandidate(const SourcePhysicsSearchTraceCandidate&candidate,
    SourcePhysicsSearchTraceCandidate&current){
    if(!candidate.allSolid&&!candidate.startSolid&&!(current.fraction>candidate.fraction))return false;
    const bool inside=current.startSolid!=0;const auto start=current.start;const float exit=current.fractionLeftSolid;current=candidate;
    if(inside){current.startSolid=1;if(exit>candidate.fractionLeftSolid){current.fractionLeftSolid=exit;current.start=start;}}
    return true;
}
namespace source_engine_point_trace_detail {
// Original point-Ray policy for the lighting and particle filters.
// Services provide world trace/annotation, spatial enumeration, collideable
// resolution, original-handle lookup and actual clipping. Enumeration order
// must be original partition order.
template<class Services,class EntityFilter>
SourcePhysicsSearchTraceCandidate trace(
    const SourceWorldLightRayCandidate&ray,uint32_t mask,
    bool worldOnly,bool filterStaticProps,Services&services,EntityFilter&&shouldHit){
    auto finite=[](float x){if(!std::isfinite(x))throw std::runtime_error("nonfinite engine point trace arithmetic");return x;};
    for(const auto&v:{ray.start,ray.delta})for(float x:v)finite(x);
    auto result=services.traceWorld(ray,mask);finite(result.fraction);
    if(result.startSolid||worldOnly)return result;
    const float scale=result.fraction;float exitScale=scale;auto clipped=ray;
    if(scale==0){clipped.delta={0,0,0};exitScale=finite(result.fractionLeftSolid);result.fractionLeftSolid=1;}
    else{
        for(unsigned j=0;j<3;++j){const float end=finite(finite(ray.delta[j]*scale)+ray.start[j]);clipped.delta[j]=finite(end-ray.start[j]);}
        result.fractionLeftSolid=finite(finite(result.fractionLeftSolid)/scale);
    }
    result.fraction=1;const auto candidates=services.enumerateCandidates(clipped);size_t count=candidates.size();
    if(count>1024){services.candidateOverflow(count-1024);count=1024;}
    for(size_t i=0;i<count;++i){
        const auto entity=candidates[i];const auto collideable=services.resolveCollideable(entity);
        const bool staticProp=sourceStaticPropEntityCandidate(entity,services);
        if((!staticProp||filterStaticProps)&&!shouldHit(entity,mask))continue;
        const auto hit=services.clipToCollideable(clipped,mask,collideable);finite(hit.fraction);
        sourceEngineMergeTraceCandidate(hit,result);if(result.allSolid)break;
    }
    result.fraction=finite(scale*result.fraction);result.fractionLeftSolid=finite(exitScale*result.fractionLeftSolid);return result;
}
}

template<class Services>
SourcePhysicsSearchTraceCandidate sourceEnginePointTraceCandidate(
    const SourceWorldLightRayCandidate&ray,uint32_t mask,
    const SourceWorldLightFilterCandidate&filter,Services&services){
    if(filter.mode!=SourceWorldLightTraceModeCandidate::WorldOnly&&
       filter.mode!=SourceWorldLightTraceModeCandidate::EverythingFilterProps)
        throw std::runtime_error("unsupported engine point trace filter");
    return source_engine_point_trace_detail::trace(ray,mask,
        filter.mode==SourceWorldLightTraceModeCandidate::WorldOnly,true,services,
        [&](uintptr_t entity,uint32_t contentsMask){
            return sourceWorldLightShouldHitEntityCandidate(filter,entity,contentsMask,services);
        });
}

// Particle TraceEverything admits static props directly. Its client
// predicate applies only to other entities.
template<class Services,class EntityFilter>
SourcePhysicsSearchTraceCandidate sourceEngineParticlePointTraceCandidate(
    const SourceWorldLightRayCandidate&ray,uint32_t mask,Services&services,EntityFilter&&shouldHit){
    return source_engine_point_trace_detail::trace(ray,mask,false,false,services,shouldHit);
}

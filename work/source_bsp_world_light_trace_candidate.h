#pragma once
#include "source_bsp_trace_surface_flags_candidate.h"
#include "source_world_light_visibility_candidate.h"
#include "source_world_ray_trace_candidate.h"
// Borrows one prepared BSP collision world and its matching surface binding.
// The caller owns both for this adapter's lifetime. Static-prop/entity collision
// is not provided: reject that filter instead of silently doing a world trace.
class SourceBspWorldLightTraceCandidate {
    const SourcePreparedWorldRayCandidate&world;
    const SourceBspTraceSurfaceFlagsCandidate&surfaces;
public:
    SourceBspWorldLightTraceCandidate(const SourcePreparedWorldRayCandidate&w,const SourceBspTraceSurfaceFlagsCandidate&s):world(w),surfaces(s){}
    SourceWorldLightTraceCandidate trace(const SourceWorldLightRayCandidate&ray,uint32_t mask,const SourceWorldLightFilterCandidate&filter)const{
        if(filter.mode!=SourceWorldLightTraceModeCandidate::WorldOnly)throw std::runtime_error("static-prop lighting trace is not implemented");
        for(float x:ray.start)if(!std::isfinite(x))throw std::runtime_error("nonfinite BSP lighting ray start");
        for(float x:ray.delta)if(!std::isfinite(x))throw std::runtime_error("nonfinite BSP lighting ray delta");
        const bool swept=sourceWorldLightSquaredCandidate(ray.delta)!=0;
        if(ray.swept!=swept)throw std::runtime_error("BSP lighting Ray_t swept flag mismatch");
        const auto result=sourceWorldRayTraceWithSweptFlagCandidate(world,ray.start,ray.delta,mask,swept);
        return {result.state.trace.fraction,surfaces.flags(result.state.trace.surface),result.endPosition};
    }
};

#pragma once
#include "source_physics_ray_preparation_candidate.h"
#include "source_physics_refinement_candidate.h"
#include "source_server_hull_ray_candidate.h"

struct SourcePhysicsHullRayPreparationCandidate {
    SourcePhysicsRayPreparationCandidate prepared;
    SourcePhysicsBoxSupportCandidate box;
};
// Original full TraceBox2B260 moving-box preparation plus existing27310,
// 295F0 and27400 preparation. The supplied isRay bit selects point behavior;
// extents do not infer it. Original box radius remains in Source units.
// This computes preparation only, not PHY intersections or trace results.
inline SourcePhysicsHullRayPreparationCandidate sourcePhysicsHullRayPreparationCandidate(
    const SourceServerHullRayCandidate& ray,const std::array<float,3>& origin,
    const std::array<float,3>& angles){
    for(const auto& values:{ray.offset,ray.extents})for(float value:values)if(!std::isfinite(value))throw std::runtime_error("nonfinite physics hull input");
    for(float extent:ray.extents)if(extent<0)throw std::runtime_error("negative physics hull extent");
    SourcePhysicsHullRayPreparationCandidate result;
    result.prepared=sourcePhysicsRayPreparationCandidate(ray.center,ray.delta,origin,angles);
    result.box.point=ray.isRay;
    if(!ray.isRay){
        for(unsigned axis=0;axis<3;++axis)result.box.axes[axis]={ray.extents[axis],-ray.extents[axis]};
        const float xx=ray.extents[0]*ray.extents[0],yy=ray.extents[1]*ray.extents[1],zz=ray.extents[2]*ray.extents[2];
        const float squared=(xx+yy)+zz;
        result.prepared.radius=std::sqrt(squared);
        if(!std::isfinite(result.prepared.radius))throw std::runtime_error("physics hull radius overflow");
    }
    return result;
}

#pragma once
#include "source_displacement_runtime_mesh_candidate.h"
#include "source_displacement_ray_tree_candidate.h"

// Semantic state for the outer world adapter 1014D010. Material properties
// are supplied by the caller; this does not reconstruct their BSP binding.
struct SourceDisplacementWorldRayStateCandidate {
 SourceDisplacementRayTraceCandidate ray;
 uint32_t contents=0,displacementHit=0;
 bool surfaceIsDisplacement=false;
 uint16_t surfaceProperty=0,surfaceFlags=0;
};

inline void sourceDisplacementWorldRayCandidate(
 const SourceDisplacementRuntimeMeshCandidate&mesh,
 const std::array<uint16_t,2>&surfaceProperties,
 const std::array<float,3>&start,const std::array<float,3>&delta,
 const std::array<float,3>&inverse,SourceDisplacementWorldRayStateCandidate&state){
 auto staged=state;
 // Native builds a full ray from work start/delta, always one-sided.
 // The two leaf subinterval parameters of 1014D010 are unused.
 if(!sourceDisplacementRayTraceCandidate(mesh.vertices,mesh.triangles,mesh.tree,
    mesh.runtimeFlags,mesh.contents,start,delta,inverse,true,staged.ray))return;
 staged.contents=mesh.contents;staged.displacementHit=1;
 staged.surfaceIsDisplacement=true;staged.surfaceFlags=0;
 staged.surfaceProperty=surfaceProperties[(staged.ray.displacementFlags&16u)?1:0];
 state=staged;
}

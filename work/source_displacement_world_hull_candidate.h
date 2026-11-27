#pragma once
#include "source_displacement_runtime_mesh_candidate.h"
#include "source_displacement_hull_sweep_candidate.h"

// Semantic output of outer world hull adapter1014d100. Surface properties are
// resolved by the caller, not inferred from geometry or native pointer values.
struct SourceDisplacementWorldHullStateCandidate {
 SourceDisplacementHullTraceCandidate hull;
 uint32_t contents=0,displacementHit=0;
 bool surfaceIsDisplacement=false;
 uint16_t surfaceProperty=0,surfaceFlags=0;
};

inline void sourceDisplacementWorldHullCandidate(
 const SourceDisplacementRuntimeMeshCandidate& mesh,
 const SourceDisplacementHullCacheCandidate& cache,
 const std::array<uint16_t,2>& surfaceProperties,
 const std::array<float,3>& start,const std::array<float,3>& delta,
 const std::array<float,3>& inverse,const std::array<float,3>& extents,
 SourceDisplacementWorldHullStateCandidate& state) {
 auto staged=state;
 const bool closer=sourceDisplacementHullSweepCandidate(mesh.tree,mesh.vertices,mesh.triangles,cache,
  mesh.runtimeFlags,start,delta,inverse,extents,staged.hull);
 if(closer){
  staged.contents=mesh.contents;staged.displacementHit=1;
  staged.surfaceIsDisplacement=true;staged.surfaceFlags=0;
  staged.surfaceProperty=surfaceProperties[(staged.hull.displacementFlags&16u)?1:0];
 }
 // Inner zero-fraction contact metadata must survive even without a closer hit.
 state=staged;
}

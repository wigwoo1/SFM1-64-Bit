#pragma once
#include "source_displacement_hull_cache_candidate.h"
#include "source_displacement_hull_tree_candidate.h"
#include "source_displacement_hull_triangle_candidate.h"

// Prepared-mesh non-ray sweep. Cache construction/lifetime and world metadata
// are separate; no original engine resource or global manager is required here.
inline bool sourceDisplacementHullSweepCandidate(
 const SourceDisplacementCollisionTreeCandidate& tree,
 const std::vector<std::array<float,3>>& vertices,
 const std::vector<SourceDisplacementCollisionTriangleCandidate>& triangles,
 const SourceDisplacementHullCacheCandidate& cache,uint32_t runtimeFlags,
 const std::array<float,3>& start,const std::array<float,3>& delta,
 const std::array<float,3>& inverse,const std::array<float,3>& extents,
 SourceDisplacementHullTraceCandidate& trace) {
 if(runtimeFlags&4u)return false;
 if(triangles.size()!=2*tree.leafPairs.size()||cache.indices.size()!=triangles.size())throw std::runtime_error("displacement sweep storage coverage");
 auto staged=trace;
 struct Host {
  const std::vector<std::array<float,3>>& vertices;
  const std::vector<SourceDisplacementCollisionTriangleCandidate>& triangles;
  const SourceDisplacementHullCacheCandidate& cache;
  const std::array<float,3>& start;const std::array<float,3>& delta;const std::array<float,3>& extents;
  SourceDisplacementHullTraceCandidate& trace;
  void beginTriangles(){} void endTriangles(){}
  void traceTriangle(uint32_t id,const std::array<float,3>& normalized,float& fraction){
   trace.fraction=fraction;
   sourceDisplacementHullTriangleCandidate(vertices,triangles.at(id),cache.planes,cache.indices.at(id),start,delta,extents,normalized,trace);
   fraction=trace.fraction;
  }
 } host{vertices,triangles,cache,start,delta,extents,staged};
 float fraction=staged.fraction;
 const bool result=sourceDisplacementHullTreeCandidate(tree,runtimeFlags,start,delta,inverse,extents,fraction,host);
 // Native contacts can update metadata at fraction zero without a lower final
 // fraction. Commit the trace even when the return value is false.
 staged.fraction=fraction;trace=staged;return result;
}

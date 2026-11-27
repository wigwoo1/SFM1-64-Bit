#pragma once
#include "source_prepared_world_hull_candidate.h"
#include "source_displacement_leaf_assignment_candidate.h"

// Owned derived tables for borrowed immutable world/mesh geometry. Explicit
// permutation input avoids any implicit dependency on original engine data.
struct SourcePreparedHullWorldTablesCandidate {
 std::vector<SourceDisplacementHullCacheCandidate> caches;
 std::vector<SourceLeafDisplacementCullCandidate> culls;
 std::vector<std::vector<uint32_t>> leafDisplacements;
};
inline SourcePreparedHullWorldTablesCandidate sourcePreparedHullWorldTablesCandidate(
 const SourceBspBrushOnlyTraceCandidate& world,
 const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,
 const SourcePlaneHashPermutationCandidate& permutation,size_t buckets,bool masked) {
 if(meshes.size()>65535)throw std::runtime_error("prepared hull displacement count outside reference domain");
 SourcePreparedHullWorldTablesCandidate result;result.leafDisplacements.resize(world.data.leafBrushes.size());
 result.caches.reserve(meshes.size());result.culls.reserve(meshes.size());
 for(size_t i=0;i<meshes.size();++i){const auto& mesh=meshes[i];const auto& bounds=mesh.tree.expandedBounds;
  SourceLeafDisplacementCullCandidate cull{{bounds[0],bounds[1],bounds[2]},{bounds[3],bounds[4],bounds[5]},mesh.contents,uint32_t(i)};
  result.caches.push_back(sourceDisplacementHullCacheCandidate(mesh.power,mesh.vertices,mesh.triangles,permutation,buckets,masked));
  for(auto leaf:sourceDisplacementLeafAssignmentCandidate(world,cull.mins,cull.maxs))result.leafDisplacements.at(leaf).push_back(uint32_t(i));
  result.culls.push_back(cull);
 }
 return result;
}

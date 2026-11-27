#pragma once
#include "source_displacement_edge_planes_candidate.h"
#include "source_displacement_plane_pool_candidate.h"

struct SourceDisplacementHullCacheCandidate {
 std::vector<std::array<uint16_t,9>> indices;
 std::vector<std::array<float,3>> planes;
};
// Geometry cache build, not global engine cache allocation/lifetime management.
// Explicit table/configuration prevents hidden reference-DLL dependencies.
inline SourceDisplacementHullCacheCandidate sourceDisplacementHullCacheCandidate(
 uint32_t power,const std::vector<std::array<float,3>>& vertices,
 const std::vector<SourceDisplacementCollisionTriangleCandidate>& triangles,
 const SourcePlaneHashPermutationCandidate& permutation,size_t buckets,bool masked) {
 if(power<1||power>4)throw std::runtime_error("invalid displacement cache power");
 const size_t cells=size_t(1)<<power;if(vertices.size()!=(cells+1)*(cells+1)||triangles.size()!=2*cells*cells)throw std::runtime_error("displacement cache grid coverage");
 SourceDisplacementPlanePoolCandidate pool(buckets,masked,permutation);SourceDisplacementHullCacheCandidate result;
 result.indices.reserve(triangles.size());for(const auto& t:triangles)result.indices.push_back(sourceDisplacementEdgePlanesCandidate(vertices,t,pool));
 result.planes=std::move(pool.planes);return result;
}

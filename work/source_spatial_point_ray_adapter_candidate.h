#pragma once
#include "source_spatial_point_partition_candidate.h"
#include "source_world_light_visibility_candidate.h"
// Engine point tracing keeps an explicit swept flag when shortening rays.
// Keep this boundary separate from the convenience fresh-delta query API.
inline std::vector<uint64_t>sourceSpatialPointEnumerateRayCandidate(
    const SourceSpatialPointPartitionCandidate&partition,
    const SourceWorldLightRayCandidate&ray,uint32_t partitionMask,size_t stopAfter=0){
    return partition.queryPointRay(ray.start,ray.delta,ray.swept,partitionMask,stopAfter);
}

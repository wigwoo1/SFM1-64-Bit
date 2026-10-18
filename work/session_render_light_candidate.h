#pragma once
#include "graph_transform_hierarchy.h"
#include "projected_light_saved_fields_candidate.h"
// Owned evaluated-graph snapshot. Renderer-specific jitter, gobo resources,
// shadow allocation and GPU constants are intentionally separate stages.
struct SessionRenderLightCandidate {
 uint32_t id;
 ProjectedLightSavedFieldsCandidate fields;
 RecoveredMatrix3x4 world;
};
inline SessionRenderLightCandidate readSessionRenderLightCandidate(const Graph&graph,uint32_t id,GraphTransformHierarchy&hierarchy){
 auto fields=projectedLightSavedFieldsCandidate(graph,id);
 auto world=hierarchy.world(id);
 for(const auto&row:world)for(float value:row)if(!std::isfinite(value))throw std::runtime_error("nonfinite light world transform");
 return {id,std::move(fields),world};
}

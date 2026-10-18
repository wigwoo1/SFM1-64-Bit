#pragma once
#include "graph_transform_hierarchy.h"
#include "render_camera_candidate.h"
struct SessionModelPlacementCandidate {std::string asset;RenderCameraCandidate matrix;};
inline SessionModelPlacementCandidate readSessionModelPlacementCandidate(const Graph&graph,uint32_t id,GraphTransformHierarchy&hierarchy){const auto&element=graph.elements.at(id);if(graph.strings.at(element.type)!="DmeGameModel")throw std::runtime_error("game model required");auto name=namedAttribute(graph,element,"modelName");if(!name||name->type!=5||name->values.size()!=1||name->values[0].text.empty())throw std::runtime_error("model asset schema");auto world=hierarchy.world(id);RenderCameraCandidate matrix{};for(size_t r=0;r<3;++r)for(size_t c=0;c<4;++c){if(!std::isfinite(world[r][c]))throw std::runtime_error("nonfinite model placement");matrix.rows[r*4+c]=world[r][c];}matrix.rows[15]=1;return {name->values[0].text,matrix};}

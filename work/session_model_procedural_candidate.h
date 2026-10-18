#pragma once
#include "clip_hierarchy.h"
inline bool sessionModelEvaluateProceduralCandidate(const Graph&graph,uint32_t model){
 const auto&element=graph.elements.at(model);if(graph.strings.at(element.type)!="DmeGameModel")throw std::runtime_error("procedural setting requires model");auto*attribute=namedAttribute(graph,element,"evaluateProceduralBones");if(!attribute)return true;
 if(attribute->type!=4||attribute->values.size()!=1||attribute->values[0].wire.size()!=1||uint8_t(attribute->values[0].wire[0])>1)throw std::runtime_error("model procedural setting schema");return attribute->values[0].wire[0]!=0;
}

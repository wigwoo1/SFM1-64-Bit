#pragma once
#include "clip_hierarchy.h"
#include <optional>
// Root element's saved activeClip is the candidate default. No index guessing.
inline uint32_t sessionRenderRootCandidate(const Graph&graph,std::optional<uint32_t>explicitRoot={}){
 uint32_t root=0;
 if(explicitRoot)root=*explicitRoot;
 else{if(graph.elements.empty())throw std::runtime_error("session graph empty");const Attribute*active=nullptr;for(const auto&attribute:graph.elements.front().attributes)if(graph.strings.at(attribute.name)=="activeClip"){if(active)throw std::runtime_error("duplicate session activeClip");active=&attribute;}if(!active||active->type!=1||active->values.size()!=1)throw std::runtime_error("session activeClip schema");root=active->values.front().reference;}
 if(root>=graph.elements.size()||graph.strings.at(graph.elements[root].type)!="DmeFilmClip")throw std::runtime_error("session render root must reference film clip");return root;
}

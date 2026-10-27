#pragma once
#include "clip_hierarchy.h"
#include "material_session_override_dispatch_candidate.h"

// Preserve the owned Graph's explicit attribute order. Native loading/creation
// order is a separate contract (native new attributes prepend). Do not sort,
// stringify numeric/matrix values, or reinterpret a string as a vector.
inline std::vector<MaterialSessionOverrideInputCandidate>readSessionMaterialOverrideDispatchCandidate(const Graph&graph,uint32_t material){
 const auto&e=graph.elements.at(material);
 if(graph.strings.at(e.type)!="DmeMaterial"||e.attributes.size()>4096)throw std::runtime_error("typed material override element/extent");
 std::vector<MaterialSessionOverrideInputCandidate>result;result.reserve(e.attributes.size());
 for(const auto&a:e.attributes){
  MaterialSessionOverrideInputCandidate row;row.name=graph.strings.at(a.name);row.typeFlags=a.type;
  if(!row.name.empty()&&row.name.front()=='$'){
   size_t bytes=0;switch(a.type){case 2:case 3:case 8:bytes=4;break;case 4:bytes=1;break;case 9:bytes=8;break;case 10:bytes=12;break;case 11:bytes=16;break;case 14:bytes=64;break;case 5:if(a.values.size()!=1)throw std::runtime_error("typed material string shape");row.textureName=a.values.front().text;break;default:break;}
   if(bytes){if(a.values.size()!=1||a.values.front().wire.size()!=bytes)throw std::runtime_error("typed material numeric shape");std::memcpy(row.words.data(),a.values.front().wire.data(),bytes);}
  }
  result.push_back(std::move(row));
 }
 return result;
}

#pragma once
#include "clip_hierarchy.h"
#include <optional>
// Original1028BEE0 path, given an explicitly resolved CMovieDoc owner element.
// Only typed float values are supported; native cross-type coercion is unresolved.
inline std::optional<float> documentFrameRateLookupCandidate(const Graph&g,uint32_t owner){
 if(owner==0xffffffffu)return std::nullopt;
 const Element*element=&g.elements.at(owner);
 for(const char*name:{"settings","renderSettings"}){
  const auto ref=hierarchySingleReference(g,*element,name);
  if(ref==0xffffffffu)return std::nullopt;
  element=&g.elements.at(ref);
 }
 const auto*rate=namedAttribute(g,*element,"frameRate");
 if(!rate)return std::nullopt;
 if(rate->type!=3||rate->values.size()!=1||rate->values[0].wire.size()!=4)throw std::runtime_error("unsupported document frame rate attribute");
 return wireFloat(rate->values[0]);
}

#pragma once
#include "session_expression_chain_plan.h"
// Candidate value-equivalence proof only; native callback ordering is unresolved.
inline bool equivalentConstantChannelWritersCandidate(const Graph&g,const std::vector<uint32_t>&writers){
 if(writers.size()<2)return false;uint32_t bits=0;ChannelEndpoint destination;bool first=true;
 for(auto id:writers){if(g.strings.at(g.elements.at(id).type)!="DmeChannel")return false;ChannelBinding b(g,id);
  if(b.mode!=3||!b.destination.present()||b.destination.scalarType!=3||b.log==0xffffffff||b.layers.size()!=1)return false;
  if(!hierarchyRefs(g,g.elements.at(b.log),"curveinfo",1).empty())return false;
  KeyTrack track(g,g.elements.at(b.layers[0]));if(!track.times->values.empty())return false;
  FloatLogDefault defaults(g,g.elements.at(b.log));if(!defaults.enabled)return false;
  uint32_t current;std::memcpy(&current,&defaults.value,4);if((current&0x7f800000u)==0x7f800000u)return false;
  if(first){destination=b.destination;bits=current;first=false;}else if(!sameRuntimeEndpoint(destination,b.destination)||bits!=current)return false;
 }return true;
}

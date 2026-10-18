#pragma once
#include "session_expression_runtime.h"
#include "graph_quaternion_key_sampler.h"
#include "graph_quaternion_curve_sampler.h"
inline bool runtimeWriteQuaternion(Graph&g,const ChannelEndpoint&endpoint,const std::array<float,4>&value,SessionAttributeChanges*changes=nullptr){
 auto*a=runtimeAttribute(g,endpoint.element,endpoint.attribute);if(!a)throw std::runtime_error("missing quaternion destination");
 auto&wire=a->values.at(endpoint.index).wire;if(wire.size()!=16)throw std::runtime_error("invalid quaternion destination width");
 bool changed=false;for(unsigned c=0;c<4;++c){float previous;std::memcpy(&previous,wire.data()+c*4,4);if(floatMutationNeeded(previous,value[c]))changed=true;}
 if(!changed)return false;
 for(unsigned c=0;c<4;++c)setFloat(*a,endpoint.index,value[c],c);
 if(changes)changes->attributes.emplace_back(endpoint.element,endpoint.attribute);return true;
}
// Nonempty uncompressed playback; owner-default and curve-info paths await recovery.
inline bool executeSessionQuaternionPlay(Graph&g,const ChannelBinding&binding,int32_t tick,SessionAttributeChanges*changes=nullptr,const std::vector<std::pair<bool,bool>>*infinite=nullptr){
 if(binding.mode!=3||!binding.destination.present()||binding.destination.scalarType!=13||binding.log==0xffffffff||binding.layers.empty())return false;
 if(binding.layers.size()>1&&!infinite)return false;
 const auto&log=g.elements.at(binding.log);if(g.strings.at(log.type)!="DmeQuaternionLog")return false;const bool curved=!hierarchyRefs(g,log,"curveinfo",1).empty();
 int32_t selected=infinite?selectLogLayer(sessionLogBounds(g,binding.layers,*infinite),tick):0;
 if(selected<0)return false;
 auto id=binding.layers.at(selected);KeyTrack track(g,g.elements.at(id));if(track.times->values.empty()&&!curved)return false;

 if(curved){auto sampler=bindGraphQuaternionCurveSampler(g,binding.log,id);runtimeWriteQuaternion(g,binding.destination,sampler.sample(tick),changes);return true;}
 auto sampler=bindGraphQuaternionKeySampler(g,binding.log,id);runtimeWriteQuaternion(g,binding.destination,sampler.sample(tick),changes);return true;
}

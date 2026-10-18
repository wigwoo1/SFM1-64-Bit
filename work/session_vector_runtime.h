#pragma once
#include "session_expression_runtime.h"
#include "graph_vector_key_sampler.h"
#include "graph_vector_curve_sampler.h"
#include "float_component_write_sequence.h"
inline bool runtimeWriteVector(Graph&g,const ChannelEndpoint&endpoint,const std::array<float,3>&value,SessionAttributeChanges*changes=nullptr){
 auto*a=runtimeAttribute(g,endpoint.element,endpoint.attribute);if(!a)throw std::runtime_error("missing vector destination");
 auto&wire=a->values.at(endpoint.index).wire;if(wire.size()!=12)throw std::runtime_error("invalid vector destination width");
 bool changed=false;for(unsigned c=0;c<3;++c){float previous;std::memcpy(&previous,wire.data()+c*4,4);if(floatMutationNeeded(previous,value[c]))changed=true;}
 if(!changed)return false;
 for(unsigned c=0;c<3;++c){uint32_t bits;std::memcpy(&bits,&value[c],4);setWireU32(a->values.at(endpoint.index),recoveredX87FloatStoreBitsCandidate(bits),c);}
 if(changes)changes->attributes.emplace_back(endpoint.element,endpoint.attribute);return true;
}
// Nonempty uncompressed playback; empty owner-default path awaits recovery; curve path needs explicit runtime edge value.
inline bool sampleSessionVectorPlayCandidate(const Graph&g,const ChannelBinding&binding,int32_t tick,std::array<float,3>&value,const std::vector<std::pair<bool,bool>>*infinite=nullptr,const std::array<float,3>*disabledEdgeValue=nullptr){
 if(binding.mode!=3||!binding.destination.present()||binding.destination.scalarType!=10||binding.log==0xffffffff||binding.layers.empty())return false;
 if(binding.layers.size()>1&&!infinite)return false;
 const auto&log=g.elements.at(binding.log);if(g.strings.at(log.type)!="DmeVector3Log")return false;const bool curved=!hierarchyRefs(g,log,"curveinfo",1).empty();if(curved&&!disabledEdgeValue)return false;
 int32_t selected=infinite?selectLogLayer(sessionLogBounds(g,binding.layers,*infinite),tick):0;
 if(selected<0)return false;
 auto id=binding.layers.at(selected);KeyTrack track(g,g.elements.at(id));if(track.times->values.empty()&&!curved)return false;
 if(curved){auto sampler=bindGraphVectorCurveSampler(g,binding.log,id,*disabledEdgeValue);value=sampler.sample(tick);return true;}
 auto sampler=bindGraphVectorKeyViewCandidate(g,binding.log,id);value=sampler.sample(tick);return true;
}

inline bool executeSessionVectorPlay(Graph&g,const ChannelBinding&binding,int32_t tick,SessionAttributeChanges*changes=nullptr,const std::vector<std::pair<bool,bool>>*infinite=nullptr,const std::array<float,3>*disabledEdgeValue=nullptr){
 std::array<float,3>value;if(!sampleSessionVectorPlayCandidate(g,binding,tick,value,infinite,disabledEdgeValue))return false;
 runtimeWriteVector(g,binding.destination,value,changes);return true;
}

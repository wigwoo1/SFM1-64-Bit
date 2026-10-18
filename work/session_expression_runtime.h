#pragma once
#include "expression_parser.h"
#include "animation_keys.h"
#include "float_key_sampler.h"
#include "graph_float_curve_sampler.h"
#include "float_mutation.h"
#include "float_log_default.h"
#include "channel_play_path.h"
#include "session_log_empty.h"
#include "session_log_layer_bounds.h"
#include <map>
struct SessionAttributeChanges {std::vector<std::pair<uint32_t,std::string>> attributes;};
// Requires the native Graph reader/writer and ChannelBinding declarations.
inline Attribute* runtimeAttribute(Graph&graph,uint32_t element,const std::string&name){
 for(auto&attribute:graph.elements.at(element).attributes)if(graph.strings.at(attribute.name)==name)return &attribute;return nullptr;
}
inline bool runtimeWriteFloat(Graph&graph,uint32_t element,const std::string&name,size_t index,float value,SessionAttributeChanges*changes=nullptr){
 auto*attribute=runtimeAttribute(graph,element,name);if(!attribute)throw std::runtime_error("missing runtime float attribute");
 if(!floatMutationNeeded(wireFloat(attribute->values.at(index)),value))return false;
 setFloat(*attribute,index,value);if(changes)changes->attributes.emplace_back(element,name);return true;
}
struct SessionExpressionSample {float result=0;bool success=false;};
// The caller keeps the graph immutable for the duration of sampling. No writes
// or dirty notifications occur here. Inputs must be current in the graph or
// supplied explicitly through privateInputs by a validated chain sampler.
inline SessionExpressionSample sampleSessionExpression(const Graph&graph,uint32_t id,const std::map<std::string,float>&privateInputs={}){
 const auto&element=graph.elements.at(id);if(graph.strings.at(element.type)!="DmeExpressionOperator")throw std::runtime_error("expected expression operation");
 const auto*formula=namedAttribute(graph,element,"expr");const auto*output=namedAttribute(graph,element,"result");
 if(!formula||formula->type!=5||formula->values.size()!=1||!output||output->type!=3||output->values.size()!=1)throw std::runtime_error("invalid expression operation schema");
 ExpressionVariables variables;
 for(const auto&attribute:element.attributes)if(attribute.type==3&&graph.strings[attribute.name]!="result")variables.set(graph.strings[attribute.name],wireFloat(attribute.values.at(0)));
 for(const auto&input:privateInputs){
  const auto*attribute=namedAttribute(graph,element,input.first.c_str());
  if(input.first=="result"||!attribute||attribute->type!=3||attribute->values.size()!=1)throw std::runtime_error("invalid private expression input");
  variables.set(input.first,input.second);
 }
 ExpressionParser parser(formula->values[0].text,variables);SessionExpressionSample sample;sample.success=parser.evaluate(sample.result);return sample;
}
inline bool executeSessionExpression(Graph&graph,uint32_t id,SessionAttributeChanges*changes=nullptr){
 const auto sample=sampleSessionExpression(graph,id);
 // Failed evaluation still commits the parser's result, matching the existing
 // serial path. Callers decide whether that failure terminates the pass.
 runtimeWriteFloat(graph,id,"result",0,sample.result,changes);return sample.success;
}
inline bool executeSessionFloatCopy(Graph&graph,const ChannelBinding&binding,SessionAttributeChanges*changes=nullptr){
 if(binding.mode!=1)return false;
 if(!binding.source.present()||!binding.destination.present())return false;
 if(binding.source.scalarType!=3||binding.destination.scalarType!=3)return false;
 // 0x104CD122 compares attribute identity before examining array indices.
 if(binding.source.element==binding.destination.element&&binding.source.attribute==binding.destination.attribute)return true;
 auto*source=runtimeAttribute(graph,binding.source.element,binding.source.attribute);auto*destination=runtimeAttribute(graph,binding.destination.element,binding.destination.attribute);
 if(!source||!destination)throw std::runtime_error("missing runtime binding");
 const float value=wireFloat(source->values.at(binding.source.index));runtimeWriteFloat(graph,binding.destination.element,binding.destination.attribute,binding.destination.index,value,changes);return true;
}
// Tick is already log-local. Multiple layers require explicit runtime flags.
inline bool sampleSessionFloatPlayCandidate(const Graph&graph,const ChannelBinding&binding,int32_t tick,float&value,bool force=false,const std::vector<std::pair<bool,bool>>*infinite=nullptr){
 if(binding.mode!=3||!binding.destination.present()||binding.destination.scalarType!=3||binding.log==0xffffffff)return false;
 if(binding.layers.size()>1&&!infinite)return false;
 const auto&log=graph.elements.at(binding.log);if(graph.strings[log.type]!="DmeFloatLog")return false;
 const bool hasCurveInfo=!hierarchyRefs(graph,log,"curveinfo",1).empty();
 FloatLogDefault defaults(graph,log);
 const bool empty=sessionFloatLogEmpty(graph,binding.layers);auto path=channelPlayPath(true,true,force,empty,defaults.enabled,binding.source.present());
 if(path==ChannelPlayPath::CopySource){if(binding.source.scalarType!=3)return false;auto*source=namedAttribute(graph,graph.elements.at(binding.source.element),binding.source.attribute.c_str());if(!source)throw std::runtime_error("missing playback fallback source");value=wireFloat(source->values.at(binding.source.index));}
 else if(binding.layers.empty())value=0.0f;
 else {
  int32_t selected=0;
  if(infinite)selected=selectLogLayer(sessionLogBounds(graph,binding.layers,*infinite),tick);
  const auto&layer=graph.elements.at(binding.layers.at(selected));KeyTrack track(graph,layer);
  if(hasCurveInfo){auto sampler=bindGraphFloatCurveSampler(graph,binding.log,binding.layers.at(selected));value=sampler.sample(tick);}
  else if(track.times->values.empty())value=defaults.emptyLayerValue();
  else {FloatKeySampler sampler(graph,layer);value=sampler.sample(tick);}
 }
 return true;
}

inline bool executeSessionFloatPlay(Graph&graph,const ChannelBinding&binding,int32_t tick,SessionAttributeChanges*changes=nullptr,bool force=false,const std::vector<std::pair<bool,bool>>*infinite=nullptr){
 float value;if(!sampleSessionFloatPlayCandidate(graph,binding,tick,value,force,infinite))return false;
 runtimeWriteFloat(graph,binding.destination.element,binding.destination.attribute,binding.destination.index,value,changes);return true;
}

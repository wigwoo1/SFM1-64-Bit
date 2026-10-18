#pragma once
#include "runtime_channel_copy.h"
struct RuntimeFloatPlayBinding:RuntimeFloatCopyBinding{
 uint32_t logHandle=0xffffffff;std::vector<uint32_t> layerHandles;
 RuntimeFloatPlayBinding(const Graph&g,uint32_t channel,const SessionRuntimeHandles&r):RuntimeFloatCopyBinding(g,channel,r){
  if(saved.log!=0xffffffff)logHandle=r.graphHandles.at(saved.log);
  for(uint32_t layer:saved.layers)layerHandles.push_back(r.graphHandles.at(layer));
 }
};
inline bool executeRuntimeFloatPlay(Graph&graph,const SessionRuntimeHandles&runtime,const RuntimeFloatPlayBinding&binding,int32_t tick,SessionAttributeChanges*changes=nullptr,bool force=false,const std::vector<std::pair<bool,bool>>*infinite=nullptr){
 if(!runtime.resolve(graph,binding.channelHandle)||!runtime.resolve(graph,binding.destinationHandle)||!runtime.resolve(graph,binding.logHandle))return false;
 for(uint32_t layer:binding.layerHandles)if(!runtime.resolve(graph,layer))return false;
 if(!runtimeChannelSnapshotCurrent(graph,binding,true))return false;
 auto live=binding.saved;if(!runtime.resolve(graph,binding.sourceHandle))live.source.element=0xffffffff;
 return executeSessionFloatPlay(graph,live,tick,changes,force,infinite);
}
struct RuntimeCachedFloatPlayBinding:RuntimeFloatPlayBinding{
 uint32_t sourceAttribute=0xffffffff,destinationAttribute=0xffffffff;
 RuntimeCachedFloatPlayBinding(const Graph&g,uint32_t channel,const SessionRuntimeHandles&r,const SessionAttributeHandles&a):RuntimeFloatPlayBinding(g,channel,r){
  if(saved.source.present())sourceAttribute=a.find(sourceHandle,saved.source.attribute);
  if(saved.destination.present())destinationAttribute=a.find(destinationHandle,saved.destination.attribute);
 }
};
// Check channel metadata without requiring the optional source attribute to exist.
inline uint32_t cachedSingleHierarchyReference(const Graph&g,const Element&e,const char*name){
 return hierarchySingleReference(g,e,name);
}
inline bool cachedHierarchyReferencesEqual(const Graph&g,const Element&e,const char*name,const std::vector<uint32_t>&expected){
 const auto*a=namedAttribute(g,e,name);if(!a)return expected.empty();
 if(a->type!=15)throw std::runtime_error("invalid hierarchy reference field");
 size_t index=0;bool equal=true;
 for(const auto&value:a->values){auto ref=value.reference;if(ref==0xffffffff)continue;if(ref>=g.elements.size())throw std::runtime_error("unresolved hierarchy reference");if(index>=expected.size()||ref!=expected[index])equal=false;++index;}
 return equal&&index==expected.size();
}
inline bool cachedPlayMetadataCurrent(const Graph&g,const RuntimeCachedFloatPlayBinding&b){
 const auto&e=g.elements.at(b.channelIndex);auto mode=namedAttribute(g,e,"mode");
 if(!mode||mode->type!=2||mode->values.size()!=1||wireInt(mode->values[0])!=b.saved.mode)return false;
 auto endpoint=[&](const char*elementName,const char*attributeName,const char*indexName,const ChannelEndpoint&saved){auto ref=cachedSingleHierarchyReference(g,e,elementName);auto name=namedAttribute(g,e,attributeName);auto index=namedAttribute(g,e,indexName);
  return ref==saved.element&&name&&name->type==5&&name->values.size()==1&&name->values[0].text==saved.attribute&&index&&index->type==2&&index->values.size()==1&&wireInt(index->values[0])==saved.index;};
 if(cachedSingleHierarchyReference(g,e,"log")!=b.saved.log)return false;
 return endpoint("fromElement","fromAttribute","fromIndex",b.saved.source)&&endpoint("toElement","toAttribute","toIndex",b.saved.destination)&&cachedHierarchyReferencesEqual(g,g.elements.at(b.saved.log),"layers",b.saved.layers);
}
inline bool executeRuntimeFloatPlay(Graph&graph,const SessionRuntimeHandles&runtime,const SessionAttributeHandles&attributes,RuntimeCachedFloatPlayBinding&binding,int32_t tick,SessionAttributeChanges*changes=nullptr,bool force=false,const std::vector<std::pair<bool,bool>>*infinite=nullptr){
 if(!runtime.resolve(graph,binding.channelHandle)||!runtime.resolve(graph,binding.destinationHandle)||!runtime.resolve(graph,binding.logHandle))return false;
 for(uint32_t layer:binding.layerHandles)if(!runtime.resolve(graph,layer))return false;
 if(!cachedPlayMetadataCurrent(graph,binding))return false;
 auto endpoint=[&](uint32_t&cache,uint32_t owner,const std::string&name){auto a=attributes.resolve(graph,runtime,cache);if(!a){cache=attributes.find(owner,name);a=attributes.resolve(graph,runtime,cache);}return a;};
 auto destination=endpoint(binding.destinationAttribute,binding.destinationHandle,binding.saved.destination.attribute);
 if(!destination||(destination->type!=3&&destination->type!=17)||binding.saved.destination.index<0||size_t(binding.saved.destination.index)>=destination->values.size())return false;
 auto live=binding.saved;auto source=endpoint(binding.sourceAttribute,binding.sourceHandle,live.source.attribute);
 if(!source||!runtime.resolve(graph,binding.sourceHandle))live.source.element=0xffffffff;
 else if((source->type!=3&&source->type!=17)||live.source.index<0||size_t(live.source.index)>=source->values.size())return false;
 return executeSessionFloatPlay(graph,live,tick,changes,force,infinite);
}

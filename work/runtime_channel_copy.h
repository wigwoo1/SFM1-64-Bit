#pragma once
#include "session_runtime_handles.h"
#include "session_attribute_handles.h"
// Snapshot must be rebuilt after endpoint or mode edits.
struct RuntimeFloatCopyBinding{
 ChannelBinding saved;
 uint32_t channelHandle,sourceHandle=0xffffffff,destinationHandle=0xffffffff;
 uint32_t channelIndex;
 RuntimeFloatCopyBinding(const Graph&graph,uint32_t channel,const SessionRuntimeHandles&runtime):saved(graph,channel),channelHandle(runtime.graphHandles.at(channel)),channelIndex(channel){
  if(saved.source.present())sourceHandle=runtime.graphHandles.at(saved.source.element);
  if(saved.destination.present())destinationHandle=runtime.graphHandles.at(saved.destination.element);
 }
};
inline bool sameRuntimeEndpoint(const ChannelEndpoint&a,const ChannelEndpoint&b){return a.element==b.element&&a.attribute==b.attribute&&a.index==b.index&&a.scalarType==b.scalarType;}
inline bool runtimeChannelSnapshotCurrent(const Graph&graph,const RuntimeFloatCopyBinding&binding,bool playback=false){
 ChannelBinding current(graph,binding.channelIndex);
 return current.mode==binding.saved.mode&&sameRuntimeEndpoint(current.source,binding.saved.source)&&sameRuntimeEndpoint(current.destination,binding.saved.destination)&&(!playback||(current.log==binding.saved.log&&current.layers==binding.saved.layers));
}
inline bool executeRuntimeFloatCopy(Graph&graph,const SessionRuntimeHandles&runtime,const RuntimeFloatCopyBinding&binding,SessionAttributeChanges*changes=nullptr){
 if(!runtime.resolve(graph,binding.channelHandle)||!runtime.resolve(graph,binding.sourceHandle)||!runtime.resolve(graph,binding.destinationHandle))return false;
 if(!runtimeChannelSnapshotCurrent(graph,binding))return false;
 return executeSessionFloatCopy(graph,binding.saved,changes);
}
// Native channels lazily refind endpoint attributes when cached handles expire.
struct RuntimeCachedFloatCopyBinding:RuntimeFloatCopyBinding{
 uint32_t sourceAttribute=0xffffffff,destinationAttribute=0xffffffff;
 RuntimeCachedFloatCopyBinding(const Graph&graph,uint32_t channel,const SessionRuntimeHandles&runtime,const SessionAttributeHandles&attributes):RuntimeFloatCopyBinding(graph,channel,runtime){
  if(saved.source.present())sourceAttribute=attributes.find(sourceHandle,saved.source.attribute);
  if(saved.destination.present())destinationAttribute=attributes.find(destinationHandle,saved.destination.attribute);
 }
};
inline bool executeRuntimeFloatCopy(Graph&graph,const SessionRuntimeHandles&runtime,const SessionAttributeHandles&attributes,RuntimeCachedFloatCopyBinding&binding,SessionAttributeChanges*changes=nullptr){
 if(!runtime.resolve(graph,binding.channelHandle)||!runtime.resolve(graph,binding.sourceHandle)||!runtime.resolve(graph,binding.destinationHandle))return false;
 auto resolveEndpoint=[&](uint32_t&cache,uint32_t owner,const std::string&name){
  auto attribute=attributes.resolve(graph,runtime,cache);
  if(!attribute){cache=attributes.find(owner,name);attribute=attributes.resolve(graph,runtime,cache);}
  return attribute;
 };
 auto source=resolveEndpoint(binding.sourceAttribute,binding.sourceHandle,binding.saved.source.attribute);
 auto destination=resolveEndpoint(binding.destinationAttribute,binding.destinationHandle,binding.saved.destination.attribute);
 if(!source||!destination)return false;
 if(!runtimeChannelSnapshotCurrent(graph,binding))return false;
 return executeSessionFloatCopy(graph,binding.saved,changes);
}

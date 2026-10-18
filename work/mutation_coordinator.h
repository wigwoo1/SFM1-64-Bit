#pragma once
#include <map>
#include "mutation_cycle.h"
#include "mutation_queues.h"
#include "attribute_callback_lists.h"
#include "element_propagation_state.h"
struct MutationOwner{
 uint8_t state=0;uint32_t mask=0;
 std::map<uintptr_t,uint16_t> attributeFlags;
 AttributeCallbackLists callbacks;
 std::list<GeneralCallbackRecord> general;
};
// Explicit native metadata; no serialized DMX fields are assumed to be flags.
struct MutationCoordinator{
 MutationCycle cycle;MutationQueues queues;std::map<uint32_t,MutationOwner> owners;
 // Caller validates live generation and supplies the native suppression state.
 ElementPropagationDecision propagateOwner(uint32_t handle,uint32_t requested,bool suppressed){
  auto it=owners.find(handle);if(it==owners.end())return {};
  auto&owner=it->second;auto result=beginElementPropagation(owner.state,owner.mask,requested,suppressed,!owner.general.empty());
  if(result.queuePropagated)queues.appendPropagated(handle);
  if(result.queuePrepared)queues.appendPrepared(handle);
  return result;
 }
 bool begin(uint32_t handle,uintptr_t attribute,bool preparationSuppressed){
  auto it=owners.find(handle);if(it==owners.end())return false;auto&owner=it->second;
  auto flags=owner.attributeFlags.find(attribute);if(flags==owner.attributeFlags.end())return false;
  return beginAttributeMutation(flags->second,cycle.phase,[&](){owner.callbacks.prepare(attribute,preparationSuppressed,handle,[&](uint32_t h){queues.appendPrepared(h);});},[&](){queues.mark(owner.state,handle);});
 }
 template<class Live,class Notify> void processMarkedLive(Live live,Notify notify){
  queues.processMarked([&](uint32_t handle)->MutationOwner*{if(!live(handle))return nullptr;auto it=owners.find(handle);return it==owners.end()?nullptr:&it->second;},notify,[](MutationOwner*owner){owner->state&=0xfe;for(auto&entry:owner->attributeFlags)entry.second&=0xff7f;});
 }
 template<class Notify> void processMarked(Notify notify){processMarkedLive([](uint32_t){return true;},notify);}
 template<class Live,class General,class Attribute> void processPreparedLive(Live live,General general,Attribute attribute){
  size_t count=queues.prepared.size();for(size_t i=0;i<count;++i){auto handle=queues.prepared[i];if(!live(handle))continue;auto it=owners.find(handle);if(it==owners.end())continue;auto&owner=it->second;
   owner.callbacks.process(owner.state,owner.mask,reinterpret_cast<uintptr_t>(&owner),owner.general,general,attribute);
 }queues.prepared.clear();
 }
 template<class General,class Attribute> void processPrepared(General general,Attribute attribute){processPreparedLive([](uint32_t){return true;},general,attribute);}
 template<class Live> void processPropagatedLive(Live live){
  size_t count=queues.propagated.size();for(size_t i=0;i<count;++i){auto handle=queues.propagated[i];if(!live(handle))continue;auto it=owners.find(handle);if(it!=owners.end())clearElementPropagation(it->second.state,it->second.mask);}
  queues.propagated.clear();
 }
};

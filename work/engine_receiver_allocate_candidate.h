#pragma once
#include "engine_receiver_unlink_candidate.h"
struct EngineReceiverAllocationCandidate {
 EngineReceiverPoolCandidate pool;
 uint32_t freeHead=0xffff,freshCount=0;
 EngineReceiverLinkCandidate* cachedStorage=nullptr;
};
//10104E70 logical allocation. Growth callback supplies actual allocator semantics.
// Pool indices/capacities must satisfy the recovered storage preconditions.
inline int32_t engineReceiverFreshCandidate(const EngineReceiverPoolCandidate& pool){
 if(pool.highwater>=0&&pool.highwater<pool.capacity){
  const uint32_t next=uint32_t(pool.highwater)+1;
  return next<uint32_t(pool.capacity)?int32_t(next):-1;
 }
 return pool.capacity>0?0:-1;
}
template<class Grow,class Warn>
uint32_t engineReceiverAllocateCandidate(EngineReceiverAllocationCandidate& state,uint32_t initialize,Grow grow,Warn warn){
 uint32_t index=state.freeHead;
 if(index==0xffff){
  int32_t candidate=engineReceiverFreshCandidate(state.pool);
  if(candidate<0||candidate>=state.pool.capacity){
   grow(state.pool,1);state.cachedStorage=state.pool.links;
   candidate=engineReceiverFreshCandidate(state.pool);
   if(candidate<0||candidate>=state.pool.capacity){warn(false);return 0xffff;}
  }
  if(candidate>0xfffe){warn(true);return 0xffff;}
  ++state.freshCount;state.pool.highwater=candidate;index=uint32_t(candidate);
 }else state.freeHead=state.pool.links[index].next;
 auto& node=state.pool.links[index];
 node.previous=node.next=uint8_t(initialize)?uint16_t(0xffff):uint16_t(index);
 return index;
}

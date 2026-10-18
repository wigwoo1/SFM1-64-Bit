#pragma once
#include "engine_shadow_pool_unlink_candidate.h"
struct ModelRenderRecordAllocationCandidate {
 EngineShadowPoolStateCandidate pool;
 uint16_t freeHead=0xffff,freshCount=0;
 EngineShadowPoolLinkCandidate* cachedStorage=nullptr;
};
inline uint16_t modelRenderFreshIndexCandidate(const EngineShadowPoolStateCandidate& pool){
 if(int32_t(pool.maximumIndex)<pool.capacity){uint16_t next=uint16_t(pool.maximumIndex+1u);return int32_t(next)<pool.capacity?next:uint16_t(0xffff);}
 return pool.capacity>0?uint16_t(0):uint16_t(0xffff);
}
//100D4C70 logical x64 allocation. Growth and rate-limited warning services
// supplied explicitly. Warning true=index range; false=capacity failure.
template<class Grow,class Warn>
inline uint16_t modelRenderRecordAllocateCandidate(ModelRenderRecordAllocationCandidate& state,uint32_t initialize,Grow grow,Warn warn){
 uint16_t index=state.freeHead;
 if(index==0xffff){
  index=modelRenderFreshIndexCandidate(state.pool);
  if(int32_t(index)>=state.pool.capacity){grow(state.pool,1);state.cachedStorage=state.pool.links;index=modelRenderFreshIndexCandidate(state.pool);
   if(int32_t(index)>=state.pool.capacity){warn(false);return 0xffff;}
  }
  if(index==0xffff){warn(true);return 0xffff;}
  state.freshCount=uint16_t(state.freshCount+1u);state.pool.maximumIndex=index;
 }else state.freeHead=state.pool.links[index].next;
 auto& link=state.pool.links[index];link.previous=link.next=uint8_t(initialize)?uint16_t(0xffff):index;
 return index;
}

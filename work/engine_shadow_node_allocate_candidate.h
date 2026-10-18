#pragma once
#include <cstdint>
struct EngineShadowPoolNodeCandidate {
 uint16_t index=0;
 EngineShadowPoolNodeCandidate* previous=nullptr;
 EngineShadowPoolNodeCandidate* next=nullptr;
};
struct EngineShadowNodeBlockCandidate {
 EngineShadowNodeBlockCandidate* next=nullptr;
 int32_t count=0;
 EngineShadowPoolNodeCandidate* nodes=nullptr;
};
struct EngineShadowNodeFrontierCandidate {EngineShadowNodeBlockCandidate* block=nullptr;int32_t index=-1;};
inline bool engineShadowFrontierValidCandidate(const EngineShadowNodeFrontierCandidate& f){return f.block&&f.index>=0&&f.index<f.block->count;}
inline EngineShadowNodeFrontierCandidate engineShadowFrontierAdvanceCandidate(EngineShadowNodeFrontierCandidate f){
 if(!engineShadowFrontierValidCandidate(f))return {};
 if(f.index+1<f.block->count)return {f.block,f.index+1};
 return f.block->next?EngineShadowNodeFrontierCandidate{f.block->next,0}:EngineShadowNodeFrontierCandidate{};
}
struct EngineShadowNodePoolCandidate {
 EngineShadowNodeBlockCandidate* first=nullptr;
 EngineShadowPoolNodeCandidate* freeHead=nullptr;
 EngineShadowNodeFrontierCandidate frontier;
 uint32_t freshCount=0,field28=0;
 EngineShadowPoolNodeCandidate* head=nullptr;
 EngineShadowPoolNodeCandidate* tail=nullptr;
 uint32_t linkedCount=0;
};
//10104A60 owned64-bit representation. Grow/failure services remain explicit.
// Existing blocks/nodes must retain addresses; native12-byte layout is not reused.
template<class Grow,class Warn>
EngineShadowPoolNodeCandidate* engineShadowNodeAllocateCandidate(EngineShadowNodePoolCandidate& pool,uint32_t initialize,Grow grow,Warn warn){
 auto* node=pool.freeHead;
 if(node)pool.freeHead=node->next;
 else {
  auto candidate=engineShadowFrontierValidCandidate(pool.frontier)?engineShadowFrontierAdvanceCandidate(pool.frontier):EngineShadowNodeFrontierCandidate{pool.first,pool.first?0:-1};
  if(!engineShadowFrontierValidCandidate(candidate)){
   grow(pool,1);pool.field28=0;
   candidate=engineShadowFrontierValidCandidate(pool.frontier)?engineShadowFrontierAdvanceCandidate(pool.frontier):EngineShadowNodeFrontierCandidate{pool.first,pool.first?0:-1};
   if(!engineShadowFrontierValidCandidate(candidate)){warn();return nullptr;}
  }
  node=candidate.block->nodes+candidate.index;pool.frontier=candidate;++pool.freshCount;
 }
 node->previous=node->next=uint8_t(initialize)?nullptr:node;
 return node;
}

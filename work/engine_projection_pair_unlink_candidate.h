#pragma once
#include "engine_shadow_pool_unlink_candidate.h"
struct EngineProjectionPairPayloadCandidate {uint16_t oppositeKey,oppositeIndex;};
struct EngineProjectionPairMapCandidate {
 EngineShadowPoolStateCandidate opposite,current;
 EngineProjectionPairPayloadCandidate* payload=nullptr;
 EngineProjectionPairPayloadCandidate* reversePayload=nullptr;
 uint16_t oppositeFree=0xffff,currentFree=0xffff;
};
//1010A070: head providers return current storage, not a saved head pointer.
// Logical x64 layout; original entries combine payload and links in8bytes.
template<class CurrentHead,class OppositeHead>
inline void engineProjectionPairUnlinkCandidate(EngineProjectionPairMapCandidate& map,uint32_t key,CurrentHead currentHead,OppositeHead oppositeHead){
 uint16_t index=*currentHead(key);
 while(index!=0xffff){uint16_t otherKey=map.payload[index].oppositeKey,other=map.payload[index].oppositeIndex;
  if(*oppositeHead(otherKey)==other){uint16_t next=map.opposite.links[other].next;*oppositeHead(otherKey)=next;}
  engineShadowPoolUnlinkCandidate(map.opposite,other);
  map.opposite.links[other].next=map.oppositeFree;map.oppositeFree=other;
  uint16_t next=map.current.links[index].next;
  engineShadowPoolUnlinkCandidate(map.current,index);
  map.current.links[index].next=map.currentFree;map.currentFree=index;index=next;
 }
 *currentHead(key)=0xffff;
}
//1010A160 traverses the first native pool (opposite above) from the model
// head and removes corresponding second-pool nodes from shadow lists.
template<class ModelHead,class ShadowHead>
inline void engineProjectionPairUnlinkModelCandidate(EngineProjectionPairMapCandidate& map,uint32_t key,ModelHead modelHead,ShadowHead shadowHead){
 uint16_t index=*modelHead(key);
 while(index!=0xffff){uint16_t shadowKey=map.reversePayload[index].oppositeKey,other=map.reversePayload[index].oppositeIndex;
  if(*shadowHead(shadowKey)==other){uint16_t next=map.current.links[other].next;*shadowHead(shadowKey)=next;}
  engineShadowPoolUnlinkCandidate(map.current,other);
  map.current.links[other].next=map.currentFree;map.currentFree=other;
  uint16_t next=map.opposite.links[index].next;
  engineShadowPoolUnlinkCandidate(map.opposite,index);
  map.opposite.links[index].next=map.oppositeFree;map.oppositeFree=index;index=next;
 }
 *modelHead(key)=0xffff;
}

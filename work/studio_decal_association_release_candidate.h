#pragma once
#include <cstdint>
//10007530: logical form of the owner's 16-byte association pointer pool.
// Frontier classification remains explicit; this routine has no lock locally.
struct StudioDecalAssociationNodeCandidate {
 uintptr_t resource=0,payload=0;
 StudioDecalAssociationNodeCandidate* previous=nullptr;
 StudioDecalAssociationNodeCandidate* next=nullptr;
};
struct StudioDecalAssociationPoolCandidate {
 StudioDecalAssociationNodeCandidate* head=nullptr;
 StudioDecalAssociationNodeCandidate* tail=nullptr;
 StudioDecalAssociationNodeCandidate* freeHead=nullptr;
 uint32_t count=0;
};
template<class Frontier>
inline void studioDecalAssociationReleaseCandidate(StudioDecalAssociationPoolCandidate& pool,uintptr_t resource,Frontier beyond){
 auto* node=pool.head;
 while(node){auto* next=node->next;
  if(node->resource==resource){
   if(!beyond(pool,node)&&node->previous!=node){
    if(node->previous)node->previous->next=next;else pool.head=next;
    if(node->next)node->next->previous=node->previous;else pool.tail=node->previous;
    node->previous=node->next=node;--pool.count;
   }
   node->next=pool.freeHead;pool.freeHead=node;
  }
  node=next;
 }
}

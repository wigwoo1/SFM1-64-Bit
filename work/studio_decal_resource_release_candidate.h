#pragma once
#include "studio_decal_pointer_insert_candidate.h"
#include <cstring>
//10008B60 outer lifecycle. Inner54-byte-pool cleanup/association services
// remain explicit until their allocator/resource contracts are ported.
template<class Associations,class Head,class Chain,class Entries,class Lock,class Unlock,class Frontier>
inline void studioDecalResourceReleaseLiveCandidate(StudioDecalPointerPoolCandidate& pool,StudioDecalPointerNodeCandidate* node,Associations removeAssociations,Head readHead,Chain releaseChain,Entries releaseEntries,Lock lock,Unlock unlock,Frontier beyond){
 if(!node)return;removeAssociations(node);
 for(int32_t i=0;i<node->entryCount;++i){uint16_t head=readHead(*node,i);if(head!=0xffff)releaseChain(head);}
 if(node->entries)releaseEntries(node->entries,3);node->entries=0;
 lock();studioDecalPointerUnlinkCandidate(pool,node,beyond);node->next=pool.freeHead;pool.freeHead=node;unlock();
}
// Original byte schema used by native parity fixtures.
template<class Associations,class Chain,class Entries,class Lock,class Unlock,class Frontier>
inline void studioDecalResourceReleaseCandidate(StudioDecalPointerPoolCandidate& pool,StudioDecalPointerNodeCandidate* node,Associations removeAssociations,Chain releaseChain,Entries releaseEntries,Lock lock,Unlock unlock,Frontier beyond){
 studioDecalResourceReleaseLiveCandidate(pool,node,removeAssociations,[](const auto& resource,int32_t i){uint16_t head;std::memcpy(&head,reinterpret_cast<const unsigned char*>(resource.entries)+size_t(i)*32,2);return head;},releaseChain,releaseEntries,lock,unlock,beyond);
}

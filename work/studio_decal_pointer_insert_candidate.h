#pragma once
#include "studio_decal_pointer_allocate_candidate.h"
//100074A0. Frontier classification remains explicit: original signed32
// address ordering must not be replaced by ordinary ownership membership.
template<class BeyondFrontier>
inline void studioDecalPointerUnlinkCandidate(StudioDecalPointerPoolCandidate& pool,StudioDecalPointerNodeCandidate* node,BeyondFrontier beyond){
 if(!node||beyond(pool,node)||node->previous==node)return;
 if(node->previous)node->previous->next=node->next;else pool.head=node->next;
 if(node->next)node->next->previous=node->previous;else pool.tail=node->previous;
 node->previous=node->next=node;--pool.linkedCount;
}
template<class BeyondFrontier>
inline void studioDecalPointerInsertCandidate(StudioDecalPointerPoolCandidate& pool,StudioDecalPointerNodeCandidate* before,StudioDecalPointerNodeCandidate* node,BeyondFrontier beyond){
 studioDecalPointerUnlinkCandidate(pool,node,beyond);
 node->next=before;auto* previous=before?before->previous:pool.tail;
 if(before)before->previous=node;else pool.tail=node;
 node->previous=previous;if(previous)previous->next=node;else pool.head=node;++pool.linkedCount;
}

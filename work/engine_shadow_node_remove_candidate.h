#pragma once
#include "engine_shadow_node_allocate_candidate.h"
//101091E0; nonnull valid node and single-free are caller preconditions.
// Frontier predicate is explicit until64-bit ownership policy is integrated.
template<class BeyondFrontier>
void engineShadowNodeRemoveCandidate(EngineShadowNodePoolCandidate& pool,EngineShadowPoolNodeCandidate* node,BeyondFrontier beyond){
 if(node&&!beyond(node)&&node->previous!=node){
  if(node->previous)node->previous->next=node->next;else pool.head=node->next;
  if(node->next)node->next->previous=node->previous;else pool.tail=node->previous;
  node->next=node->previous=node;--pool.linkedCount;
 }
 node->next=pool.freeHead;pool.freeHead=node;
}

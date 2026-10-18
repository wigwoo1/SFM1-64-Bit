#pragma once
#include "engine_shadow_node_allocate_candidate.h"
//10109250 owned links. Valid distinct target/newnode required by caller.
template<class BeyondFrontier>
void engineShadowNodeInsertCandidate(EngineShadowNodePoolCandidate& pool,EngineShadowPoolNodeCandidate* target,EngineShadowPoolNodeCandidate* node,BeyondFrontier beyond){
 if(node&&!beyond(node)&&node->previous!=node){
  if(node->previous)node->previous->next=node->next;else pool.head=node->next;
  if(node->next)node->next->previous=node->previous;else pool.tail=node->previous;
  node->next=node->previous=node;--pool.linkedCount;
 }
 node->next=target;EngineShadowPoolNodeCandidate* previous;
 if(!target){previous=pool.tail;pool.tail=node;}
 else {previous=target->previous;target->previous=node;}
 node->previous=previous;
 if(!previous)pool.head=node;else previous->next=node;
 ++pool.linkedCount;
}

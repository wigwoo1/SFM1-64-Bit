#pragma once
#include "engine_shadow_pool_unlink_candidate.h"
//10003FE0/10004050: logical links from original30h/18h entries.
// Payloads remain untouched; this operation does not free backing storage.
inline void studioIndexPoolRemoveAllCandidate(EngineShadowPoolStateCandidate& pool,uint16_t& freeHead){
 if(pool.maximumIndex==0xffff)return;
 uint16_t index=pool.head;
 while(index!=0xffff){auto& node=pool.links[index];uint16_t next=node.next;node.previous=index;node.next=next==0xffff?freeHead:next;index=next;}
 if(pool.head!=0xffff)freeHead=pool.head;
 pool.head=pool.tail=0xffff;pool.count=0;
}

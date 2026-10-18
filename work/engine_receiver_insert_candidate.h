#pragma once
#include "engine_receiver_unlink_candidate.h"
//101092E0: unlink first, then insert before target or append on full32ffff.
// Valid target/index and neighbor storage are native caller preconditions.
inline void engineReceiverInsertCandidate(EngineReceiverPoolCandidate& pool,uint32_t target,uint32_t index){
 engineReceiverUnlinkCandidate(pool,int32_t(index));
 auto& node=pool.links[index];node.next=uint16_t(target);
 uint16_t previous;
 if(target==0xffff){previous=uint16_t(pool.tail);node.previous=previous;pool.tail=index;}
 else {previous=pool.links[target].previous;node.previous=previous;pool.links[target].previous=uint16_t(index);}
 if(previous==0xffff)pool.head=index;else pool.links[previous].next=uint16_t(index);
 ++pool.count;
}

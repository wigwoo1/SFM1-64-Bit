#pragma once
#include "engine_shadow_pool_unlink_candidate.h"
//100D7EB0: unlink incoming index, then insert before target or appendffff.
// Valid indices/storage are caller preconditions, matching original writes.
inline void modelRenderRecordInsertCandidate(EngineShadowPoolStateCandidate& pool,uint32_t beforeArgument,uint32_t indexArgument){
 engineShadowPoolUnlinkCandidate(pool,indexArgument);
 uint16_t index=uint16_t(indexArgument),before=uint16_t(beforeArgument),previous;
 pool.links[index].next=before;
 if(before==0xffff){previous=pool.tail;pool.links[index].previous=previous;pool.tail=index;}
 else{previous=pool.links[before].previous;pool.links[index].previous=previous;pool.links[before].previous=index;}
 if(previous==0xffff)pool.head=index;else pool.links[previous].next=index;
 pool.count=uint16_t(pool.count+1u);
}

#pragma once
#include "engine_shadow_pool_unlink_candidate.h"
#include "studio_decal_chain_release_candidate.h"
//10006A30 shares the recovered guarded uint16 unlink behavior. Logical pool
// links live separately from the native 84-byte payload layout.
inline void studioDecalIndexUnlinkCandidate(EngineShadowPoolStateCandidate& pool,uint32_t argument){engineShadowPoolUnlinkCandidate(pool,argument);}
template<class GetNode>
inline void studioDecalChainIndexUnlinkCandidate(EngineShadowPoolStateCandidate& pool,uint32_t argument,GetNode getNode){
 studioDecalIndexUnlinkCandidate(pool,argument);
 uint16_t index=uint16_t(argument);
 if(int32_t(index)>=pool.capacity||index>pool.maximumIndex)return;
 auto* node=getNode(index);node->previous=pool.links[index].previous;node->next=pool.links[index].next;
}

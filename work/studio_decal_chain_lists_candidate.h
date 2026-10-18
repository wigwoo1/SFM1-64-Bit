#pragma once
#include "studio_decal_chain_release_candidate.h"
#include "studio_index_pool_remove_all_candidate.h"
// Resolve logical list metadata separately from backing addresses. Both native
// strides share the verified remove-all link behavior; payloads stay untouched.
struct StudioDecalChainListCandidate {
 EngineShadowPoolStateCandidate pool;
 uint16_t freeHead=0xffff;
};
template<class GetNode,class Unlink,class ResolveList,class Free>
inline void studioDecalChainReleaseWithListsCandidate(uint16_t head,uint16_t& freeHead,uint32_t& counter,GetNode getNode,Unlink unlinkIndex,ResolveList resolveList,Free freeBuffer){
 auto remove=[&](StudioDecalChainBufferCandidate& buffer){auto& list=resolveList(buffer);studioIndexPoolRemoveAllCandidate(list.pool,list.freeHead);};
 studioDecalChainReleaseCandidate(head,freeHead,counter,getNode,unlinkIndex,remove,remove,freeBuffer);
}

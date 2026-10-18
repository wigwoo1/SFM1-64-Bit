#pragma once
#include "studio_decal_chain_lists_candidate.h"
#include "studio_decal_resource_release_candidate.h"
struct StudioDecalEntryCandidate {
 uint16_t chainHead=0xffff;
 StudioDecalChainBufferCandidate buffer;
 StudioDecalChainListCandidate list;
};
template<class Associations,class Chain,class Entries,class Lock,class Unlock,class Frontier>
inline void studioDecalTypedResourceReleaseCandidate(StudioDecalPointerPoolCandidate& pool,StudioDecalPointerNodeCandidate* node,Associations removeAssociations,Chain releaseChain,Entries releaseEntries,Lock lock,Unlock unlock,Frontier beyond){
 studioDecalResourceReleaseLiveCandidate(pool,node,removeAssociations,[](const auto& resource,int32_t i){return reinterpret_cast<const StudioDecalEntryCandidate*>(resource.entries)[i].chainHead;},releaseChain,releaseEntries,lock,unlock,beyond);
}
//100073D0. Count is the allocation's saved array header, not the resource's
// live entry count. Nonnegative representable counts/storage are preconditions.
// Allocation identities are explicit; native header byte arithmetic is not x64 ABI.
template<class FreeBuffer,class FreeAllocation>
inline uintptr_t studioDecalEntryDeleteCandidate(StudioDecalEntryCandidate* entries,int32_t savedCount,uint32_t flags,uintptr_t entryIdentity,uintptr_t allocationIdentity,FreeBuffer freeBuffer,FreeAllocation freeAllocation){
 auto destroy=[&](auto& entry){studioIndexPoolRemoveAllCandidate(entry.list.pool,entry.list.freeHead);if(entry.buffer.growth>=0){if(entry.buffer.address){freeBuffer(entry.buffer.address);entry.buffer.address=0;}entry.buffer.capacity=0;}};
 if(flags&2){for(int32_t i=savedCount;i>0;--i)destroy(entries[i-1]);if(flags&1)freeAllocation(allocationIdentity);return allocationIdentity;}
 destroy(*entries);if(flags&1)freeAllocation(entryIdentity);return entryIdentity;
}

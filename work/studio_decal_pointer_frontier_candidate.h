#pragma once
#include "studio_decal_pointer_allocate_candidate.h"
//10003ED0: unsigned ranges, explicit frontier ordering. Original setg on
// signed32 addresses needs a caller-selected x64 policy; do not silently narrow.
// Includes unaligned interior addresses, matching observed native range tests.
template<class FrontierOrder>
inline bool studioDecalPointerBeyondFrontierCandidate(const StudioDecalPointerPoolCandidate& pool,const StudioDecalPointerNodeCandidate* node,FrontierOrder order){
 auto f=pool.frontier;if(!studioDecalFrontierValidCandidate(f))return false;
 const uintptr_t address=reinterpret_cast<uintptr_t>(node);
 auto inside=[&](const StudioDecalPointerBlockCandidate* block){uintptr_t start=reinterpret_cast<uintptr_t>(block->nodes),end=start+uintptr_t(block->count)*sizeof(StudioDecalPointerNodeCandidate);return address>=start&&address<end;};
 if(inside(f.block))return order(address,reinterpret_cast<uintptr_t>(f.block->nodes+f.index));
 for(auto* block=f.block->next;block;block=block->next)if(inside(block))return true;
 return false;
}

#pragma once
#include "studio_decal_association_release_candidate.h"
struct StudioDecalAssociationBlockCandidate {
 StudioDecalAssociationBlockCandidate* next=nullptr;
 int32_t count=0;
 StudioDecalAssociationNodeCandidate* nodes=nullptr;
};
//10003E50: range checks admit unaligned interior addresses. Address ordering
// is explicit because original signed32 setg is not a full-width x64 policy.
template<class Order>
inline bool studioDecalAssociationBeyondFrontierCandidate(StudioDecalAssociationBlockCandidate* block,int32_t index,const StudioDecalAssociationNodeCandidate* node,Order order){
 if(!block||index<0||index>=block->count)return false;
 uintptr_t address=reinterpret_cast<uintptr_t>(node);
 auto inside=[&](auto* b){uintptr_t start=reinterpret_cast<uintptr_t>(b->nodes);return address>=start&&address<start+uintptr_t(b->count)*sizeof(StudioDecalAssociationNodeCandidate);};
 if(inside(block))return order(address,reinterpret_cast<uintptr_t>(block->nodes+index));
 for(block=block->next;block;block=block->next)if(inside(block))return true;
 return false;
}

#pragma once
#include "engine_shadow_node_allocate_candidate.h"
//1010A6B0: capture successor before release mutates node/free-list links.
// Final setHead must reload current owner storage through its provider.
template<class Head,class Entry,class Release,class SetHead>
inline void engineProjectionCleanupCandidate(uint32_t handle,Head head,Entry entry,Release release,SetHead setHead){
 uint16_t selected=uint16_t(handle);auto* node=head(selected);
 while(node){uint16_t index=node->index;auto* next=node->next;auto* object=entry(index)->object;release(object,index);node=next;}
 setHead(selected,nullptr);
}

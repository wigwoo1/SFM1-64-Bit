#pragma once
#include <cstdint>
#include <vector>
#include "element_mutation_state.h"
// Three native append-only vectors at framework +0x64, +0x78, +0x8C.
struct MutationQueues{
 std::vector<uint32_t> marked,propagated,prepared;
 void appendMarked(uint32_t handle){marked.push_back(handle);}
 void appendPropagated(uint32_t handle){propagated.push_back(handle);}
 void appendPrepared(uint32_t handle){prepared.push_back(handle);}
 void mark(uint8_t&ownerState,uint32_t handle){markElementMutation(ownerState,handle,[&](uint32_t h){appendMarked(h);});}
 // 0x1038F210 snapshots count, resolves each handle, calls owner slot +0x0C,
 // then clears owner mutation state. Appends during processing are not visited.
 template<class Resolve,class Notify,class Reset> void processMarked(Resolve resolve,Notify notify,Reset reset){
  size_t count=marked.size();for(size_t i=0;i<count;++i){auto owner=resolve(marked[i]);if(owner){notify(owner);reset(owner);}}
  marked.clear();
 }
};

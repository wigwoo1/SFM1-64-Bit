#pragma once
#include <cstring>
#include "studio_material_stack_growth_candidate.h"
// Storage portion of 1002aed0. Other context fields and its vtables are separate.
struct StudioMaterialBaseStacksCandidate {
 StudioMaterialContextDescriptorCandidate state;
 std::array<StudioMaterialContextDescriptorCandidate,11> matrices;
 uintptr_t currentMatrix=0;
};
// Allocation must succeed and provide initialized bytes for the retained flags.
// Initial state is opaque; its future pointer-bearing schema is not yet recovered.
template<class Allocate,class Reallocate>void studioMaterialBaseStacksConstructCandidate(StudioMaterialBaseStacksCandidate& stacks,Allocate allocate,Reallocate reallocate){
 stacks.state={0,32,16,0,0};stacks.state.address=allocate(32*36);stacks.state.cache=stacks.state.address;
 stacks.matrices={};
 uint32_t initialState[9]={0,0,0,0,0,0,0,0xffffffffu,0xffffffffu};
 ++stacks.state.count;std::memcpy(reinterpret_cast<void*>(stacks.state.address),initialState,sizeof(initialState));
 float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
 for(auto& d:stacks.matrices){if(d.count>=d.capacity)studioMaterialStackGrowCandidate(d,1,68,allocate,reallocate);++d.count;d.cache=d.address;auto address=d.address+uintptr_t(d.count-1)*68;
 std::memcpy(reinterpret_cast<void*>(address),identity,sizeof(identity));uint32_t flags;std::memcpy(&flags,reinterpret_cast<void*>(address+64),sizeof(flags));flags|=3;std::memcpy(reinterpret_cast<void*>(address+64),&flags,sizeof(flags));}
 stacks.currentMatrix=stacks.matrices[0].address+uintptr_t(stacks.matrices[0].count-1)*68;
}

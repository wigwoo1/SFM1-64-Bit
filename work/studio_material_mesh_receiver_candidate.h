#pragma once
#include <cstdint>
// Logical10021100; callbacks represent immediate-context slots127/57/159.
// Returned receiver is stored after the selected callback finishes.
template<class Existing,class Default,class Formatted>
void studioMaterialMeshReceiverCandidate(uintptr_t& receiver,uint32_t formatLow,uint32_t formatHigh,uint32_t flags,uintptr_t argument4,uintptr_t argument5,uintptr_t argument6,Existing existing,Default defaults,Formatted formatted){
 uintptr_t selected;
 if(flags&2)selected=existing();
 else if(!(formatLow|formatHigh))selected=defaults(flags&1,argument4,argument5,argument6);
 else selected=formatted(formatLow,formatHigh,flags&1,argument4,argument5,argument6);
 receiver=selected;
}

#pragma once
#include "reference_handle_binding.h"
//10354B20: retired slots are not resolvable; handle bit31 is not generation.
inline uintptr_t resolveRecoveredReferenceHandleCandidate(uint32_t handle,const std::vector<RecoveredReferenceHandleSlotCandidate>&slots){
 if(handle==0xffffffffu)return 0;
 const uint32_t index=handle&0x3fffffu;if(index>=slots.size())return 0;
 const auto&slot=slots[index];const uint32_t generation=(handle>>22)&0x1ffu;
 if(((generation^slot.state)&0x7fffffffu)!=0||(slot.state&0x80000000u))return 0;
 return slot.element;
}

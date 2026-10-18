#pragma once
#include <cstdint>
#include <vector>
struct RecoveredReferenceHandleSlotCandidate{uint32_t state=0;uintptr_t element=0;};
//10354AD0: 22bit slot, 9bit generation, bit31 ignored in handle comparison.
inline void bindRecoveredReferenceHandleCandidate(uint32_t handle,uintptr_t element,std::vector<RecoveredReferenceHandleSlotCandidate>&slots,uint32_t&active){
 if(handle==0xffffffffu)return;
 const uint32_t index=handle&0x3fffffu;if(index>=slots.size())return;
 auto&slot=slots[index];const uint32_t generation=(handle>>22)&0x1ffu;
 if(((generation^slot.state)&0x7fffffffu)!=0)return;
 if(slot.state&0x80000000u){++active;slot.state&=0x7fffffffu;}
 slot.element=element;
}

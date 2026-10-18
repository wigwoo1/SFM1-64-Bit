#pragma once
#include "reference_handle_binding.h"
//10356E70: index-only retirement; caller must validate handle ownership.
template<class AppendFree>
void retireRecoveredReferenceHandleCandidate(uint32_t handle,std::vector<RecoveredReferenceHandleSlotCandidate>&slots,uint32_t&active,AppendFree appendFree){
 const uint32_t index=handle&0x3fffffu;if(index>=slots.size())return;
 auto&slot=slots[index];slot.state=(slot.state&0x80000000u)|((slot.state+1)&0x7fffffffu);
 if(!(slot.state&0x80000000u)){slot.state|=0x80000000u;--active;}
 slot.element=0;
 if((slot.state&0x7fffffffu)<0x1ffu)appendFree(index);
}

#pragma once
#include "reference_handle_retirement.h"
//10359C80 selects framework+C0EC table, rejectsFFFFFFFF, then index-only retire.
template<class AppendFree>
void retireRecoveredReferenceAttributeHandleCandidate(uint32_t handle,std::vector<RecoveredReferenceHandleSlotCandidate>& slots,uint32_t& active,AppendFree appendFree){
 if(handle!=0xffffffffu)retireRecoveredReferenceHandleCandidate(handle,slots,active,appendFree);
}

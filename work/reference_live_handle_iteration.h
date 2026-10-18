#pragma once
#include "reference_handle_binding.h"
//10356040/103560D0: snapshot count per iterator call, resolve every candidate
// includingFFFFFFFF for NULL slots. next(FFFFFFFF) starts at0, preservesbit31.
template<class Resolve>
uint32_t scanRecoveredReferenceLiveHandleCandidate(const std::vector<RecoveredReferenceHandleSlotCandidate>& slots,uint32_t start,uint32_t tag,Resolve resolve){
 const auto count=slots.size();for(uint32_t i=start;i<count;++i){auto h=slots[i].element&&i<=0x3fffffu?tag|((slots[i].state&511)<<22)|i:0xffffffffu;if(resolve(h))return h;}return 0xffffffffu;
}
template<class Resolve>uint32_t firstRecoveredReferenceLiveHandleCandidate(const std::vector<RecoveredReferenceHandleSlotCandidate>&slots,Resolve resolve){return scanRecoveredReferenceLiveHandleCandidate(slots,0,0,resolve);}
template<class Resolve>uint32_t nextRecoveredReferenceLiveHandleCandidate(const std::vector<RecoveredReferenceHandleSlotCandidate>&slots,uint32_t current,Resolve resolve){return scanRecoveredReferenceLiveHandleCandidate(slots,current==0xffffffffu?0:(current&0x3fffffu)+1,current&0x80000000u,resolve);}

#pragma once
#include "reference_record_transfer.h"
// Original1035B384..1035B415: true retires handle; false preserves pending record.
inline bool retireRecoveredRemovedElementHandleCandidate(const RecoveredTransferReferenceRecordCandidate& record,uint32_t mode){
 return mode==0||(mode==2&&record.headAttribute==0xffffffffu&&(record.counts&0x3fffffffu)==0);
}

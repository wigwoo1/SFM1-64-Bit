#pragma once
#include "pending_reference_identity.h"
#include "reference_record_transfer.h"
//Pointer-width runtime state; native identity at+4 and record at+1C.
struct RecoveredReferenceRuntimeElementCandidate{
 RecoveredPendingIdentityCandidate identity{};
 RecoveredTransferReferenceRecordCandidate record{};
 uint32_t cleanupContext=0xffffffffu; //Native+38; semantic identity not yet recovered.
};
inline bool recoveredReferenceTargetNeedsCleanupCandidate(const RecoveredReferenceRuntimeElementCandidate*element){return !element||element->cleanupContext!=0xffffffffu;}
inline void setRecoveredReferenceElementIdentityCandidate(RecoveredReferenceRuntimeElementCandidate&element,const RecoveredPendingIdentityCandidate&identity){element.identity=identity;}
inline void setRecoveredReferenceElementHandleCandidate(RecoveredReferenceRuntimeElementCandidate&element,uint32_t handle){element.record.handle=handle;}

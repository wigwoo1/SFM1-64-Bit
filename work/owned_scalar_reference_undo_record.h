#pragma once
#include "reference_identity_runtime.h"
#include "scalar_reference_undo_replay.h"
#include "undo_retained_target_assignment.h"
// Ledger must outlive retained targets. No copy: each object owns one count.
class RecoveredOwnedUndoTargetCandidate {
 RecoveredReferenceIdentityRuntimeCandidate& runtime;
public:
 uint32_t handle=0xffffffffu;
 explicit RecoveredOwnedUndoTargetCandidate(RecoveredReferenceIdentityRuntimeCandidate&r):runtime(r){}
 RecoveredOwnedUndoTargetCandidate(const RecoveredOwnedUndoTargetCandidate&)=delete;
 RecoveredOwnedUndoTargetCandidate&operator=(const RecoveredOwnedUndoTargetCandidate&)=delete;
 RecoveredOwnedUndoTargetCandidate(RecoveredOwnedUndoTargetCandidate&&other)noexcept:runtime(other.runtime),handle(other.handle){other.handle=0xffffffffu;}
 ~RecoveredOwnedUndoTargetCandidate(){runtime.mutateUndoTarget(handle,true);}
 // Native 1025BF80: release, write, filter resolved class, retain.
 template<class Accept>void assign(uint32_t incoming,Accept accept){assignRecoveredUndoRetainedTargetCandidate(handle,incoming,[&](uint32_t h){runtime.mutateUndoTarget(h,true);},[&](uint32_t h){return runtime.resolve(h);},[&](auto*e){return accept(*e);},[&](uint32_t h){runtime.mutateUndoTarget(h,false);});}
};
struct RecoveredOwnedScalarUndoRecordCandidate {
 uint32_t owner,name;RecoveredOwnedUndoTargetCandidate before,after;
 template<class Accept>RecoveredOwnedScalarUndoRecordCandidate(RecoveredReferenceIdentityRuntimeCandidate&r,uint32_t ownerHandle,uint32_t nameId,uint32_t oldTarget,uint32_t newTarget,Accept accept):owner(ownerHandle),name(nameId),before(r),after(r){before.assign(oldTarget,accept);after.assign(newTarget,accept);}
 RecoveredOwnedScalarUndoRecordCandidate(RecoveredOwnedScalarUndoRecordCandidate&&)=default;
 RecoveredScalarReferenceUndoPayloadCandidate payload()const{return {owner,name,before.handle,after.handle};}
};

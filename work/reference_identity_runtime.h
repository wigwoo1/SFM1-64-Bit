#pragma once
#include "pending_identity_handle_creation.h"
#include "reference_identity_reassignment.h"
#include "reference_record_list_arena.h"
#include "reference_target_wrapper.h"
#include "reference_cleanup_dispatch.h"
#include "undo_reference_target_counts.h"
//Caller-owned elements must remain alive and at stable addresses while bound.
class RecoveredReferenceIdentityRuntimeCandidate{
public:
 RecoveredReferenceHandleTableCandidate handles;
 RecoveredLiveIdentityRegistryCandidate live;
 RecoveredPendingReferenceRegistryCandidate pending;
 RecoveredReferenceRecordListArenaCandidate nodes;
 uint32_t frameworkFlags=0;
 uint8_t frameworkState=0;
 void mutateUndoTarget(uint32_t handle,bool release){mutateRecoveredUndoTargetCountCandidate(handle,release,frameworkState,[&](uint32_t h){return record(h);},[&](uint32_t h){return recoveredReferenceTargetNeedsCleanupCandidate(resolve(h));});}
 template<class RunPass>void dispatchCleanup(RunPass pass){dispatchRecoveredReferenceCleanupCandidate(frameworkState,pass);}
 RecoveredReferenceRuntimeElementCandidate*resolve(uint32_t handle)const{return reinterpret_cast<RecoveredReferenceRuntimeElementCandidate*>(handles.resolve(handle));}
 //Binding a supplied logical element; not the original full element factory.
 uint32_t registerExistingElement(RecoveredReferenceRuntimeElementCandidate&element){uint32_t h=handles.allocate();setRecoveredReferenceElementHandleCandidate(element,h);handles.bind(h,reinterpret_cast<uintptr_t>(&element));live.insert(h,[&](uint32_t value)->const RecoveredPendingIdentityCandidate*{auto*p=resolve(value);return p?&p->identity:nullptr;});return h;}
 template<class Notify>uint32_t pendingHandle(const RecoveredPendingIdentityCandidate&identity,Notify notify){return createRecoveredPendingIdentityHandleCandidate(identity,handles,pending,frameworkFlags,[&](const auto&key){return live.find(key,[&](uint32_t h)->const RecoveredPendingIdentityCandidate*{auto*p=resolve(h);return p?&p->identity:nullptr;});},notify);}
 RecoveredTransferReferenceRecordCandidate*record(uint32_t handle){return pending.lookupRecord(handle,[&](uint32_t h)->RecoveredTransferReferenceRecordCandidate*{auto*p=resolve(h);return p?&p->record:nullptr;});}
 uint32_t reassign(uint32_t handle,const RecoveredPendingIdentityCandidate&oldIdentity,const RecoveredPendingIdentityCandidate&newIdentity){return reassignRecoveredReferenceIdentityCandidate(handle,oldIdentity,newIdentity,handles,live,pending,frameworkFlags);}
 template<class NeedsCleanup>void mutate(uint32_t target,uint32_t attribute,bool present,bool removal,NeedsCleanup needsCleanup){mutateRecoveredReferenceTargetCandidate(target,present,removal,frameworkState,[&](uint32_t h){return record(h);},needsCleanup,[&](auto&r,bool remove){if(remove)nodes.remove(r,attribute);else nodes.add(r,attribute);});}
 void mutate(uint32_t target,uint32_t attribute,bool present,bool removal){mutate(target,attribute,present,removal,[&](uint32_t h){return recoveredReferenceTargetNeedsCleanupCandidate(resolve(h));});}
};

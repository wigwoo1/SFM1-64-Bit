#pragma once
#include "reference_runtime_element.h"
#include "live_reference_identity_registry.h"
#include "pending_reference_registry.h"
#include "reference_handle_table.h"
inline uint32_t reassignRecoveredReferenceIdentityCandidate(uint32_t current,const RecoveredPendingIdentityCandidate&oldIdentity,const RecoveredPendingIdentityCandidate&newIdentity,RecoveredReferenceHandleTableCandidate&handles,RecoveredLiveIdentityRegistryCandidate&live,RecoveredPendingReferenceRegistryCandidate&pending,uint32_t&frameworkFlags){
 auto resolve=[&](uint32_t h)->const RecoveredPendingIdentityCandidate*{auto*p=reinterpret_cast<RecoveredReferenceRuntimeElementCandidate*>(handles.resolve(h));return p?&p->identity:nullptr;};
 const uint32_t oldPosition=live.findPosition(oldIdentity,resolve);if(oldPosition==0xffffffffu)return current;
 auto*element=reinterpret_cast<RecoveredReferenceRuntimeElementCandidate*>(handles.resolve(current));if(!element)return 0xffffffffu;
 if(live.findPosition(newIdentity,resolve)!=0xffffffffu)return 0xffffffffu;
 live.erase(oldPosition);setRecoveredReferenceElementIdentityCandidate(*element,newIdentity);
 uint32_t bucket=0,index=0;if(!pending.find(newIdentity,bucket,index)){live.insert(current,resolve);return current;}
 const auto record=pending.buckets()[bucket][index].record;
 handles.bind(record.handle,reinterpret_cast<uintptr_t>(element));setRecoveredReferenceElementHandleCandidate(*element,record.handle);transferRecoveredReferenceRecordCandidate(element->record,record);
 handles.retire(current);frameworkFlags|=0x1000u;live.insert(record.handle,resolve);pending.erase((bucket<<16)|index);return record.handle;
}

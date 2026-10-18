#pragma once
#include "reference_handle_table.h"
#include "pending_reference_registry.h"
//1035B7B0: live identity first, pending identity next, allocation last.
template<class FindLive,class Notify>
uint32_t createRecoveredPendingIdentityHandleCandidate(const RecoveredPendingIdentityCandidate&identity,RecoveredReferenceHandleTableCandidate&handles,RecoveredPendingReferenceRegistryCandidate&pending,uint32_t&frameworkFlags,FindLive findLive,Notify notify){
 if(uint32_t live=findLive(identity);live!=0xffffffffu)return live;
 uint32_t bucket=0,index=0;if(pending.find(identity,bucket,index))return pending.buckets()[bucket][index].record.handle;
 frameworkFlags|=0x1000u;
 RecoveredPendingReferenceEntryCandidate entry{};entry.identity=identity;entry.record.handle=handles.allocate();
 pending.findOrAppend(entry);notify(entry.record.handle);return entry.record.handle;
}

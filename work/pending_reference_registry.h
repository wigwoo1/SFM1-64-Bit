#pragma once
#include <vector>
#include <stdexcept>
#include "pending_reference_identity.h"
#include "pending_reference_identity_lookup.h"
#include "pending_reference_find_or_append.h"
#include "pending_reference_entry.h"
#include "pending_reference_fast_remove.h"
#include "reference_record_lookup.h"
//Non-owning record pointers; allocation and runtime notification remain caller responsibilities.
class RecoveredPendingReferenceRegistryCandidate{
 std::vector<std::vector<RecoveredPendingReferenceEntryCandidate>> values{16};
public:
 const auto&buckets()const{return values;}
 template<class Resolve>
 RecoveredTransferReferenceRecordCandidate* lookupRecord(uint32_t handle,Resolve resolve,RecoveredPendingReferenceEntryCandidate**pending=nullptr){
  return lookupRecoveredReferenceRecordInBucketsCandidate<RecoveredTransferReferenceRecordCandidate>(handle,resolve,values,[](const auto&e){return e.record.handle;},[](auto&e)->RecoveredTransferReferenceRecordCandidate&{return e.record;},pending);
 }
 bool find(const RecoveredPendingIdentityCandidate&identity,uint32_t&bucket,uint32_t&index)const{
  return recoveredPendingIdentityLookupCandidate(identity,true,15,16,bucket,index,recoveredPendingIdentityHashCandidate,[&](uint32_t b){return uint32_t(values[b].size());},[&](uint32_t b,uint32_t i)->const auto&{return values[b][i].identity;},recoveredPendingIdentityEqualCandidate);
 }
 uint32_t findOrAppend(const RecoveredPendingReferenceEntryCandidate&entry){
  return recoveredPendingFindOrAppendCandidate(entry,[&](const auto&e,uint32_t&b,uint32_t&i){return find(e.identity,b,i);},[&](uint32_t b){return uint32_t(values[b].size());},[&](uint32_t b,uint32_t i,const auto&e){
   if(i>=0xffffu)throw std::length_error("pending bucket index capacity exceeded");
   RecoveredPendingReferenceEntryCandidate fresh{};constructRecoveredPendingReferenceEntryCandidate(fresh,e);values[b].push_back(fresh);return i;
  });
 }
 bool erase(uint32_t composite){return removeRecoveredPendingReferenceCandidate(values,composite);}
};

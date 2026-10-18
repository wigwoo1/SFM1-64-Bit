#pragma once
#include <cstdint>
#include <vector>
template<class Record>struct RecoveredPendingReferenceCandidate{uint32_t handle;Record record;};
//103596D0: live record first, then pending buckets in ascending bucket/index order.
template<class Record,class Entry,class Resolve,class GetHandle,class GetRecord>
Record* lookupRecoveredReferenceRecordInBucketsCandidate(uint32_t handle,Resolve resolve,std::vector<std::vector<Entry>>&buckets,GetHandle getHandle,GetRecord getRecord,Entry**pending=nullptr){
 if(pending)*pending=nullptr;
 if(auto*live=resolve(handle))return live;
 for(auto&bucket:buckets)for(auto&entry:bucket)if(getHandle(entry)==handle){if(pending)*pending=&entry;return &getRecord(entry);}
 return nullptr;
}
template<class Record,class Resolve>
Record* lookupRecoveredReferenceRecordCandidate(uint32_t handle,Resolve resolve,std::vector<std::vector<RecoveredPendingReferenceCandidate<Record>>>&buckets,RecoveredPendingReferenceCandidate<Record>**pending=nullptr){
 return lookupRecoveredReferenceRecordInBucketsCandidate<Record>(handle,resolve,buckets,[](const auto&e){return e.handle;},[](auto&e)->Record&{return e.record;},pending);
}

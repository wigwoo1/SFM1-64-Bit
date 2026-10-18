#pragma once
#include <vector>
#include <cstdint>
//10356F30/10354DA0: replace removed entry with last, then shrink. No list cleanup.
// Entry must have non-owning/copyable reference fields during pending->live transfer.
template<class Entry>
bool removeRecoveredPendingReferenceCandidate(std::vector<std::vector<Entry>>&buckets,uint32_t index){
 const auto bucket=index>>16,entry=index&0xffffu;if(bucket>=buckets.size()||entry>=buckets[bucket].size())return false;
 auto&values=buckets[bucket];if(entry+1!=values.size())values[entry]=values.back();values.pop_back();return true;
}

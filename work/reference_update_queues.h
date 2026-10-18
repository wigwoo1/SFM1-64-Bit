#pragma once
#include "reference_free_ring_capacity.h"
#include <vector>
struct RecoveredReferenceUpdateQueueCandidate {
 std::vector<uint32_t> handles;uint32_t storageCapacity=0;int32_t growSize=0;
 void append(uint32_t handle){if(handles.size()>=storageCapacity){auto n=recoveredReferenceStorageCapacityCandidate(storageCapacity,growSize,uint32_t(handles.size()+1-storageCapacity),8);if(n<=handles.size())throw std::length_error("external queue has no room");handles.reserve(n);storageCapacity=n;}handles.push_back(handle);}
};
// Framework offsets64,78,8C: plain append, duplicates/invalid handles retained.
struct RecoveredReferenceUpdateQueuesCandidate {RecoveredReferenceUpdateQueueCandidate registration,dirty,cache;};

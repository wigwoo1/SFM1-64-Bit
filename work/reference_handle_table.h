#pragma once
#include "reference_handle_allocation.h"
#include "reference_handle_retirement.h"
#include "reference_handle_resolution.h"
#include "reference_free_ring.h"
//Internal table; retirement requires caller-validated ownership.
class RecoveredReferenceHandleTableCandidate{
 std::vector<RecoveredReferenceHandleSlotCandidate> slotValues;
 std::vector<uint32_t> freeValues;
 uint32_t activeCount=0;
 int32_t freeHead=-1,freeTail=-1;
public:
 explicit RecoveredReferenceHandleTableCandidate(size_t capacity=0):freeValues(capacity){}
 uint32_t allocate(){return allocateRecoveredReferenceHandleCandidate(slotValues,activeCount,freeValues,freeHead,freeTail);}
 void bind(uint32_t handle,uintptr_t element){bindRecoveredReferenceHandleCandidate(handle,element,slotValues,activeCount);}
 uintptr_t resolve(uint32_t handle)const{return resolveRecoveredReferenceHandleCandidate(handle,slotValues);}
 void retire(uint32_t handle){retireRecoveredReferenceHandleCandidate(handle,slotValues,activeCount,[&](uint32_t index){appendRecoveredReferenceFreeRingCandidate(freeValues,freeHead,freeTail,index);});}
 const auto&slots()const{return slotValues;}
 const auto&freeRing()const{return freeValues;}
 uint32_t active()const{return activeCount;}
 int32_t head()const{return freeHead;}
 int32_t tail()const{return freeTail;}
};

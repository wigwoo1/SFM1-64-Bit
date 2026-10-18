#pragma once
#include <stdexcept>
#include "reference_handle_binding.h"
#include "reference_free_ring_capacity.h"
//1035AB20: deterministic bit31=0; native reads that bit from an uninitialized local.
inline uint32_t allocateRecoveredReferenceHandleCandidate(std::vector<RecoveredReferenceHandleSlotCandidate>&slots,uint32_t&active,const std::vector<uint32_t>&freeRing,int32_t&head,int32_t&tail){
 uint32_t index;
 if(head!=-1){
  if(head<0||tail<0||size_t(head)>=freeRing.size()||size_t(tail)>=freeRing.size())throw std::out_of_range("invalid free ring");
  index=freeRing[size_t(head)];
  if(head==tail)head=tail=-1;else head=size_t(head)+1==freeRing.size()?0:head+1;
  if(index>=slots.size())throw std::out_of_range("invalid free slot");
 }else{index=uint32_t(slots.size());if(slots.size()==slots.capacity())slots.reserve(recoveredReferenceSlotCapacityCandidate(uint32_t(slots.capacity()),0,1));slots.push_back({});}
 auto&slot=slots[index];slot.state&=0x7fffffffu;slot.element=0;++active;
 if(index>0x3fffffu)return 0xffffffffu;
 return ((slot.state&0x1ffu)<<22)|index;
}

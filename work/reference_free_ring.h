#pragma once
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <algorithm>
#include "reference_free_ring_capacity.h"
//10354C10 queue placement; replacement doubles capacity when full.
inline void appendRecoveredReferenceFreeRingCandidate(std::vector<uint32_t>&ring,int32_t&head,int32_t&tail,uint32_t value){
 if(tail==-1){if(ring.empty())ring.resize(1);head=tail=0;ring[0]=value;return;}
 if(head<0||tail<0||size_t(head)>=ring.size()||size_t(tail)>=ring.size())throw std::out_of_range("invalid ring cursors");
 size_t next=size_t(tail)+1==ring.size()?0:size_t(tail)+1;
 if(next==size_t(head)){
  const size_t old=ring.size();ring.resize(recoveredReferenceFreeRingCapacityCandidate(uint32_t(old),0,1));
  next=size_t(tail)+1==ring.size()?0:size_t(tail)+1;
  if(head){std::copy_backward(ring.begin()+head,ring.begin()+old,ring.end());head+=int32_t(ring.size()-old);}
 }
 tail=int32_t(next);ring[next]=value;
}

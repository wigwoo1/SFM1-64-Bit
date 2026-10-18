#pragma once
#include "model_render_record_allocate_candidate.h"
//100D32E0 capacity calculation for the usable pool domain:
// capacity0..65535, positive increment1..65536, positive growth<=65535.
// Extreme signed32 overflow outside that domain remains unverified.
inline bool modelRenderGrowthCapacityCandidate(int32_t capacity,int32_t growth,int32_t additional,int32_t& result){
 if(growth<0)return false;
 int32_t desired=capacity+additional,selected;
 if(growth)selected=((desired-1)/growth+1)*growth;
 else{selected=capacity?capacity:1;while(selected<desired)selected*=2;}
 if(int32_t(uint16_t(selected))<desired){
  if(uint16_t(selected)==0&&desired<=65535)--selected;
  else{if(int32_t(uint16_t(desired))!=desired)return false;do{selected=(selected+desired)/2;}while(int32_t(uint16_t(selected))<desired);}
 }
 result=selected;return true;
}
// Owned storage callback receives element count, not native108h byte size.
// Commit-before-allocation and unconditional null-result assignment retained.
template<class Allocate>
inline void modelRenderGrowCandidate(EngineShadowPoolStateCandidate& pool,int32_t growth,int32_t additional,Allocate allocate){
 int32_t selected;if(!modelRenderGrowthCapacityCandidate(pool.capacity,growth,additional,selected))return;
 auto* old=pool.links;pool.capacity=selected;pool.links=allocate(old,selected);
}

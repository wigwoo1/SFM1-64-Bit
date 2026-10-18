#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>
struct ElementHandleSlot{uint32_t metadata=0;uintptr_t object=0;};
struct ElementHandleReleaseTable{
 uint32_t liveCount=0;std::vector<ElementHandleSlot> slots;std::vector<uint32_t> reusable;
 uint32_t allocate(){
  uint32_t index;
  if(reusable.empty()){if(slots.size()>0x3fffff)throw std::runtime_error("element handle index exhausted");index=uint32_t(slots.size());slots.push_back({});}
  else {index=reusable.front();reusable.erase(reusable.begin());}
  auto&slot=slots.at(index);slot.metadata&=0x7fffffffu;slot.object=0;++liveCount;
  return index|((slot.metadata&0x1ff)<<22);
 }
 // 0x10354AD0 can reactivate a generation-matching free slot.
 bool publish(uint32_t handle,uintptr_t object){
  if(handle==0xffffffff)return false;uint32_t index=handle&0x3fffff;
  if(index>=slots.size())return false;auto&slot=slots[index];
  if(((handle>>22)&0x1ff)!=(slot.metadata&0x7fffffffu))return false;
  if(slot.metadata&0x80000000u){++liveCount;slot.metadata&=0x7fffffffu;}
  slot.object=object;return true;
 }
 // Public datamodel lookup 0x10356C00 validates generation/free state.
 uintptr_t resolve(uint32_t handle)const{
  if(handle==0xffffffff)return 0;uint32_t index=handle&0x3fffff;
  if(index>=slots.size())return 0;const auto&slot=slots[index];
  if(((handle>>22)&0x1ff)!=(slot.metadata&0x7fffffffu)||(slot.metadata&0x80000000u))return 0;
  return slot.object;
 }
 // Native low-level helper 0x10356E70 accepts index bits only. Caller
 // validates generation first; do not expose as a safe public destroy API.
 bool releaseIndex(uint32_t handle){
  uint32_t index=handle&0x3fffff;if(index>=slots.size())return false;
  auto&slot=slots[index];slot.metadata=(slot.metadata&0x80000000u)|((slot.metadata+1)&0x7fffffffu);
  if(!(slot.metadata&0x80000000u)){slot.metadata|=0x80000000u;--liveCount;}
  slot.object=0;if((slot.metadata&0x7fffffffu)<0x1ff)reusable.push_back(index);return true;
 }
};

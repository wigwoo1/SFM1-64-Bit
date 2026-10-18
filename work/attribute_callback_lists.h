#pragma once
#include <cstdint>
#include <list>
#include <vector>
struct AttributeCallbackRecord{uintptr_t callback=0,attribute=0;};
struct GeneralCallbackRecord{uintptr_t callback=0;uint32_t mask=0;};
// Addresses are pointer-width identities; native lists are +0x48 and +0x4C.
struct AttributeCallbackLists{
 std::list<AttributeCallbackRecord> waiting,prepared;
 template<class Queue> void prepare(uintptr_t attribute,bool suppressed,uint32_t ownerHandle,Queue queue){
  if(suppressed)return;
  for(auto it=waiting.begin();it!=waiting.end();){auto current=it++;if(current->attribute==attribute)prepared.splice(prepared.begin(),waiting,current);}
  queue(ownerHandle);
 }
 // Attribute portion of native 0x103603F0: snapshot, restore list, dispatch.
 template<class Before,class Dispatch> void processPreparedAfter(Before before,Dispatch dispatch){
  std::vector<AttributeCallbackRecord> snapshot(prepared.begin(),prepared.end());
  prepared.splice(prepared.end(),waiting);waiting.swap(prepared);
  before();
  for(auto record:snapshot)dispatch(record.callback,record.attribute);
 }
 template<class Dispatch> void processPrepared(Dispatch dispatch){processPreparedAfter([](){},dispatch);}
 template<class GeneralDispatch,class AttributeDispatch> void process(uint8_t ownerState,uint32_t&ownerMask,uintptr_t owner,const std::list<GeneralCallbackRecord>&general,GeneralDispatch generalDispatch,AttributeDispatch attributeDispatch){
  std::vector<uintptr_t> snapshot;
  if(ownerState&0x10)for(auto record:general)if((record.mask&ownerMask&3)==record.mask)snapshot.push_back(record.callback);
  processPreparedAfter([&](){for(auto callback:snapshot)generalDispatch(callback,owner,ownerMask&3);},attributeDispatch);
 }
};

#pragma once
#include <d3d9.h>
#include <cassert>
#include "studio_scalar_constant_submit_candidate.h"
#include "native_constant_storage_initialize_candidate.h"
static_assert(sizeof(BOOL)==4 && sizeof(int)==4,"Recovered scalar register widths");
inline unsigned studioScalarConstantBufferIdCandidate(unsigned ownerSlot){
 switch(ownerSlot){case 41:return 4;case 42:return 5;case 43:return 1;case 44:return 2;default:assert(false);return 0;}
}
template<class Device,class Lifecycle>
struct NativeScalarConstantBackendCandidateT{
 Device*& device;uint32_t& queued;StudioDeviceQueueStateCandidate& queue;Lifecycle& lifecycle;
 uint32_t* primary(unsigned slot){return lifecycle.cache(studioScalarConstantBufferIdCandidate(slot));}
 uint32_t* secondary(unsigned slot){return lifecycle.staging(studioScalarConstantBufferIdCandidate(slot));}
 bool queueEnabled(){return queued!=0;}auto& queueState(){return queue;}
 void refill(StudioDeviceQueueStateCandidate& state){lifecycle.refill(state);}
 void direct(unsigned slot,uint32_t start,const uint32_t* source,uint32_t count){
  switch(slot){
   case 98:device->SetVertexShaderConstantB(start,reinterpret_cast<const BOOL*>(source),count);break;
   case 96:device->SetVertexShaderConstantI(start,reinterpret_cast<const int*>(source),count);break;
   case 113:device->SetPixelShaderConstantB(start,reinterpret_cast<const BOOL*>(source),count);break;
   case 111:device->SetPixelShaderConstantI(start,reinterpret_cast<const int*>(source),count);break;
   default:assert(false);
  }
 }
};
template<class Device,class Lifecycle>
struct NativeCompleteConstantInitializationServicesCandidateT:NativeConstantStorageInitializeServicesCandidateT<Device,Lifecycle>{
 using Base=NativeConstantStorageInitializeServicesCandidateT<Device,Lifecycle>;
 NativeCompleteConstantInitializationServicesCandidateT(Device*& device,uint32_t& queued,StudioDeviceQueueStateCandidate& queue,Lifecycle& lifecycle):Base{device,queued,queue,lifecycle}{}
 void submitSubobject(unsigned slot,uint32_t start,uint32_t* source,uint32_t count,uint32_t flag){
  NativeScalarConstantBackendCandidateT<Device,Lifecycle> backend{this->device,this->queued,this->queue,this->lifecycle};
  assert(slot>=41&&slot<=44);
  studioScalarConstantSubmitCandidate(studioScalarConstantSubmissionsCandidate[slot-41],start,source,count,flag,backend);
 }
};

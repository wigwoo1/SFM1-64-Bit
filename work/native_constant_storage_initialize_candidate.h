#pragma once
#include <d3d9.h>
#include <cstring>
#include "studio_constant_storage_initialize_candidate.h"
#include "studio_device_queue_refill_candidate.h"
// Borrows live device/queue state. Lifecycle owns valid buffers and unresolved
// Source subobject methods; HRESULT handling follows the recovered low-level code.
template<class Device,class Lifecycle>
struct NativeConstantStorageInitializeServicesCandidateT{
 Device*& device;uint32_t& queued;StudioDeviceQueueStateCandidate& queue;Lifecycle& lifecycle;
 auto capability(){return lifecycle.capability();}
 uint32_t* staging(unsigned id){return lifecycle.staging(id);}
 uint32_t* cache(unsigned id){return lifecycle.cache(id);}
 void copy(uint32_t* destination,const uint32_t* source,uint32_t bytes){std::memmove(destination,source,bytes);}
 void submitSubobject(unsigned slot,uint32_t start,uint32_t* data,uint32_t count,uint32_t flag){lifecycle.submitSubobject(slot,start,data,count,flag);}
 void submitFloat(bool vertex,uint32_t start,uint32_t count,uint32_t* data){
  if(!queued){
   const auto* floats=reinterpret_cast<const float*>(data);
   if(vertex)device->SetVertexShaderConstantF(start,floats,count);
   else device->SetPixelShaderConstantF(start,floats,count);
   return;
  }
  const uint32_t words=count*4+3;
  if(queue.remaining<words)lifecycle.refill(queue);
  queue.remaining-=words;
  queue.cursor[0]=vertex?10u:7u;queue.cursor[1]=start;queue.cursor[2]=count;
  std::memmove(queue.cursor+3,data,count<<4);
  // Original advances by a separately wrapping byte count, not host size_t.
  queue.cursor+=static_cast<uint32_t>((count<<4)+12)/4;
 }
};

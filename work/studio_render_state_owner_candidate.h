#pragma once
#include "studio_shader_render_state_submit_candidate.h"
#include "studio_device_queue_refill_candidate.h"
// Borrowed render-state cache and normalized queue are supplied by the owner.
// Services controls suppression, flushing, direct submission and block lifetime.
// This preserves scalar opcode1 transport; it does not establish concurrency.
struct StudioRenderStateOwnerCandidate {
 uint32_t* cache=nullptr;
 StudioDeviceQueueStateCandidate* queue=nullptr;
};
template<class Services>struct StudioRenderStateOwnerAdapterCandidate {
 StudioRenderStateOwnerCandidate& owner;Services& services;
 unsigned suppressed(){return services.suppressed();}
 void flush(){services.flush();}
 bool queueEnabled(){return services.queueEnabled();}
 void directState(unsigned key,unsigned value){services.directState(key,value);}
 void publish(uint32_t* block){services.publish(block);}
 uint32_t* acquire(unsigned header){return services.acquire(header);}
 void queueState(unsigned key,unsigned value){
  auto& state=*owner.queue;
  if(state.remaining<3)studioDeviceQueueRefillCandidate(state,*this);
  state.remaining-=3;
  state.cursor[0]=1;state.cursor[1]=key;state.cursor[2]=value;
  state.cursor+=3;
 }
};
template<class Services>void studioRenderStateOwnerSubmitCandidate(
 StudioRenderStateOwnerCandidate& owner,uint32_t key,uint32_t value,uint32_t flush,Services& services){
 StudioRenderStateOwnerAdapterCandidate<Services> adapter{owner,services};
 studioShaderRenderStateSubmitCandidate(owner.cache[key],key,value,flush,adapter);
}

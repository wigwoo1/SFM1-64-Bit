#pragma once
#include <cstdint>
// Normalized x64 state, not the native32 byte layout. Owner supplies storage,
// allocator and publisher; no concurrency guarantees are inferred here.
struct StudioDeviceQueueStateCandidate{
 uint32_t* block=nullptr;uint32_t* cursor=nullptr;uint32_t remaining=0;
};
template<class Ops>void studioDeviceQueueRefillCandidate(StudioDeviceQueueStateCandidate& state,Ops& ops){
 if(state.block){
  if(state.cursor==state.block+1)return;
  *state.cursor=0;
  *state.block=2;
  ops.publish(state.block);
 }
 state.block=ops.acquire(1);
 state.cursor=state.block+1;
 state.remaining=4095;
}

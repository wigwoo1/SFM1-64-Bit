#pragma once
#include "studio_device_queue_refill_candidate.h"
#include <array>
#include <cstring>
// Recovered single-register pixel tail only. Native HRESULT is not used to
// gate later cache publication; Ops can record device results independently.
template<class Ops>void studioPixelConstantSubmitCandidate(uint32_t index,const std::array<uint32_t,4>& local,Ops& ops){
 if(!ops.queueEnabled()){ops.directPixel(index,1,local.data());return;}
 auto& state=ops.queueState();
 if(state.remaining<7)studioDeviceQueueRefillCandidate(state,ops);
 state.remaining-=7;
 state.cursor[0]=7;state.cursor[1]=index;state.cursor[2]=1;
 std::memcpy(state.cursor+3,local.data(),16);
 state.cursor+=7;
}
template<class Ops>void studioPixelConstantSubmitCandidate(const std::array<uint32_t,4>& local,Ops& ops){
 studioPixelConstantSubmitCandidate(29,local,ops);
}

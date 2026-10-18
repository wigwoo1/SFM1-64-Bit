#pragma once
#include "studio_device_queue_refill_candidate.h"
#include <array>
#include <cstring>
// Recovered slot264 single-register vertex submission: opcode10 or COM94.
// Backend owns queue allocation/publication and direct device submission.
template<class Backend>void studioVertexConstantSubmitCandidate(
 uint32_t index,const std::array<uint32_t,4>& local,Backend& backend){
 if(!backend.queueEnabled()){backend.directVertex(index,1,local.data());return;}
 auto& state=backend.queueState();
 if(state.remaining<7)studioDeviceQueueRefillCandidate(state,backend);
 state.remaining-=7;
 state.cursor[0]=10;state.cursor[1]=index;state.cursor[2]=1;
 std::memcpy(state.cursor+3,local.data(),16);
 state.cursor+=7;
}

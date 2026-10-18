#pragma once
#include "studio_device_queue_refill_candidate.h"
// Normalized owner policy. Completion reads/polling belong to the service;
// this function does not confer synchronization on shared pool/ring storage.
template<class Ops> void studioDeviceQueueFlushCandidate(bool enabled,StudioDeviceQueueStateCandidate& state,Ops& ops){
 if(!enabled)return;
 studioDeviceQueueRefillCandidate(state,ops);
 while(ops.produced()!=ops.completed())ops.pollCompletion();
}

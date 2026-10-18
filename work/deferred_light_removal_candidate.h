#pragma once
#include <cstdint>
#include <vector>
#include <cstddef>
// Recovered100BF0A0 logical queue behavior; owned x64 storage.
// Allocation and concurrency policy differ until original lifecycle is recovered.
inline void queueDeferredLightRemovalCandidate(std::vector<uint16_t>& queue,uint32_t argument){
 const auto handle=uint16_t(argument);
 for(const auto existing:queue)if(existing==handle)return;
 queue.push_back(handle);
}
// Normal-return drain100BAB40. Callback owns removal and deferral policy.
// Re-read current queue size after callbacks; do not snapshot pending work.
template<class Remove>
void drainDeferredLightRemovalCandidate(std::vector<uint16_t>& queue,Remove&& remove){
 for(std::size_t index=0;index<queue.size();++index){
  const uint16_t handle=queue[index];
  remove(handle);
 }
 queue.clear();
}
// Recovered normal-return branch100C357E: enable, evaluate selected path,
// disable before draining. Does not restore an earlier deferral flag.
template<class EvaluateA,class EvaluateB,class Remove>
void evaluateWithDeferredLightRemovalCandidate(uint8_t& deferred,std::vector<uint16_t>& queue,
 bool alternate,EvaluateA&& evaluateA,EvaluateB&& evaluateB,Remove&& remove){
 deferred=1;
 if(alternate)evaluateA();else evaluateB();
 deferred=0;
 drainDeferredLightRemovalCandidate(queue,remove);
}

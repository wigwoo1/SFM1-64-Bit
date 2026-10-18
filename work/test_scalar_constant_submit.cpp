#include "studio_scalar_constant_submit_candidate.h"
#include "studio_device_queue_refill_candidate.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <initializer_list>
struct Backend{
 std::array<uint32_t,24> first{},second{},replacement{};std::array<uint32_t,64> block{},next{};
 StudioDeviceQueueStateCandidate state{block.data(),block.data()+1,3};bool queued,mutate;
 unsigned directs=0,refills=0,slot=0;unsigned expectedDevice=0;uint32_t* liveFirst=first.data();
 Backend(bool q,bool m):queued(q),mutate(m){first.fill(0x11111111);second.fill(0x22222222);replacement.fill(0x33333333);}
 uint32_t* primary(unsigned){return liveFirst;}uint32_t* secondary(unsigned){return second.data();}
 bool queueEnabled(){return queued;}auto& queueState(){return state;}
 void refill(StudioDeviceQueueStateCandidate& s){++refills;s={next.data(),next.data()+1,63};}
 void direct(unsigned deviceSlot,uint32_t start,const uint32_t*,uint32_t){assert(deviceSlot==expectedDevice&&start==1);++directs;slot=deviceSlot;if(mutate)liveFirst=replacement.data();}
};
int main(){unsigned cases=0;for(auto kind:studioScalarConstantSubmissionsCandidate)for(bool queued:{false,true})for(uint32_t force:{0u,1u,256u})for(bool same:{false,true})for(uint32_t count:{0u,1u,2u}){
 Backend backend(queued,!queued);backend.expectedDevice=kind.deviceSlot;std::array<uint32_t,8> source{};source.fill(0x76543210);const unsigned words=count*kind.wordsPerRegister;const unsigned offset=kind.wordsPerRegister;
 if(same)std::memcpy(backend.second.data()+offset,source.data(),words*4);
 const bool expected=(force&255)!=0||(!same&&count!=0);const bool submitted=studioScalarConstantSubmitCandidate(kind,1,source.data(),count,force,backend);assert(submitted==expected);assert(backend.directs==(!queued&&expected?1u:0u));assert(backend.refills==(queued&&expected&&count!=0?1u:0u));
 if(expected){assert(std::memcmp(backend.liveFirst+offset,source.data(),words*4)==0);assert(std::memcmp(backend.second.data()+offset,source.data(),words*4)==0);if(queued){auto* output=count?backend.next.data():backend.block.data();assert(output[1]==kind.opcode&&output[2]==1&&output[3]==count);assert(std::memcmp(output+4,source.data(),words*4)==0);assert(backend.state.cursor==output+4+words);}}
 if(!queued&&expected&&count)assert(backend.first[offset]==0x11111111);++cases;
 }std::printf("scalar_constant_submit_cases=%u passed\n",cases);}

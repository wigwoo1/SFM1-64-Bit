#include "studio_default_render_state_owner_candidate.h"
#include <array>
#include <cassert>
#include <vector>
#include <cstdio>
struct Capability{unsigned flags,vendor;};
struct Services{Capability caps;bool queued,suppress;std::vector<uint32_t> direct;unsigned suppressed(){return suppress?1u:0u;}bool queueEnabled(){return queued;}void directState(unsigned key,unsigned value){direct.push_back(key);direct.push_back(value);}void flush(){assert(false);}void publish(uint32_t*){assert(false);}uint32_t* acquire(unsigned){return nullptr;}Capability* capability(){return &caps;}unsigned flags(Capability* value){return value->flags;}unsigned vendor(Capability* value){return value->vendor;}};
int main(){unsigned cases=0;for(unsigned flags:{0u,0x10u,0x20u,0x30u})for(unsigned vendor:{0x1002u,0x10deu})for(bool queued:{false,true})for(bool suppress:{false,true}){
 std::array<uint32_t,256> cache{};cache.fill(99);std::array<uint32_t,512> block{};StudioDeviceQueueStateCandidate queue{block.data(),block.data(),512};StudioRenderStateOwnerCandidate owner{cache.data(),&queue};Services services{{flags,vendor},queued,suppress};studioDefaultRenderStateOwnerCandidate(owner,services);
 std::array<uint32_t,256> expectedCache{};expectedCache.fill(99);std::vector<uint32_t> expectedDirect,expectedQueue;
 if(!suppress)for(unsigned i=0;i<91;++i){if(i==42&&(!(flags&0x10)||vendor!=0x1002))continue;if(i==43&&!(flags&0x20))continue;const auto entry=studioDefaultRenderStateEntriesCandidate[i];assert(entry.state<256);expectedCache[entry.state]=entry.value;expectedDirect.push_back(entry.state);expectedDirect.push_back(entry.value);expectedQueue.push_back(1);expectedQueue.push_back(entry.state);expectedQueue.push_back(entry.value);}
 assert(cache==expectedCache);if(queued){assert(services.direct.empty());assert(queue.cursor==block.data()+expectedQueue.size()&&queue.remaining==512-expectedQueue.size());for(unsigned i=0;i<expectedQueue.size();++i)assert(block[i]==expectedQueue[i]);}else{assert(services.direct==expectedDirect&&queue.cursor==block.data()&&queue.remaining==512);}++cases;
 }std::printf("default_render_state_owner_cases=%u passed\n",cases);}

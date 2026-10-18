#include "studio_constant_storage_initialize_candidate.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <vector>
struct Capability{std::array<uint32_t,6> counts;uint32_t count(unsigned id){return counts[id];}};
struct Services{
 Capability original,replacement;Capability* active=&original;
 std::array<std::array<uint32_t,16>,6> buffers,caches;
 std::vector<unsigned> trace;bool swap=false;unsigned floatCalls=0,copyCalls=0;
 Services(uint32_t count,bool mutate):original{{count,count,count,count,count,count}},replacement{{1,1,1,1,1,1}},swap(mutate){for(auto& b:buffers)b.fill(0xdeadbeef);for(auto& c:caches)c.fill(0xcafebabe);}
 Capability* capability(){return active;}uint32_t* staging(unsigned id){return buffers[id].data();}uint32_t* cache(unsigned id){return caches[id].data();}
 void submitFloat(bool vertex,uint32_t start,uint32_t count,uint32_t* data){assert(start==0&&count==original.counts[vertex?3:0]&&data==staging(vertex?3:0));for(uint32_t i=0;i<count*4;++i)assert(data[i]==0);trace.push_back(vertex?10:7);++floatCalls;if(swap)active=&replacement;}
 void copy(uint32_t* destination,uint32_t* source,uint32_t bytes){assert(bytes==original.counts[trace.front()==10?3:0]*16);if(destination!=source)std::memcpy(destination,source,bytes);++copyCalls;}
 void submitSubobject(unsigned slot,uint32_t start,uint32_t* data,uint32_t count,uint32_t flag){assert(start==0&&flag==1);const unsigned id=slot==43?1:slot==44?2:slot==41?4:5;assert(data==staging(id)&&count==active->counts[id]);for(uint32_t i=0;i<count*(id==1||id==4?1:4);++i)assert(data[i]==0);trace.push_back(slot);}
};
int main(){unsigned cases=0;for(bool vertex:{false,true})for(uint32_t count:{0u,1u,4u})for(bool swap:{false,true}){Services services(count,swap);studioConstantStorageInitializeGroupCandidate(vertex,services);assert(services.floatCalls==1&&services.copyCalls==2);const bool submits=vertex||swap||count!=0;assert(services.trace.size()==(submits?3u:1u));assert(services.trace[0]==(vertex?10u:7u));if(submits){assert(services.trace[1]==(vertex?41u:43u)&&services.trace[2]==(vertex?42u:44u));}++cases;}assert(!studioConstantCountPositiveCandidate(0x80000000)&&!studioConstantCountPositiveCandidate(0xffffffff));std::printf("constant_initialization_cases=%u passed\n",cases);}

#include "model_render_record_grow_candidate.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
#define check(ok) do{if(!(ok)){std::fprintf(stderr,"check failed line %d\n",__LINE__);std::abort();}}while(false)
int main(){unsigned cases=0;int growths[7]={0,1,2,3,127,1024,65535};
 for(int growth:growths)for(int capacity=0;capacity<=65535;++capacity){int result=-77;bool allocated=modelRenderGrowthCapacityCandidate(capacity,growth,1,result);check(allocated==(capacity<65535));
  if(allocated){check(result>=capacity+1&&result<=65535);if(growth==0){int expected=capacity?capacity:1;while(expected<capacity+1)expected*=2;if(expected<=65535)check(result==expected);else if((expected&65535)==0)check(result==65535);}if(growth==1)check(result==capacity+1);}
  else check(result==-77);++cases;
 }
 int value=9;check(!modelRenderGrowthCapacityCandidate(8,-1,1,value)&&value==9);check(modelRenderGrowthCapacityCandidate(60000,20000,1,value)&&value==65000);check(modelRenderGrowthCapacityCandidate(0,0,65535,value)&&value==65535);check(!modelRenderGrowthCapacityCandidate(0,0,65536,value));
 ModelRenderRecordAllocationCandidate state;state.pool.maximumIndex=0xffff;std::vector<EngineShadowPoolLinkCandidate> storage;unsigned allocations=0,warnings=0;
 auto grow=[&](auto& pool,int increment){modelRenderGrowCandidate(pool,0,increment,[&](auto* old,int count){check(old==storage.data()||storage.empty());check(pool.capacity==count);storage.resize(count);++allocations;return storage.data();});};
 for(unsigned i=0;i<65535;++i)check(modelRenderRecordAllocateCandidate(state,0,grow,[&](bool){++warnings;})==i);
 check(state.pool.capacity==65535&&allocations==17&&warnings==0&&state.cachedStorage==storage.data());check(modelRenderRecordAllocateCandidate(state,0,grow,[&](bool range){check(!range);++warnings;})==0xffff&&warnings==1&&allocations==17);
 for(unsigned i=0;i<65535;++i)check(storage[i].previous==i&&storage[i].next==i);
 EngineShadowPoolStateCandidate failed;modelRenderGrowCandidate(failed,0,1,[&](auto*,int count)->EngineShadowPoolLinkCandidate*{check(count==1&&failed.capacity==1);return nullptr;});check(failed.capacity==1&&!failed.links);
 std::printf("PASS: %u growth boundary cases; full65535-index allocation with17 storage growths, exhaustion and commit-before-null semantics. Native growth parity pending.\n",cases);
}



#include "model_render_record_release_candidate.h"
#include "model_render_record_resource_cleanup_candidate.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
void check(bool x){if(!x)std::abort();}
int main(){unsigned releases=0;
 for(unsigned length=1;length<=128;++length){std::vector<EngineShadowPoolLinkCandidate> links(length),old;std::vector<ModelRenderRecordResourceCandidate> resources(length);for(unsigned i=0;i<length;++i){links[i]={i?uint16_t(i-1):uint16_t(0xffff),i+1<length?uint16_t(i+1):uint16_t(0xffff)};resources[i].resource=uintptr_t(0x1234567800000000ull)+i;}
  ModelRenderRecordAllocationCandidate state;state.pool={links.data(),int32_t(length),0,uint16_t(length-1),uint16_t(length),uint16_t(length-1)};state.freshCount=uint16_t(length);state.cachedStorage=links.data();
  for(unsigned phase=0;phase<2;++phase)for(unsigned index=phase;index<length;index+=2){unsigned order=0;uint32_t argument=0xabcd0000u|index;
   modelRenderRecordReleaseCandidate(state,argument,[&](uint16_t i){check(i==index&&order++==0);return uintptr_t(0xfedcba9800000000ull)+i;},[&](uintptr_t value){check(value==uintptr_t(0xfedcba9800000000ull)+index&&order++==1);},[&](uint32_t key){check(key==argument&&order++==2);},[&](uint32_t key){check(key==argument&&order++==3);modelRenderRecordResourceCleanupCandidate(key,[&](uint16_t i)->ModelRenderRecordResourceCandidate&{return resources[i];},[&](uintptr_t value,int a,int b,int c){check(value==uintptr_t(0x1234567800000000ull)+index&&a==0&&b==0&&c==1);old=links;auto replacement=links;links.swap(replacement);state.pool.links=links.data();});});
   check(order==4&&state.pool.links==links.data()&&resources[index].resource==0&&links[index].previous==index);check(old[index].previous!=index);check(state.freshCount==length&&state.pool.maximumIndex==length-1);++releases;
  }
  check(state.pool.count==0&&state.pool.head==0xffff&&state.pool.tail==0xffff);uint16_t node=state.freeHead;std::vector<bool> seen(length);for(unsigned i=0;i<length;++i){check(node<length&&!seen[node]);seen[node]=true;node=links[node].next;}check(node==0xffff);
 }
 ModelRenderRecordAllocationCandidate invalid;modelRenderRecordReleaseCandidate(invalid,0x1234ffff,[](uint16_t)->uintptr_t{std::abort();},[](uintptr_t){std::abort();},[](uint32_t){std::abort();},[](uint32_t){std::abort();});
 std::printf("PASS: %u ordered model releases, full64-bit service identities, live link-storage replacement, resource cleanup, exact free chains and invalid-index skip. Pair/Studio services remain observers.\n",releases);
}

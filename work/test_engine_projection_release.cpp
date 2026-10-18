#include "engine_projection_release_candidate.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
void check(bool v){if(!v)std::abort();}
int main(){unsigned cases=0;
 for(unsigned length=1;length<=64;++length){std::vector<EngineReceiverLinkCandidate> links(length);std::vector<EngineProjectionAssociationEntryCandidate> entries(length);EngineReceiverAllocationCandidate receivers;receivers.pool={links.data(),int32_t(length),-1,0xffff,0xffff,0};std::vector<EngineShadowPoolNodeCandidate> storage(length);EngineShadowNodeBlockCandidate block{nullptr,int32_t(length),storage.data()};EngineShadowNodePoolCandidate nodes;nodes.first=&block;EngineProjectionAssociationObjectCandidate object{0x800,0xffff};EngineShadowPoolNodeCandidate* head=nullptr;
  for(unsigned i=0;i<length;++i)check(engineProjectionAssociateCandidate(receivers,nodes,object,7,[](auto&,int){check(false);},[](auto&,int){check(false);},[&](uint16_t n){return &entries[n];},[&](uint16_t){return head;},[&](uint16_t,auto* n){head=n;},[&](uint32_t){return i;},[](auto*){return false;},[](int){check(false);})==i);
  std::vector<bool> removed(length);unsigned releases=0,auxStage=0;
  for(unsigned parity=0;parity<2;++parity)for(unsigned i=parity;i<length;i+=2){entries[i].auxiliary=uint16_t(i);
   engineProjectionReleaseCandidate(receivers,nodes,object,uint16_t(i),[&](uint16_t n){return &entries[n];},[&](uint16_t h){check(h==7);return head;},[&](uint16_t h,auto* n){check(h==7);head=n;},[&](uint16_t a){check(a==i&&auxStage==0);auxStage=1;},[&](uint16_t a){check(a==i&&auxStage==1);auxStage=2;},[&](auto* o,uint16_t r){check(o==&object&&r==i&&auxStage==2);++releases;},[](auto*){return false;});auxStage=0;removed[i]=true;check(entries[i].node==nullptr&&entries[i].auxiliary==0xffff&&entries[i].resource==i);
   auto* n=head;uint16_t receiver=object.head;for(int k=int(length)-1;k>=0;--k)if(!removed[k]){check(n==&storage[k]&&receiver==k);n=n->next;receiver=links[receiver].next;}check(!n&&receiver==0xffff);++cases;
  }
  check(releases==length);std::vector<bool> reused(length);for(unsigned i=0;i<length;++i){auto index=engineReceiverAllocateCandidate(receivers,1,[](auto&,int){check(false);},[](bool){check(false);});check(index<length&&!reused[index]);reused[index]=true;}check(receivers.freeHead==0xffff);
 }
 std::printf("PASS: %u association releases, auxiliary/resource order, surviving-list traversal and entry reuse. Native release/resource parity pending.\n",cases);
}

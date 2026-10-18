#include "engine_projection_pair_unlink_candidate.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
void check(bool ok){if(!ok)std::abort();}
int main(){unsigned transitions=0,removedTotal=0;
 for(unsigned length=0;length<=128;++length){std::vector<EngineShadowPoolLinkCandidate> shadowNodes(length),modelNodes(length);std::vector<EngineProjectionPairPayloadCandidate> forward(length),reverse(length);uint16_t shadowHeads[4]={0xffff,0xffff,0xffff,0xffff},modelHeads[3]={0xffff,0xffff,0xffff};std::vector<bool> removed(length);
  for(unsigned i=0;i<length;++i){uint16_t j=uint16_t(length-1-i);unsigned model=i%3,shadow=(i/3)%4;forward[i]={uint16_t(model),j};reverse[j]={uint16_t(shadow),uint16_t(i)};shadowNodes[i]={0xffff,shadowHeads[shadow]};if(shadowHeads[shadow]!=0xffff)shadowNodes[shadowHeads[shadow]].previous=uint16_t(i);shadowHeads[shadow]=uint16_t(i);modelNodes[j]={0xffff,modelHeads[model]};if(modelHeads[model]!=0xffff)modelNodes[modelHeads[model]].previous=j;modelHeads[model]=j;}
  EngineProjectionPairMapCandidate map;map.current={shadowNodes.data(),int32_t(length),0xffff,0xffff,uint16_t(length),uint16_t(length?length-1:0)};map.opposite={modelNodes.data(),int32_t(length),0xffff,0xffff,uint16_t(length),uint16_t(length?length-1:0)};map.payload=forward.data();map.reversePayload=reverse.data();
  const unsigned operations[9]={1,4,0,6,5,2,3,4,1};
  for(unsigned operation:operations){bool model=operation>=4;unsigned group=model?operation-4:operation;
   auto shadowHead=[&](uint32_t key){check(uint16_t(key)<4);return &shadowHeads[uint16_t(key)];};auto modelHead=[&](uint32_t key){check(uint16_t(key)<3);return &modelHeads[uint16_t(key)];};
   if(model)engineProjectionPairUnlinkModelCandidate(map,0xabcd0000u|group,modelHead,shadowHead);else engineProjectionPairUnlinkCandidate(map,0xabcd0000u|group,shadowHead,modelHead);
   for(unsigned i=0;i<length;++i)if(!removed[i]&&(model?i%3==group:(i/3)%4==group)){removed[i]=true;++removedTotal;}
   unsigned alive=0;for(bool r:removed)alive+=!r;check(map.current.count==alive&&map.opposite.count==alive);
   for(unsigned side=0;side<2;++side)for(unsigned g=0;g<(side?3u:4u);++g){uint16_t node=side?modelHeads[g]:shadowHeads[g],previous=0xffff;for(int i=int(length)-1;i>=0;--i)if(!removed[i]&&(side?unsigned(i)%3==g:(unsigned(i)/3)%4==g)){uint16_t expected=side?uint16_t(length-1-unsigned(i)):uint16_t(i);auto& links=side?modelNodes:shadowNodes;check(node==expected&&links[node].previous==previous);previous=node;node=links[node].next;}check(node==0xffff);}
   for(unsigned side=0;side<2;++side){auto& links=side?modelNodes:shadowNodes;uint16_t node=side?map.oppositeFree:map.currentFree;std::vector<bool> seen(length);unsigned freeCount=0;while(node!=0xffff){check(node<length&&!seen[node]);unsigned pair=side?length-1-node:node;check(removed[pair]);seen[node]=true;++freeCount;node=links[node].next;}check(freeCount==length-alive);}
   ++transitions;
  }
  for(bool r:removed)check(r);
 }
 std::printf("PASS: %u mixed-direction cleanup transitions /%u pairs; multiple shadows per model, reversed indices, surviving lists, exact free chains and repeated cleanup checked. Native combined parity pending.\n",transitions,removedTotal);
}

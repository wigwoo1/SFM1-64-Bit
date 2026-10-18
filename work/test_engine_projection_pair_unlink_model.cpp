#include "engine_projection_pair_unlink_candidate.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
void check(bool ok){if(!ok)std::abort();}
int main(){unsigned removedTotal=0;
 for(unsigned length=0;length<=64;++length){std::vector<EngineShadowPoolLinkCandidate> current(length),opposite(length);std::vector<EngineProjectionPairPayloadCandidate> payload(length);uint16_t currentHeads[3]={0xffff,0xffff,0xffff},oppositeHeads[4]={0xffff,0xffff,0xffff,0xffff};
  for(unsigned i=0;i<length;++i){uint16_t j=uint16_t(length-1-i);payload[i]={uint16_t(i%4),j};current[i]={0xffff,currentHeads[i%3]};if(currentHeads[i%3]!=0xffff)current[currentHeads[i%3]].previous=uint16_t(i);currentHeads[i%3]=uint16_t(i);opposite[j]={0xffff,oppositeHeads[i%4]};if(oppositeHeads[i%4]!=0xffff)opposite[oppositeHeads[i%4]].previous=j;oppositeHeads[i%4]=j;}
  EngineProjectionPairMapCandidate map;map.opposite={current.data(),int32_t(length),0xffff,0xffff,uint16_t(length),uint16_t(length?length-1:0)};map.current={opposite.data(),int32_t(length),0xffff,0xffff,uint16_t(length),uint16_t(length?length-1:0)};map.reversePayload=payload.data();std::vector<bool> removed(length);unsigned order[3]={1,0,2};
  for(unsigned group:order){unsigned headReads=0;engineProjectionPairUnlinkModelCandidate(map,group|0xabcd0000u,[&](uint32_t key){check(key==(group|0xabcd0000u));++headReads;return &currentHeads[group];},[&](uint32_t key){check(key<4);return &oppositeHeads[key];});check(headReads==2&&currentHeads[group]==0xffff);
   for(unsigned i=0;i<length;++i)if(i%3==group){removed[i]=true;++removedTotal;}
   for(unsigned g=0;g<3;++g){uint16_t node=currentHeads[g],previous=0xffff;for(int i=int(length)-1;i>=0;--i)if(!removed[i]&&unsigned(i)%3==g){check(node==unsigned(i)&&current[node].previous==previous);previous=node;node=current[node].next;}check(node==0xffff);}
   for(unsigned g=0;g<4;++g){uint16_t node=oppositeHeads[g],previous=0xffff;for(int i=int(length)-1;i>=0;--i)if(!removed[i]&&unsigned(i)%4==g){check(node==length-1-unsigned(i)&&opposite[node].previous==previous);previous=node;node=opposite[node].next;}check(node==0xffff);}
  }
  for(unsigned side=0;side<2;++side){std::vector<bool> seen(length);uint16_t index=side?map.currentFree:map.oppositeFree;unsigned count=0;while(index!=0xffff){check(index<length&&!seen[index]);seen[index]=true;++count;index=(side?opposite:current)[index].next;}check(count==length);}check(map.opposite.count==0&&map.current.count==0);
 }
 std::printf("PASS: %u model-side paired removals across lengths0..64, three source/four opposite lists and both complete free lists. Native parity pending.\n",removedTotal);
}

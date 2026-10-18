#include "engine_projection_enable_stage_candidate.h"
#include <cstdio>
#include <cstdlib>
void check(bool v){if(!v)std::abort();}
struct Entry{unsigned* object;};
int main(){unsigned cases=0;uint32_t enabledValues[4]={0,1,0x100,0x101};
 for(unsigned initial=0;initial<256;++initial)for(uint32_t enabled:enabledValues){
  EngineShadowPoolNodeCandidate nodes[3];unsigned objects[3]={0,1,2};Entry entries[3];for(unsigned i=0;i<3;++i){nodes[i].index=uint16_t(i);nodes[i].next=i<2?nodes+i+1:nullptr;entries[i].object=objects+i;}auto* head=nodes;
  EngineShadowPoolLinkCandidate current[3]={{0xffff,1},{0,2},{1,0xffff}},opposite[3]={{0xffff,1},{0,2},{1,0xffff}};EngineProjectionPairPayloadCandidate payload[3]={{0,0},{0,1},{0,2}};EngineProjectionPairMapCandidate pairs;pairs.current={current,3,0,2,3,2};pairs.opposite={opposite,3,0,2,3,2};pairs.payload=payload;uint16_t sourceHead=0,otherHead=0;
  uint16_t oldFlags=uint16_t(initial),newFlags=0x1230;auto* flags=&oldFlags;unsigned releases=0,pairReads=0,flagReads=0;
  engineProjectionEnableStageCandidate(0xabcd0007,enabled,pairs,[&](uint16_t h){check(h==7);return head;},[&](uint16_t i){return &entries[i];},[&](unsigned* object,uint16_t i){check(object==objects+i&&i==releases);++releases;nodes[i].next=nullptr;},[&](uint16_t h,EngineShadowPoolNodeCandidate* node){check(h==7&&!node);head=node;},[&](uint32_t h){check(h==0xabcd0007&&releases==3);++pairReads;flags=&newFlags;return &sourceHead;},[&](uint32_t k){check(k==0&&releases==3);return &otherHead;},[&](uint16_t h)->uint16_t&{check(h==7);++flagReads;return *flags;});
  check(flagReads==1);
  if(uint8_t(enabled)){check(releases==0&&pairReads==0&&head==nodes&&sourceHead==0&&otherHead==0);check(oldFlags==uint16_t(initial&0xfff7)&&newFlags==0x1230);}
  else{check(releases==3&&pairReads==2&&!head&&sourceHead==0xffff&&otherHead==0xffff);check(oldFlags==initial&&newFlags==0x1238);check(pairs.current.count==0&&pairs.opposite.count==0&&pairs.currentFree==2&&pairs.oppositeFree==2);}++cases;
 }
 std::printf("PASS: %u integrated enable/disable states, low-byte control, cleanup before pair removal and final flag-storage reload. Native resource/runtime integration pending.\n",cases);
}


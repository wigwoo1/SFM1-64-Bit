#include "engine_projection_associate_candidate.h"
#include <cstdio>
#include <cstdlib>
void check(bool ok){if(!ok)std::abort();}
int main(){unsigned cases=0;
 for(unsigned handle=0;handle<65536;++handle)for(unsigned resourceBit=0;resourceBit<2;++resourceBit)for(unsigned failNode=0;failNode<2;++failNode){
  EngineReceiverLinkCandidate links[1];EngineReceiverAllocationCandidate receivers;receivers.pool={links,1,-1,0xffff,0xffff,0};
  EngineShadowPoolNodeCandidate storage[1];EngineShadowNodeBlockCandidate block{nullptr,1,storage};EngineShadowNodePoolCandidate nodes;if(!failNode)nodes.first=&block;
  EngineProjectionAssociationObjectCandidate object{resourceBit?0x800u:0u,0xffff};EngineProjectionAssociationEntryCandidate original[1],replacement[1];auto* entries=original;EngineShadowPoolNodeCandidate* shadowHead=nullptr;unsigned resources=0,warnings[4]{},nodeGrowth=0;
  uint16_t result=engineProjectionAssociateCandidate(receivers,nodes,object,handle|0xabcd0000u,[](auto&,int){std::abort();},[&](auto&,int){++nodeGrowth;},[&](uint16_t i){check(i==0);return entries+i;},[&](uint16_t h){check(h==handle);return shadowHead;},[&](uint16_t h,auto* node){check(h==handle);shadowHead=node;},[&](uint32_t h){check(h==(handle|0xabcd0000u));check(object.head==0&&original[0].object==&object);++resources;return 0x1234fedcu;},[](auto*){std::abort();return false;},[&](int code){++warnings[code];});
  check(result==0&&object.head==0&&original[0].object==&object&&original[0].handle==handle);check(original[0].resource==(resourceBit?0xfedc:0xffff)&&resources==resourceBit);check(original[0].auxiliary==0xffff&&original[0].fieldE==0xffff);
  check(receivers.freshCount==1&&receivers.pool.count==0);check(warnings[0]==0&&warnings[1]==0&&warnings[2]==failNode&&warnings[3]==failNode&&nodeGrowth==failNode);
  if(failNode)check(shadowHead==nullptr&&original[0].node==nullptr);else check(shadowHead==storage&&original[0].node==storage&&storage[0].index==0&&storage[0].previous==nullptr&&storage[0].next==nullptr);++cases;
 }
 // Node growth may replace entry storage after payload/resource initialization.
 EngineReceiverLinkCandidate links[1];EngineReceiverAllocationCandidate receivers;receivers.pool={links,1,-1,0xffff,0xffff,0};EngineShadowPoolNodeCandidate storage[1];EngineShadowNodeBlockCandidate block{nullptr,1,storage};EngineShadowNodePoolCandidate nodes;
 EngineProjectionAssociationObjectCandidate object{0,0xffff};EngineProjectionAssociationEntryCandidate oldEntry[1],newEntry[1];auto* entries=oldEntry;EngineShadowPoolNodeCandidate* head=nullptr;
 check(engineProjectionAssociateCandidate(receivers,nodes,object,5,[](auto&,int){std::abort();},[&](auto& pool,int){newEntry[0]=oldEntry[0];entries=newEntry;pool.first=&block;},[&](uint16_t i){return entries+i;},[&](uint16_t){return head;},[&](uint16_t,auto* node){head=node;},[](uint32_t){std::abort();return 0;},[](auto*){return false;},[](int){std::abort();})==0);
 check(oldEntry[0].node==nullptr&&newEntry[0].node==storage&&newEntry[0].handle==5&&head==storage);
 EngineReceiverAllocationCandidate empty;EngineShadowNodePoolCandidate emptyNodes;EngineProjectionAssociationObjectCandidate untouched;unsigned warnings[4]{};
 check(engineProjectionAssociateCandidate(empty,emptyNodes,untouched,9,[](auto&,int){},[](auto&,int){std::abort();},[](uint16_t)->EngineProjectionAssociationEntryCandidate*{std::abort();return nullptr;},[](uint16_t)->EngineShadowPoolNodeCandidate*{std::abort();return nullptr;},[](uint16_t,auto*){std::abort();},[](uint32_t){std::abort();return 0;},[](auto*){return false;},[&](int c){++warnings[c];})==0xffff);check(untouched.head==0xffff&&warnings[0]==1&&warnings[1]==1);
 std::printf("PASS: %u association states, retained partial failures, entry reload after node growth and failed entry allocation. Native association/resource parity pending.\n",cases);
}

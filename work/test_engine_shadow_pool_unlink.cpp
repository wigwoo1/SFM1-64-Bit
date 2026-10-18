#include "engine_shadow_pool_unlink_candidate.h"
#include <vector>
#include <iostream>
#include <stdexcept>
static void require(bool ok){if(!ok)throw std::runtime_error("pool unlink contract");}
int main(){try{unsigned checks=0;
 for(unsigned length=1;length<=64;++length)for(unsigned removed=0;removed<length;++removed){
  std::vector<EngineShadowPoolLinkCandidate> links(length);
  for(unsigned i=0;i<length;++i)links[i]={uint16_t(i?i-1:0xffff),uint16_t(i+1<length?i+1:0xffff)};
  EngineShadowPoolStateCandidate pool{links.data(),int32_t(length),0,uint16_t(length-1),uint16_t(length),uint16_t(length-1)};
  engineShadowPoolUnlinkCandidate(pool,0xabcd0000u|removed);
  require(pool.count==length-1&&links[removed].previous==removed&&links[removed].next==removed);
  unsigned seen=0;uint16_t prior=0xffff;
  for(auto index=pool.head;index!=0xffff;index=links[index].next){require(index<length&&index!=removed&&links[index].previous==prior);prior=index;require(++seen<=length);}
  require(seen==length-1&&prior==pool.tail);
  const auto saved=links;const auto count=pool.count;
  engineShadowPoolUnlinkCandidate(pool,removed);
  require(pool.count==count);
  for(unsigned i=0;i<length;++i)require(links[i].previous==saved[i].previous&&links[i].next==saved[i].next);
  ++checks;
 }
 EngineShadowPoolLinkCandidate single{0xffff,0xffff};
 EngineShadowPoolStateCandidate pool{&single,-1,0,0,0,0};
 engineShadowPoolUnlinkCandidate(pool,0);require(single.previous==0xffff&&pool.count==0);
 pool.capacity=1;engineShadowPoolUnlinkCandidate(pool,0);require(pool.count==0xffff&&pool.head==0xffff&&pool.tail==0xffff);
 std::cout<<"pool_removal_topologies="<<checks<<" repeat_removal=passed signed_capacity=passed count_wrap=passed\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}

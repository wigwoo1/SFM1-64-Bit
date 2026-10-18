#include "engine_shadow_enable_candidate.h"
#include <array>
#include <vector>
#include <iostream>
#include <stdexcept>
static void require(bool ok){if(!ok)throw std::runtime_error("shadow enable contract");}
int main(){try{
 unsigned checks=0;
 for(uint32_t initial=0;initial<65536;++initial)for(uint32_t mode: {0u,1u,2u,255u,256u,0xabcdef00u,0xabcdef01u}){
  std::array<uint16_t,2> records{uint16_t(initial),uint16_t(initial^0xa55au)};
  auto* current=&records[0];std::vector<unsigned> events;
  const uint32_t handle=0x12340007u;
  engineShadowEnableCandidate(handle,mode,[&](uint32_t h){require(h==handle);events.push_back(1);current=&records[1];},
   [&](uint32_t h){require(h==handle&&current==&records[1]);events.push_back(2);},
   [&](uint16_t h)->uint16_t&{require(h==7);events.push_back(3);return *current;});
  if(uint8_t(mode)==0){require(events==std::vector<unsigned>({1,2,3}));require(records[0]==initial);require(records[1]==uint16_t((initial^0xa55au)|8u));}
  else{require(events==std::vector<unsigned>({3}));require(records[0]==uint16_t(initial&0xfff7u));require(records[1]==uint16_t(initial^0xa55au));}
  ++checks;
 }
 unsigned cleanupChecks=0;
 for(unsigned length=0;length<=64;++length){
  std::vector<EngineShadowCleanupNodeCandidate> nodes(length);
  for(unsigned i=0;i<length;++i){nodes[i].index=uint16_t(i);nodes[i].next=i+1<length?&nodes[i+1]:nullptr;}
  auto* first=length?&nodes[0]:nullptr;
  EngineShadowCleanupNodeCandidate placeholder;
  auto* replacement=&placeholder;
  auto** current=&first;
  std::vector<uint16_t> released;
  engineShadowCleanupCandidate(0xabcd0007u,[&](uint16_t h)->EngineShadowCleanupNodeCandidate*&{require(h==7);return *current;},
   [&](uint16_t index){return unsigned(index)+100u;},
   [&](unsigned target,uint16_t index){require(target==index+100u);released.push_back(index);nodes[index].next=nullptr;nodes[index].index=0xffff;current=&replacement;});
  require(*current==nullptr&&released.size()==length);
  for(unsigned i=0;i<length;++i)require(released[i]==i);
  if(length)require(first==&nodes[0]);
  ++cleanupChecks;
 }
 std::cout<<"shadow_cleanup_sequences="<<cleanupChecks<<" saved_next=passed reloaded_head=passed ";
 std::cout<<"shadow_enable_sequences="<<checks<<" callback_order=passed replaced_storage=passed high_argument_bits=passed\n";
 return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}

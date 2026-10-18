#include "deferred_light_removal_candidate.h"
#include <iostream>
#include <stdexcept>
int main(){try{
 std::vector<uint16_t> queue;
 for(uint32_t i=0;i<65536;++i)queueDeferredLightRemovalCandidate(queue,i|0xabcd0000u);
 if(queue.size()!=65536)throw std::runtime_error("lost handles");
 for(uint32_t i=65536;i>0;--i)queueDeferredLightRemovalCandidate(queue,(i-1)|0x12340000u);
 if(queue.size()!=65536)throw std::runtime_error("duplicate alias");
 for(uint32_t i=0;i<65536;++i)if(queue[i]!=i)throw std::runtime_error("ordering");
 std::vector<uint16_t> events;
 queue={7,2};
 drainDeferredLightRemovalCandidate(queue,[&](uint16_t h){
  events.push_back(h);
  if(h==7){queueDeferredLightRemovalCandidate(queue,2);queueDeferredLightRemovalCandidate(queue,9);}
 });
 if(!queue.empty()||events!=std::vector<uint16_t>({7,2,9}))throw std::runtime_error("live queue drain");
 drainDeferredLightRemovalCandidate(queue,[&](uint16_t){throw std::runtime_error("empty drain callback");});
 for(unsigned alternate=0;alternate<2;++alternate){
  uint8_t deferred=7;queue.clear();events.clear();unsigned selected=0;
  auto evaluate=[&](unsigned path){if(deferred!=1)throw std::runtime_error("deferral not enabled");selected=path;queueDeferredLightRemovalCandidate(queue,12);};
  evaluateWithDeferredLightRemovalCandidate(deferred,queue,alternate!=0,[&]{evaluate(1);},[&]{evaluate(2);},[&](uint16_t h){if(deferred!=0)throw std::runtime_error("drain still deferred");events.push_back(h);});
  if(selected!=(alternate?1u:2u)||deferred!=0||!queue.empty()||events!=std::vector<uint16_t>({12}))throw std::runtime_error("evaluation scope");
 }
 std::cout<<"all_65536_handles_preserved high_word_aliases_deduplicated insertion_order_preserved\n";
 return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}

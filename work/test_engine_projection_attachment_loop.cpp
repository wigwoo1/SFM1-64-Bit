#include "engine_projection_attachment_loop_candidate.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
void check(bool ok){if(!ok)std::abort();}
int main(){unsigned cases=0;
 for(unsigned length=0;length<=64;++length){uint16_t live=uint16_t(length),base=65535;unsigned lookups=0,counts=0,prepared=0,gated=0;std::vector<unsigned> events;
  bool result=engineProjectionAttachmentLoopCandidate(1,[&]{++counts;return live;},[&]{return base;},[&](uint32_t slot){check(slot==uint32_t(base)+lookups);++lookups;return uint16_t(slot);},[&](uint16_t index){events.push_back(1);return index;},[&](uint16_t){events.push_back(2);return lookups%2?0x100:0x101;},[&](uint16_t){events.push_back(3);++prepared;},[&](uint16_t,uint8_t flag){check(flag==1);events.push_back(4);++gated;base=100;});
  check(result&&lookups==length&&counts==length+1&&prepared==(length+1)/2&&gated==prepared);size_t event=0;for(unsigned i=0;i<length;++i){check(events[event++]==1&&events[event++]==2);if(i%2==0)check(events[event++]==3&&events[event++]==4);}check(event==events.size());++cases;
 }
 uint16_t live=1;unsigned seen=0;
 engineProjectionAttachmentLoopCandidate(0,[&]{return live;},[]{return uint16_t(0);},[](uint32_t slot){return uint16_t(slot);},[](uint16_t i){return i;},[](uint16_t){return 0;},[&](uint16_t){if(seen==0)live=3;},[&](uint16_t object,uint8_t flag){check(object==seen&&flag==0);++seen;if(seen==2)live=2;});check(seen==2);
 std::printf("PASS: %u attachment lengths; uint32 base addition, low-byte skip, callback order, live base/count growth and shrink. Full native callback integration pending.\n",cases);
}

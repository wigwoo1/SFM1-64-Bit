#include "studio_slot267_candidate.h"
#include <cstdio>
struct Ops{
 uint8_t& cache;uint8_t before;unsigned calls=0;
 void renderState(unsigned state,unsigned value,unsigned force){
  if(state!=28||value>255||force||cache!=before)throw 1;
  ++calls;cache=static_cast<uint8_t>(value^0xff);
 }
};
int main(){unsigned cases=0;
 for(unsigned input=0;input<256;++input)for(unsigned old=0;old<256;++old)for(unsigned high=0;high<2;++high){
  uint8_t cache=static_cast<uint8_t>(old);Ops ops{cache,cache};
  const bool changed=studioSlot267Candidate(input|(high?0xabcd0000u:0u),cache,ops);
  if(changed!=(input!=old)||ops.calls!=unsigned(input!=old)||cache!=input)return 2;
  ++cases;
 }
 std::printf("slot267_cases=%u exact_byte_compare=1 post_callback_publication=1\n",cases);
}

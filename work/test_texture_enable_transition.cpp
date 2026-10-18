#include "studio_texture_enable_transition_candidate.h"
#include <cstdio>
struct Ops {
 StudioTextureEnableStageCandidate& live;bool queued;unsigned action=0;
 bool queueEnabled(){return queued;}
 void bind(uint32_t stage,uintptr_t handle,uint32_t force){if(stage!=7||handle!=0x12345678abcdef12ull||force!=1||live.enabled!=1)throw 1;action=1;live.handle=19;live.enabled=9;}
 void queueNullTexture(uint32_t s){if(s!=7||live.enabled)throw 2;action=2;live.enabled=9;}
 void directNullTexture(uint32_t s){if(s!=7||live.enabled)throw 3;action=3;live.enabled=9;}
};
int main(){unsigned cases=0;
 for(unsigned byte=0;byte<256;++byte)for(unsigned cached=0;cached<256;++cached)for(unsigned mode=0;mode<4;++mode){
  StudioTextureEnableStageCandidate live{mode&1?0x12345678abcdef12ull:0ull,uint8_t(cached)};
  Ops ops{live,bool(mode&2)};
  // Enable callback expects captured nonzero handle; zero handle is also valid.
  if((byte&1)&&!(mode&1))live.handle=0x12345678abcdef12ull;
  const bool changed=(byte&1)!=cached;
  const unsigned expected=!changed?0:(byte&1)?1:(mode&1)?(mode&2?2:3):0;
  studioTextureEnableTransitionCandidate(uint8_t(byte),7,live,ops);
  if(ops.action!=expected||live.enabled!=(expected?9:changed?byte&1:cached))return 4;
  ++cases;
 }
 std::printf("texture_enable_cases=%u exact_byte_cache=1 pre_callback_publication=1 callback_mutation_retained=1\n",cases);return 0;
}

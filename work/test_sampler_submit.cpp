#include "studio_sampler_submit_candidate.h"
#include <cstdio>
struct Ops {
 uint32_t gate;bool queued,flip;unsigned calls=0,kind=0;std::array<uint32_t,4> packet{};
 uint32_t suppressed(){if(flip)queued=!queued;return gate;}
 bool queueEnabled(){return queued;}
 void queueSampler(const std::array<uint32_t,4>& p){++calls;kind=1;packet=p;}
 void directSampler(uint32_t s,uint32_t t,uint32_t v){++calls;kind=2;packet={17,s,t,v};}
};
int main(){unsigned cases=0;
 for(uint32_t gate=0;gate<512;++gate)for(unsigned mode=0;mode<4;++mode){
  Ops ops{gate,bool(mode&1),bool(mode&2)};
  studioSamplerSubmitCandidate(257,6,0xabcdef01,ops);
  const bool execute=!uint8_t(gate),queued=bool(mode&1)!=bool(mode&2);
  if(ops.calls!=(execute?1u:0u)||ops.kind!=(execute?(queued?1u:2u):0u))return 1;
  if(execute&&ops.packet!=std::array<uint32_t,4>{17,257,6,0xabcdef01})return 2;
  ++cases;
 }
 std::printf("sampler_submit_cases=%u lowbyte_suppression=1 live_queue_choice=1 packet_fields=1\n",cases);return 0;
}

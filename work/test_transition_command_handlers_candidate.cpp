#include "studio_transition_command_handlers_candidate.h"
#include <cassert>
#include <iostream>
struct Services{
 StudioTransitionState72Candidate* state{};
 unsigned key{},value{},stage{},cache{},calls{},capability{},reads{};
 bool mutate{};
 void render(unsigned k,unsigned v){key=k;value=v;++calls;if(mutate)state->values[2]=99;}
 void publishCache140(unsigned v){cache=v;}
 void sampler(unsigned s,unsigned k,unsigned v){stage=s;key=k;value=v;++calls;}
 unsigned specialCapability(){++reads;return capability;}
};
int main(){
 StudioTransitionState72Candidate state{};Services s{};s.state=&state;
 state.values[3]=0x12345678;studioTransitionRender8Candidate(state,s);assert(s.key==8&&s.value==0x12345678);
 for(unsigned b=0;b<256;++b){state.flags44=static_cast<std::uint8_t>(b);studioTransitionRender14Candidate(state,s);assert(s.key==14&&s.value==((b&2)?1u:0u));
  for(unsigned stage=0;stage<16;++stage){state.values[12+stage/4]=b<<((stage%4)*8);
   studioTransitionSampler11Candidate(state,stage,s);assert(s.stage==stage&&s.key==11&&s.value==((b&2)?1u:0u));
   for(unsigned cap:{0u,1u,256u}){s.capability=cap;const auto before=s.calls,reads=s.reads;studioTransitionSampler8Candidate(state,stage,s);assert(s.reads==reads+1);
    if(cap==1){assert(s.calls==before+1&&s.stage==stage&&s.key==8&&s.value==((b&4)?0x34544547u:0x31544547u));}else assert(s.calls==before);
   }
  }
 }
 state.values[2]=17;s.mutate=true;studioTransitionRender168Candidate(state,s);assert(s.key==168&&s.value==17&&s.cache==99);
 s.capability=0;studioTransitionSampler8Candidate(state,100,s); // Disabled capability skips even an invalid owned stage.
 std::cout<<"transition_command_handlers_policy=passed byte_stage_cases=4096\n";
}

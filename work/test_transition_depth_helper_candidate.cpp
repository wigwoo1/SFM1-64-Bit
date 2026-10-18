#include "studio_transition_depth_helper_candidate.h"
#include <vector>
#include <cassert>
#include <cstdio>
struct Services{
 StudioTransitionState72Candidate& state;bool mutate;
 std::vector<unsigned> trace;
 void submit(unsigned key,unsigned value){trace.insert(trace.end(),{key,value});if(mutate&&key==7){state.values[1]=0;state.flags44=8;}}
 void applyDepthBackend(StudioTransitionState72Candidate& value){assert(&value==&state);trace.push_back(261);if(mutate)value.flags44=12;}
};
int main(){unsigned cases=0;
 for(unsigned flags=0;flags<256;++flags)for(unsigned cached=0;cached<4;++cached)for(unsigned dirty=0;dirty<2;++dirty)for(unsigned mutation=0;mutation<2;++mutation){
  StudioTransitionState72Candidate state{};state.values[0]=3;state.values[1]=1;state.flags44=static_cast<std::uint8_t>(flags);
  StudioTransitionDepthCacheCandidate cache{};cache.mode124=cached;cache.dirty178=static_cast<std::uint8_t>(dirty);
  Services services{state,mutation!=0,{}};studioTransitionDepthHelperCandidate(state,cache,services);
  const unsigned effective=mutation?8:flags;const unsigned mode=(effective>>2)&3;
  const bool backend=cached!=mode||((effective&12)==8&&dirty);
  std::vector<unsigned> expected{7,1};if(!mutation)expected.insert(expected.end(),{23,3});if(backend)expected.push_back(261);
  assert(services.trace==expected&&cache.state7==1&&cache.state23==(mutation?0u:3u));
  assert(cache.mode124==(backend?(mutation?3u:mode):cached)&&cache.dirty178==dirty);++cases;
 }
 std::printf("transition_depth_helper_policy_cases=%u passed\n",cases);
}


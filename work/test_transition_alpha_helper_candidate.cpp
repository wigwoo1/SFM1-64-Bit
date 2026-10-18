#include "studio_transition_alpha_helper_candidate.h"
#include <vector>
#include <cassert>
#include <cstdio>
struct Services{
 StudioTransitionState72Candidate& state;unsigned mutation;std::vector<unsigned> trace;
 void render(unsigned key,unsigned value){trace.insert(trace.end(),{key,value});
  if(key==15&&mutation==1)state.flags44=64;
  if(key==15&&mutation==2)state.flags44=0;
  if(key==25&&mutation==3){state.values[10]=99;state.values[11]=100;}
  if(key==24&&mutation==3)state.values[11]=101;
 }
};
int main(){unsigned cases=0;
 for(unsigned flags=0;flags<256;++flags)for(unsigned enabled:{0u,1u,255u})for(unsigned equal=0;equal<4;++equal)for(unsigned mutation=0;mutation<4;++mutation){
  StudioTransitionState72Candidate state{};state.flags44=static_cast<std::uint8_t>(flags);state.values[10]=7;state.values[11]=13;
  StudioTransitionAlphaCacheCandidate cache{static_cast<std::uint8_t>(enabled),(equal&1)?7u:0u,(equal&2)?13u:0u};Services services{state,mutation,{}};
  const unsigned initial=(flags&64)?1u:0u;const bool first=enabled!=initial;
  unsigned effective=initial;if(first&&mutation==1)effective=1;if(first&&mutation==2)effective=0;
  std::vector<unsigned> expected;if(first)expected.insert(expected.end(),{15,initial});
  const bool function=effective&&!(equal&1);unsigned finalReference=13;
  if(function){expected.insert(expected.end(),{25,7});if(mutation==3)finalReference=100;}
  const unsigned oldReference=(equal&2)?13u:0u;const bool reference=effective&&oldReference!=finalReference;
  if(reference)expected.insert(expected.end(),{24,finalReference});
  studioTransitionAlphaHelperCandidate(state,cache,services);
  assert(services.trace==expected&&cache.enabled128==(first?effective:enabled));
  assert(cache.function12c==(function?(mutation==3?99u:7u):((equal&1)?7u:0u)));
  assert(cache.reference130==(reference?(mutation==3?101u:finalReference):oldReference));++cases;
 }
 std::printf("transition_alpha_helper_policy_cases=%u passed\n",cases);
}

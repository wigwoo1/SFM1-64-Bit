#include "studio_transition_blend_helper_candidate.h"
#include <vector>
#include <cassert>
#include <cstdio>
struct Services{
 StudioTransitionState72Candidate& state;unsigned mutation;std::vector<unsigned> trace;
 void render(unsigned key,unsigned value){trace.insert(trace.end(),{key,value});
  if(key==27&&mutation==1)state.flags44=32;
  if(key==27&&mutation==2)state.flags44=0;
  if(mutation==3){if(key==19){state.values[4]=99;state.values[5]=100;}if(key==20){state.values[5]=101;state.values[6]=102;}if(key==171)state.values[6]=103;}
 }
};
int main(){unsigned cases=0;
 for(unsigned flags=0;flags<256;++flags)for(unsigned enabled:{0u,1u,255u})for(unsigned equal=0;equal<8;++equal)for(unsigned mutation=0;mutation<4;++mutation){
  StudioTransitionState72Candidate state{};state.flags44=static_cast<std::uint8_t>(flags);state.values[4]=7;state.values[5]=13;state.values[6]=17;
  StudioTransitionBlendCacheCandidate cache{static_cast<std::uint8_t>(enabled),(equal&1)?7u:0u,(equal&2)?13u:0u,(equal&4)?17u:0u};Services services{state,mutation,{}};
  const unsigned initial=(flags&32)?1u:0u;const bool first=enabled!=initial;unsigned effective=initial;
  if(first&&mutation==1)effective=1;if(first&&mutation==2)effective=0;
  std::vector<unsigned> expected;if(first)expected.insert(expected.end(),{27,initial});
  const bool op=effective&&!(equal&1);const unsigned src=op&&mutation==3?100u:13u;
  const bool source=effective&&((equal&2)?13u:0u)!=src;const unsigned dst=source&&mutation==3?102u:17u;
  const bool dest=effective&&((equal&4)?17u:0u)!=dst;
  if(op)expected.insert(expected.end(),{19,7});if(source)expected.insert(expected.end(),{20,src});if(dest)expected.insert(expected.end(),{171,dst});
  studioTransitionBlendHelperCandidate(state,cache,services);
  assert(services.trace==expected&&cache.enabledFc==(first?effective:enabled));
  assert(cache.operation100==(op?(mutation==3?99u:7u):((equal&1)?7u:0u)));
  assert(cache.source104==(source?(mutation==3?101u:src):((equal&2)?13u:0u)));
  assert(cache.destination108==(dest?(mutation==3?103u:dst):((equal&4)?17u:0u)));++cases;
 }
 std::printf("transition_blend_helper_policy_cases=%u passed\n",cases);
}

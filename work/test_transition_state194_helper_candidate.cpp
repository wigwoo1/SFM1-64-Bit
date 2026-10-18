#include "studio_transition_state194_helper_candidate.h"
#include <vector>
#include <cassert>
#include <cstdio>
struct Services{
 StudioTransitionState72Candidate& state;unsigned cap,extra;int version;bool mutate;std::vector<unsigned> trace;
 unsigned capability41(){trace.push_back(41);return cap;}
 int capability17(){trace.push_back(17);return version;}
 unsigned capability25(){trace.push_back(25);return extra;}
 void render(unsigned key,unsigned value){trace.insert(trace.end(),{key,value});if(mutate){state.flags44=129;state.flags45=4;state.value40=99;}}
 void applyBackend271(unsigned value){trace.insert(trace.end(),{271,value});}
 void applyCombined(unsigned value,unsigned a,unsigned b,unsigned c){trace.insert(trace.end(),{264,value,a,b,c});}
};
int main(){unsigned cases=0;
 for(unsigned flags=0;flags<256;++flags)for(unsigned forced:{0u,1u})for(unsigned cap:{0u,1u})for(int version:{-1,91,92})for(unsigned extra:{0u,1u,256u})for(unsigned mutation=0;mutation<2;++mutation){
  StudioTransitionState72Candidate state{};state.flags44=static_cast<std::uint8_t>(flags);state.flags45=4;state.value40=17;
  Services services{state,cap,extra,version,mutation!=0,{}};std::vector<unsigned> expected;
  if(forced)expected={194,0,271,0};else{
   expected.push_back(41);if(cap)expected.push_back(17);
   expected.insert(expected.end(),{194,cap&&version>=92?0u:((flags&128)?1u:0u),271,mutation?1u:((flags&128)?1u:0u),25});
   if(extra==1)expected.insert(expected.end(),{264,mutation?99u:17u,mutation?1u:(flags&1u),mutation?1u:((flags&128)?1u:0u),1});
  }
  studioTransitionState194HelperCandidate(state,static_cast<std::uint8_t>(forced),services);assert(services.trace==expected);++cases;
 }
 std::printf("transition_state194_helper_policy_cases=%u passed\n",cases);
}

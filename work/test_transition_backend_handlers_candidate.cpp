#include "studio_transition_backend_handlers_candidate.h"
#include <vector>
#include <cstdio>
struct Services{int count=3;unsigned reads=0;std::vector<unsigned> trace;
 int stageCount(){++reads;return count;}
 void applyStage(StudioTransitionState72Candidate* state,int stage){trace.push_back(static_cast<unsigned>(stage));state->value40+=1;count=0;}
 void applyFlag44(unsigned value){trace.push_back(value);}
 void applyFlag45(unsigned value){trace.push_back(value);}
 void applyCombined(unsigned value,unsigned first,unsigned second,unsigned third){trace.insert(trace.end(),{value,first,second,third});}
};
int main(){unsigned cases=0;for(unsigned a=0;a<256;++a)for(unsigned b=0;b<256;++b){StudioTransitionState72Candidate state;state.flags44=static_cast<uint8_t>(a);state.flags45=static_cast<uint8_t>(b);state.value40=0xabcdef01;Services services;
 studioTransitionFlag44Candidate(state,services);studioTransitionFlag45Candidate(state,services);studioTransitionCombinedFlagsCandidate(state,services);
 const std::vector<unsigned> expected{(a&16)?1u:0u,(b&8)?1u:0u,0xabcdef01,(a&1)?1u:0u,(a&128)?1u:0u,(b&4)?1u:0u};if(services.trace!=expected)return 1;++cases;
 }StudioTransitionState72Candidate state;Services services;studioTransitionStagesCandidate(&state,services);if(services.reads!=1||services.trace!=std::vector<unsigned>{0,1,2}||state.value40!=3)return 2;services.count=-1;studioTransitionStagesCandidate(nullptr,services);if(services.reads!=2||services.trace.size()!=3)return 3;std::printf("transition_backend_flag_cases=%u stage_capture=passed\n",cases);return 0;}

#include "studio_restore_commit_candidate.h"
#include <vector>
#include <cstdio>
#include <stdexcept>
struct Services{StudioRestoreCommitStateCandidate& state;std::vector<unsigned> trace;bool mutate;
 void apply134(unsigned value){trace.push_back(1);trace.push_back(value);if(mutate){state.dirty135=1;state.value138=99;}}
 void apply135(unsigned value){trace.push_back(2);trace.push_back(value);if(mutate){state.value138=123;state.dirty13c=1;state.enabled13d=1;}}
 void submit(unsigned key,unsigned value){trace.push_back(key);trace.push_back(value);if(mutate&&key==168&&value==8){state.dirty13e=1;state.enabled13f=1;}}
};
int main(){
 for(bool mutation:{false,true}){StudioRestoreCommitStateCandidate state;state.dirty134=1;Services services{state,{},mutation};studioRestoreCommitCandidate(state,services);
  const std::vector<unsigned> expected=mutation?std::vector<unsigned>{1,3,2,1,14,123,168,8,168,15}:std::vector<unsigned>{1,3};
  if(services.trace!=expected||state.cached140!=(mutation?15u:0u)||state.dirty134!=1)return 1;
 }
 StudioRestoreCommitStateCandidate same;same.dirty13c=same.dirty13e=same.enabled13d=same.enabled13f=1;same.cached140=0xabcd000f;Services services{same,{},false};studioRestoreCommitCandidate(same,services);if(!services.trace.empty()||same.cached140!=0xabcd000f)return 2;
 std::puts("restore_commit_live_mutation_cases=3 passed");return 0;
}

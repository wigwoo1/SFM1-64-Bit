#include "studio_restore_blend_commit_candidate.h"
#include <array>
#include <cstdio>
using Event=std::array<uint32_t,4>;
struct Services{StudioRestoreBlendStateCandidate& state;unsigned suppression,special;int liveCount=2;unsigned countReads=0,gateReads=0,transitions=0,commits=0;std::vector<Event> events;
 void render(unsigned key,unsigned value){events.push_back({1,key,value});if(key==27)state.f100=99;}
 int samplerCount(){++countReads;return liveCount;}
 void sampler(unsigned stage,unsigned type,unsigned value){events.push_back({2,stage,type,value});if(stage==0&&type==11){liveCount=0;state.samplers[0].second=1;}}
 unsigned specialSamplerEnabled(){return special;}
 unsigned suppressed(){++gateReads;state.kind=9;state.descriptor=0x02123456;return suppression;}
 void transition(int kind,unsigned offset,unsigned count){if(kind!=-2||offset!=0x123456||count!=2)throw 1;++transitions;state.commit.dirty134=state.commit.dirty135=state.commit.dirty13c=state.commit.dirty13e=1;state.commit.enabled13d=state.commit.enabled13f=1;state.commit.value138=123;}
 void commit(){++commits;studioRestoreBlendCommitCandidate(state,*this);state.cachedKind=77;}
 void vertexShader(int value){if(value!=-1)throw 2;events.push_back({3,9,UINT32_MAX});state.cachedHandle=12;}
 void pixelShader(int value){if(value!=-1)throw 3;events.push_back({3,10,UINT32_MAX});state.cachedHandle=34;}
};
int main(){unsigned cases=0;try{for(unsigned suppress:{0u,1u,256u})for(unsigned special:{0u,1u,256u}){
 StudioRestoreBlendStateCandidate state;state.kind=-2;state.cachedKind=5;state.cachedHandle=6;state.descriptor=0;state.samplers={{17,0,0},{19,1,0}};state.commit.dirty134=state.commit.dirty135=state.commit.dirty13c=state.commit.dirty13e=1;state.f144=1;
 Services services{state,suppress,special};studioRestoreBlendCandidate(state,services);
 std::vector<Event> expected{{1,27,0},{1,19,99},{1,20,1},{1,171,1},{1,206,0},{1,207,2},{1,208,1},{1,209,1},{1,7,1},{1,23,4},{1,15,0},{1,25,7},{1,24,0},{2,0,11,17}};
 if(static_cast<uint8_t>(special))expected.push_back({2,0,8,0x34544547u});expected.push_back({2,1,11,19});if(static_cast<uint8_t>(special))expected.push_back({2,1,8,0x34544547u});if(!static_cast<uint8_t>(suppress)){expected.push_back({1,23,3});expected.push_back({1,14,123});expected.push_back({1,168,8});expected.push_back({1,168,15});}expected.push_back({3,9,UINT32_MAX});expected.push_back({3,10,UINT32_MAX});
 if(services.events!=expected||services.countReads!=1||services.gateReads!=1||state.cachedHandle!=-1||state.cachedKind!=(static_cast<uint8_t>(suppress)?5:-2)||services.transitions!=(static_cast<uint8_t>(suppress)?0u:1u)||services.commits!=services.transitions||state.commit.dirty134!=(static_cast<uint8_t>(suppress)?0:1)||state.commit.dirty135!=(static_cast<uint8_t>(suppress)?0:1)||state.commit.dirty13c!=(static_cast<uint8_t>(suppress)?0:1)||state.commit.dirty13e!=(static_cast<uint8_t>(suppress)?0:1)||state.f144)return 1;++cases;
 }std::printf("restore_blend_commit_composition_cases=%u passed\n",cases);return 0;}catch(...){return 2;}}



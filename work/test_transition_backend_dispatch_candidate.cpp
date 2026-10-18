#include "studio_transition_backend_dispatch_candidate.h"
#include <vector>
#include <cstdio>
struct Services{int count=3;unsigned reads=0;std::vector<uint32_t> trace;
 StudioTransitionDepthCacheCandidate cache{};
 StudioTransitionAlphaCacheCandidate alpha{};
 StudioTransitionBlendCacheCandidate blend{};
 StudioTransitionSeparateBlendCacheCandidate separateBlend{};
 StudioTransitionSeparateBlendCacheCandidate& separateBlendCache(){return separateBlend;}
 StudioTransitionBlendCacheCandidate& blendCache(){return blend;}
 StudioTransitionAlphaCacheCandidate& alphaCache(){return alpha;}
 StudioTransitionDepthCacheCandidate& depthCache(){return cache;}
 void applyDepthBackend(StudioTransitionState72Candidate& state){trace.insert(trace.end(),{261,(state.flags44>>2)&3u});}
 int stageCount(){++reads;return count;}
 void applyStage(StudioTransitionState72Candidate* state,int stage){trace.push_back(100+static_cast<unsigned>(stage));++state->value40;count=0;}
 void applyFlag44(unsigned value){trace.insert(trace.end(),{44,value});}
 void applyFlag45(unsigned value){trace.insert(trace.end(),{45,value});}
 void applyCombined(unsigned value,unsigned a,unsigned b,unsigned c){trace.insert(trace.end(),{40,value,a,b,c});}
 std::uint8_t force144(){return 0;}
 unsigned capability41(){return 1;}
 int capability17(){return 92;}
 unsigned capability25(){return 1;}
 void applyBackend271(unsigned value){trace.insert(trace.end(),{271,value});}
 void render(unsigned key,unsigned value){trace.insert(trace.end(),{300,key,value});}
 void publishCache140(unsigned value){trace.insert(trace.end(),{400,value});}
 unsigned specialCapability(){return 1;}
 void sampler(unsigned stage,unsigned key,unsigned value){trace.insert(trace.end(),{200,stage,key,value});}
};
int main(){StudioTransitionState72Candidate state{};state.flags44=0x91;state.flags45=0x0c;state.value40=10;state.values[2]=17;state.values[3]=23;state.values[15]=0x06000000;const uint8_t ops[]{8,9,10,11,12,0x9f,0x8f,1,2,4};Services services;
 studioTransitionBackendDispatchCandidate(&state,ops,10,services);
 const std::vector<uint32_t> expected{44,1,40,10,1,1,1,100,101,102,40,13,1,1,1,45,1,200,15,8,0x34544547,200,15,11,1,300,14,0,300,168,17,400,17,300,8,23};if(services.trace!=expected||services.reads!=1||state.value40!=13)return 1;
 services.trace.clear();state.values[0]=3;state.values[1]=1;state.flags44=8;const uint8_t depthOps[]{0,0};
 studioTransitionBackendDispatchCandidate(&state,depthOps,2,services);
 if(services.trace!=std::vector<uint32_t>{300,7,1,300,23,3,261,2}||services.cache.state7!=1||services.cache.state23!=3||services.cache.mode124!=2)return 2;
 services.trace.clear();services.cache.dirty178=1;studioTransitionBackendDispatchCandidate(&state,depthOps,2,services);
 if(services.trace!=std::vector<uint32_t>{261,2,261,2}||services.cache.dirty178!=1)return 3;
 services.trace.clear();state.flags44=64;state.values[10]=7;state.values[11]=13;const uint8_t alphaOps[]{3,3};
 studioTransitionBackendDispatchCandidate(&state,alphaOps,2,services);
 if(services.trace!=std::vector<uint32_t>{300,15,1,300,25,7,300,24,13}||services.alpha.enabled128!=1||services.alpha.function12c!=7||services.alpha.reference130!=13)return 4;
 services.trace.clear();state.flags44=32;state.values[4]=7;state.values[5]=13;state.values[6]=17;const uint8_t blendOps[]{6,6};
 studioTransitionBackendDispatchCandidate(&state,blendOps,2,services);
 if(services.trace!=std::vector<uint32_t>{300,27,1,300,19,7,300,20,13,300,171,17}||services.blend.enabledFc!=1||services.blend.operation100!=7||services.blend.source104!=13||services.blend.destination108!=17)return 5;
 services.trace.clear();state.flags45=1;state.values[7]=7;state.values[8]=13;state.values[9]=17;const uint8_t separateOps[]{7,7};
 studioTransitionBackendDispatchCandidate(&state,separateOps,2,services);
 if(services.trace!=std::vector<uint32_t>{300,206,1,300,207,7,300,208,13,300,209,17}||services.separateBlend.enabled10c!=1||services.separateBlend.operation110!=7||services.separateBlend.source114!=13||services.separateBlend.destination118!=17)return 6;
 services.trace.clear();state.flags44=129;state.flags45=4;const uint8_t stateOps[]{5};studioTransitionBackendDispatchCandidate(&state,stateOps,1,services);
 if(services.trace!=std::vector<uint32_t>{300,194,0,271,1,40,13,1,1,1})return 7;
 std::puts("transition_backend_dispatch_composition=passed");return 0;}


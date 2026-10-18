#include "studio_transition_backend_dispatch_candidate.h"
#include <array>
#include <vector>
#include <fstream>
#include <cstring>
#include <cstdio>
struct Services{
 StudioTransitionDepthCacheCandidate depth{};StudioTransitionAlphaCacheCandidate alpha{};StudioTransitionBlendCacheCandidate blend{};StudioTransitionSeparateBlendCacheCandidate separate{};
 std::array<unsigned,128> queue{},cache{};std::vector<unsigned> trace;unsigned used=0,getters=0;
 StudioTransitionState72Candidate* directState=nullptr;
 Services(){queue.fill(0xcccccccc);}
 auto& depthCache(){return depth;}auto& alphaCache(){return alpha;}auto& blendCache(){return blend;}auto& separateBlendCache(){return separate;}
 void render(unsigned key,unsigned value){++getters;queue[used++]=1;queue[used++]=key;queue[used++]=value;if(directState&&key==194)directState->value40=99;}
 void sampler(unsigned stage,unsigned key,unsigned value){++getters;queue[used++]=17;queue[used++]=stage;queue[used++]=key;queue[used++]=value;}
 void publishCache140(unsigned value){cache[0x140/4]=value;}
 std::uint8_t force144(){return 0;}
 unsigned capability41(){trace.push_back(41);return 1;}int capability17(){trace.push_back(17);return 92;}unsigned capability25(){trace.push_back(25);return 1;}unsigned specialCapability(){trace.push_back(49);return 1;}
 int stageCount(){trace.push_back(1);return 2;}
 void applyDepthBackend(StudioTransitionState72Candidate& state){trace.insert(trace.end(),{261,(state.flags44>>2)&3u});}
 void applyStage(StudioTransitionState72Candidate*,int stage){trace.insert(trace.end(),{262,static_cast<unsigned>(stage)});}
 void applyFlag44(unsigned v){trace.insert(trace.end(),{263,v});}void applyFlag45(unsigned v){trace.insert(trace.end(),{272,v});}void applyBackend271(unsigned v){trace.insert(trace.end(),{271,v});}
 void applyCombined(unsigned v,unsigned a,unsigned b,unsigned c){trace.insert(trace.end(),{264,v,a,b,c});}
 void marshal(){
  cache[0x11c/4]=depth.state7;cache[0x120/4]=depth.state23;cache[0x124/4]=depth.mode124;reinterpret_cast<unsigned char*>(cache.data())[0x178]=depth.dirty178;
  reinterpret_cast<unsigned char*>(cache.data())[0x128]=alpha.enabled128;cache[0x12c/4]=alpha.function12c;cache[0x130/4]=alpha.reference130;
  reinterpret_cast<unsigned char*>(cache.data())[0xfc]=blend.enabledFc;cache[0x100/4]=blend.operation100;cache[0x104/4]=blend.source104;cache[0x108/4]=blend.destination108;
  reinterpret_cast<unsigned char*>(cache.data())[0x10c]=separate.enabled10c;cache[0x110/4]=separate.operation110;cache[0x114/4]=separate.source114;cache[0x118/4]=separate.destination118;
 }
};
int main(){static_assert(sizeof(void*)==8,"x64 comparison required");StudioTransitionState72Candidate state{};state.flags44=255;state.flags45=13;state.values[0]=3;state.values[1]=1;state.values[2]=17;state.values[3]=23;state.values[4]=state.values[7]=state.values[10]=7;state.values[5]=state.values[8]=state.values[11]=13;state.values[6]=state.values[9]=17;state.value40=17;state.values[15]=0x06000000;
 const unsigned char operations[]{0,1,2,3,4,5,6,7,8,9,10,11,12,0x8f,0x9f};Services services;studioTransitionBackendDispatchCandidate(&state,operations,15,services);services.marshal();
 std::array<unsigned,340> actual{},native{};std::memcpy(actual.data(),&state,72);std::memcpy(actual.data()+18,services.cache.data(),512);std::memcpy(actual.data()+146,services.queue.data(),512);for(unsigned i=0;i<services.trace.size();++i)actual[274+i]=services.trace[i];actual[338]=services.getters;actual[339]=static_cast<unsigned>(services.trace.size());
 std::ifstream oracle("outputs/native-transition-composed.bin",std::ios::binary);oracle.read(reinterpret_cast<char*>(native.data()),sizeof(native));if(!oracle)return 1;
 for(unsigned i=0;i<actual.size();++i)if(actual[i]!=native[i]){std::printf("mismatch word=%u actual=%u native=%u\n",i,actual[i],native[i]);return 2;}
 if(services.used!=59||services.getters!=19)return 3;
 services.queue.fill(0xcccccccc);services.used=services.getters=0;services.trace.clear();studioTransitionBackendDispatchCandidate(&state,operations,15,services);services.marshal();actual.fill(0);
 std::memcpy(actual.data(),&state,72);std::memcpy(actual.data()+18,services.cache.data(),512);std::memcpy(actual.data()+146,services.queue.data(),512);for(unsigned i=0;i<services.trace.size();++i)actual[274+i]=services.trace[i];actual[338]=services.getters;actual[339]=static_cast<unsigned>(services.trace.size());oracle.read(reinterpret_cast<char*>(native.data()),sizeof(native));if(!oracle)return 4;
 if(actual!=native||services.used!=20||services.getters!=6)return 5;
 Services direct;direct.directState=&state;state.value40=17;studioTransitionBackendDispatchCandidate(&state,operations,15,direct);direct.marshal();actual.fill(0);std::memcpy(actual.data(),&state,72);std::memcpy(actual.data()+18,direct.cache.data(),512);std::memcpy(actual.data()+146,direct.queue.data(),512);for(unsigned i=0;i<direct.trace.size();++i)actual[274+i]=direct.trace[i];actual[338]=direct.getters;actual[339]=static_cast<unsigned>(direct.trace.size());oracle.read(reinterpret_cast<char*>(native.data()),sizeof(native));if(!oracle||oracle.peek()!=std::char_traits<char>::eof())return 6;if(actual!=native||state.value40!=99||direct.used!=59)return 7;
 std::puts("transition_composed_x64_full_record=passed queued_repeat_and_direct=passed");
}

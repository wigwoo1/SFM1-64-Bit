#include "studio_transition_restore_cache_views_candidate.h"
#include "studio_transition_backend_dispatch_candidate.h"
#include "studio_restore_blend_commit_candidate.h"
#include <cassert>
#include <cstdio>
struct Services{
 StudioRestoreBlendStateCandidate& parent;StudioTransitionState72Candidate& model;
 std::uint8_t dirty=0;StudioTransitionRestoreDepthViewCandidate depth;StudioTransitionRestoreAlphaViewCandidate alpha;StudioTransitionRestoreBlendViewCandidate blend;StudioTransitionRestoreSeparateBlendViewCandidate separate;
 unsigned suppression,transitions=0,commits=0,depthCalls=0,shaderCalls=0;std::vector<unsigned> commands;
 bool all=false;std::vector<unsigned> backend,samplers;
 Services(StudioRestoreBlendStateCandidate& p,StudioTransitionState72Candidate& m,unsigned gate):parent(p),model(m),depth(p,dirty),alpha(p),blend(p),separate(p),suppression(gate){}
 auto& depthCache(){return depth;}auto& alphaCache(){return alpha;}auto& blendCache(){return blend;}auto& separateBlendCache(){return separate;}
 void render(unsigned key,unsigned value){commands.insert(commands.end(),{key,value});}
 int samplerCount(){return 0;}void sampler(unsigned stage,unsigned key,unsigned value){samplers.insert(samplers.end(),{stage,key,value});}unsigned specialSamplerEnabled(){return 0;}
 unsigned suppressed(){return suppression;}
 void transition(int kind,unsigned offset,unsigned count){assert(kind==0&&offset==1&&count==(all?15u:4u));++transitions;const unsigned char operations[]{255,0,3,6,7};const unsigned char allOperations[]{255,0,1,2,3,4,5,6,7,8,9,10,11,12,0x8f,0x9f};studioTransitionBackendDispatchCandidate(&model,(all?allOperations:operations)+offset,static_cast<int>(count),*this);}
 void commit(){++commits;studioRestoreBlendCommitCandidate(parent,*this);}
 void vertexShader(int v){assert(v==-1);++shaderCalls;}void pixelShader(int v){assert(v==-1);++shaderCalls;}
 std::uint8_t force144(){return parent.f144;}void publishCache140(unsigned value){parent.commit.cached140=value;}
 unsigned capability41(){return 1;}int capability17(){return 92;}unsigned capability25(){return 1;}unsigned specialCapability(){return 1;}int stageCount(){return 1;}
 void applyStage(StudioTransitionState72Candidate* state,int stage){assert(state==&model);backend.insert(backend.end(),{262,static_cast<unsigned>(stage)});}void applyFlag44(unsigned v){backend.insert(backend.end(),{263,v});}void applyFlag45(unsigned v){backend.insert(backend.end(),{272,v});}void applyBackend271(unsigned v){backend.insert(backend.end(),{271,v});}void applyCombined(unsigned v,unsigned a,unsigned b,unsigned c){backend.insert(backend.end(),{264,v,a,b,c});}
 void applyDepthBackend(StudioTransitionState72Candidate& state){assert(&state==&model);++depthCalls;}
};
int main(){unsigned cases=0;for(unsigned gate:{0u,1u,256u}){
 StudioRestoreBlendStateCandidate parent;parent.kind=0;parent.cachedKind=9;parent.cachedHandle=12;parent.descriptor=0x04000001;StudioTransitionState72Candidate model{};model.flags44=255;model.flags45=1;model.values[0]=3;model.values[1]=1;model.values[4]=model.values[7]=model.values[10]=7;model.values[5]=model.values[8]=model.values[11]=13;model.values[6]=model.values[9]=17;
 Services services(parent,model,gate);studioRestoreBlendCandidate(parent,services);const bool active=gate!=1;
 const std::vector<unsigned> initial{27,0,19,2,20,1,171,1,206,0,207,2,208,1,209,1,7,1,23,4,15,0,25,7,24,0};
 auto expected=initial;if(active){const std::vector<unsigned> tail{23,3,15,1,24,13,27,1,19,7,20,13,171,17,206,1,207,7,208,13,209,17};expected.insert(expected.end(),tail.begin(),tail.end());}
 assert(services.commands==expected&&services.transitions==unsigned(active)&&services.commits==unsigned(active)&&services.depthCalls==unsigned(active)&&services.shaderCalls==2);
 assert(parent.cachedKind==(active?0:9)&&parent.cachedHandle==-1&&parent.f11c==1&&parent.f120==(active?3u:4u));
 assert(parent.ffc==unsigned(active)&&parent.f10c==unsigned(active)&&parent.f128==unsigned(active)&&parent.f124==(active?3u:0u));++cases;
 }
 StudioRestoreBlendStateCandidate parent;parent.descriptor=0x0f000001;StudioTransitionState72Candidate model{};model.flags44=255;model.flags45=13;model.values[0]=3;model.values[1]=1;model.values[2]=17;model.values[3]=23;model.value40=17;model.values[4]=model.values[7]=model.values[10]=7;model.values[5]=model.values[8]=model.values[11]=13;model.values[6]=model.values[9]=17;model.values[15]=0x06000000;
 Services services(parent,model,0);services.all=true;studioRestoreBlendCandidate(parent,services);
 const std::vector<unsigned> expected{27,0,19,2,20,1,171,1,206,0,207,2,208,1,209,1,7,1,23,4,15,0,25,7,24,0,23,3,14,1,168,17,15,1,24,13,8,23,194,0,27,1,19,7,20,13,171,17,206,1,207,7,208,13,209,17};
 assert(services.commands==expected&&services.samplers==std::vector<unsigned>({15,11,1,15,8,0x34544547}));
 assert(services.backend==std::vector<unsigned>({271,1,264,17,1,1,1,263,1,264,17,1,1,1,262,0,264,17,1,1,1,272,1}));
 assert(services.depthCalls==1&&services.transitions==1&&services.commits==1&&services.shaderCalls==2&&parent.commit.cached140==17&&parent.cachedKind==0&&parent.cachedHandle==-1);++cases;
 std::printf("restore_blend_actual_transition_composition_cases=%u passed\n",cases);return 0;}


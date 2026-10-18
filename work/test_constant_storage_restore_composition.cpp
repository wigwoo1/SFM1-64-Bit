#include "studio_constant_storage_restore_candidate.h"
#include "native_scalar_constant_submit_candidate.h"
#include "studio_constant_restore_defaults_candidate.h"
#include "native_shader_binding_state_candidate.h"
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <initializer_list>
struct Shader{};
struct Device{
 std::vector<unsigned>& events;
 template<class T>HRESULT constants(unsigned opcode,UINT,const T*,UINT){events.push_back(opcode);return E_FAIL;}
 HRESULT SetPixelShaderConstantF(UINT a,const float* b,UINT c){return constants(7,a,b,c);}HRESULT SetVertexShaderConstantF(UINT a,const float* b,UINT c){return constants(10,a,b,c);}
 HRESULT SetPixelShaderConstantB(UINT a,const BOOL* b,UINT c){return constants(8,a,b,c);}HRESULT SetVertexShaderConstantB(UINT a,const BOOL* b,UINT c){return constants(11,a,b,c);}
 HRESULT SetPixelShaderConstantI(UINT a,const int* b,UINT c){return constants(9,a,b,c);}HRESULT SetVertexShaderConstantI(UINT a,const int* b,UINT c){return constants(12,a,b,c);}
 HRESULT SetVertexShader(Shader* p){assert(!p);events.push_back(6);return E_FAIL;}HRESULT SetPixelShader(Shader* p){assert(!p);events.push_back(5);return E_FAIL;}
};
struct Capability{std::array<uint32_t,6> counts{4,2,2,256,2,2};uint32_t count(unsigned id){return counts[id];}};
struct Lifecycle{
 StudioConstantStorageOwnerCandidate& owner;Capability caps;
 Capability* capability(){return &caps;}uint32_t* staging(unsigned id){return static_cast<uint32_t*>(owner.staging[id]);}uint32_t* cache(unsigned id){return static_cast<uint32_t*>(owner.primary[id]);}
 void refill(StudioDeviceQueueStateCandidate&){assert(false);}
};
struct Allocation{
 Capability& caps;std::vector<unsigned>& events;unsigned id=0,frees=0,allocs=0;
 void free(void* p){++frees;std::free(p);}uint32_t count(unsigned value){id=value;return caps.count(id);}
 void* allocate(uint32_t bytes){++allocs;events.push_back(100+id);auto* p=std::malloc(bytes);assert(p);std::memset(p,0xaa,bytes);return p;}
};
using Initialization=NativeCompleteConstantInitializationServicesCandidateT<Device,Lifecycle>;
struct BindingLifecycle{
 Device*& current;uint32_t& queued;std::vector<unsigned>& events;
 void recordBind(bool){}Device** commandState(){return &current;}bool queueEnabled(Device**){return queued!=0;}Device* device(Device** p){return *p;}
 void queueShader(Device**,bool vertex,Shader* p){assert(!p);events.push_back(vertex?6:5);}
};
struct Tail{
 Lifecycle& lifecycle;Initialization& initialize;BindingLifecycle& bindings;
 uint32_t* primaryCache(){return lifecycle.cache(3);}uint32_t* secondaryCache(){return lifecycle.staging(3);}
 void submitVertex(unsigned index,const std::array<uint32_t,4>& words){initialize.submitFloat(true,index,1,const_cast<uint32_t*>(words.data()));}
 void restoreVertexDefaults(uint32_t bits){studioConstantRestoreDefaultsCandidate(bits,*this);}
 void unbindShaders(){NativeShaderBindingStateCandidateT<Shader> vertex,pixel;nativeShaderRestoreUnbindCandidate(vertex,pixel,bindings);assert(vertex.valid&&pixel.valid&&!vertex.shader&&!pixel.shader);}
};
int main(){unsigned cases=0;for(bool queuedMode:{false,true})for(bool present:{false,true}){
 StudioConstantStorageOwnerCandidate owner;std::vector<unsigned> events;Lifecycle lifecycle{owner};Allocation allocation{lifecycle.caps,events};if(present)for(unsigned id=0;id<6;++id){owner.primary[id]=std::malloc(16);owner.staging[id]=std::malloc(16);assert(owner.primary[id]&&owner.staging[id]);}
 Device object{events};Device* device=&object;uint32_t queued=queuedMode?1:0;std::array<uint32_t,4096> block{};StudioDeviceQueueStateCandidate queue{block.data(),block.data()+1,4095};Initialization initialize{device,queued,queue,lifecycle};BindingLifecycle bindings{device,queued,events};Tail tail{lifecycle,initialize,bindings};studioConstantStorageRestoreCandidate(owner,allocation,initialize,tail);assert(allocation.allocs==12&&allocation.frees==(present?12u:0u));
 for(unsigned id=0;id<6;++id){const unsigned width=id==1||id==4?1:4;for(unsigned i=0;i<lifecycle.caps.count(id)*width;++i){const auto value=lifecycle.cache(id)[i];assert(value==lifecycle.staging(id)[i]);if(id!=3)assert(value==0);}}
 assert(lifecycle.cache(3)[1]==0x3f800000&&lifecycle.cache(3)[7]==0x3f000000&&lifecycle.cache(3)[12]==0x41d80000&&lifecycle.cache(3)[900]==0);
 if(!queuedMode){const std::vector<unsigned> expected{100,100,101,101,102,102,7,8,9,103,103,104,104,105,105,10,11,12,10,10,10,6,5};assert(events==expected);}else{const std::vector<unsigned> expected{100,100,101,101,102,102,103,103,104,104,105,105,6,5};assert(events==expected);const unsigned opcodes[]={7,8,9,10,11,12,10,10,10};auto* cursor=block.data()+1;for(unsigned op:opcodes){assert(cursor[0]==op);const unsigned width=op==8||op==11?1:4;cursor+=3+cursor[2]*width;}assert(cursor==queue.cursor&&queue.remaining==4095-static_cast<uint32_t>(cursor-block.data()-1));}
 for(unsigned id=0;id<6;++id){std::free(owner.primary[id]);std::free(owner.staging[id]);}++cases;
 }std::printf("constant_restore_composition_cases=%u passed\n",cases);}

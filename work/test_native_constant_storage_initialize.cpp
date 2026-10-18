#include "native_constant_storage_initialize_candidate.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <initializer_list>
struct Capability{uint32_t amount;uint32_t count(unsigned){return amount;}};
struct Device{unsigned calls=0;bool expectedVertex;uint32_t expectedCount;HRESULT SetVertexShaderConstantF(UINT start,const float* data,UINT count){assert(expectedVertex);return check(start,data,count);}HRESULT SetPixelShaderConstantF(UINT start,const float* data,UINT count){assert(!expectedVertex);return check(start,data,count);}HRESULT check(UINT start,const float* data,UINT count){assert(start==0&&count==expectedCount);for(unsigned i=0;i<count*4;++i)assert(data[i]==0);++calls;return E_FAIL;}};
struct Lifecycle{
 Capability caps;std::array<std::array<uint32_t,16>,6> buffers,caches;
 std::array<uint32_t,64> replacement{};unsigned refills=0,submissions=0;
 explicit Lifecycle(uint32_t count):caps{count}{for(auto& b:buffers)b.fill(0xdeadbeef);for(auto& c:caches)c.fill(0xcafebabe);}
 Capability* capability(){return &caps;}uint32_t* staging(unsigned id){return buffers[id].data();}uint32_t* cache(unsigned id){return caches[id].data();}
 void submitSubobject(unsigned,uint32_t start,uint32_t* data,uint32_t count,uint32_t flag){assert(start==0&&count==caps.amount&&flag==1);assert(count==0||data[0]==0);++submissions;}
 void refill(StudioDeviceQueueStateCandidate& state){++refills;state.block=replacement.data();state.cursor=state.block+1;state.remaining=63;}
};
// Compile the same bridge against the actual SDK interface without GPU calls.
void compileSdkBridge(IDirect3DDevice9*& device,uint32_t& queued,StudioDeviceQueueStateCandidate& queue,Lifecycle& lifecycle){NativeConstantStorageInitializeServicesCandidateT<IDirect3DDevice9,Lifecycle> services{device,queued,queue,lifecycle};studioConstantStorageInitializeGroupCandidate(false,services);}
int main(){unsigned cases=0;for(bool vertex:{false,true})for(uint32_t count:{0u,1u,4u})for(bool queuedMode:{false,true}){
 Lifecycle lifecycle(count);Device object{0,vertex,count};Device* device=&object;uint32_t queued=queuedMode?1u:0u;std::array<uint32_t,64> block{};StudioDeviceQueueStateCandidate queue{block.data(),block.data()+1,8};NativeConstantStorageInitializeServicesCandidateT<Device,Lifecycle> services{device,queued,queue,lifecycle};studioConstantStorageInitializeGroupCandidate(vertex,services);
 assert(object.calls==(queuedMode?0u:1u));assert(lifecycle.submissions==(vertex||count?2u:0u));assert(lifecycle.refills==(queuedMode&&count==4?1u:0u));
 const unsigned id=vertex?3:0;for(unsigned i=0;i<count*4;++i)assert(lifecycle.caches[id][i]==0);assert(lifecycle.caches[id][count*4<16?count*4:15]==(count==4?0u:0xcafebabe));
 if(queuedMode){auto* output=count==4?lifecycle.replacement.data():block.data();assert(output[1]==(vertex?10u:7u)&&output[2]==0&&output[3]==count);for(unsigned i=0;i<count*4;++i)assert(output[4+i]==0);assert(queue.cursor==output+4+count*4);assert(queue.remaining==(count==4?63u:8u)-(count*4+3));}++cases;
 }std::printf("typed_constant_initialization_cases=%u passed\n",cases);}

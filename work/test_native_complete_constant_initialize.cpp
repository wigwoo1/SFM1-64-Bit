#include "native_scalar_constant_submit_candidate.h"
#include <array>
#include <vector>
#include <cstdio>
#include <initializer_list>
struct Capability{uint32_t amount;uint32_t count(unsigned){return amount;}};
struct Lifecycle{
 Capability caps;std::array<std::array<uint32_t,16>,6> buffers,caches;
 explicit Lifecycle(uint32_t count):caps{count}{for(auto& b:buffers)b.fill(0xdeadbeef);for(auto& c:caches)c.fill(0xcafebabe);}
 Capability* capability(){return &caps;}uint32_t* staging(unsigned id){return buffers[id].data();}uint32_t* cache(unsigned id){return caches[id].data();}
 void refill(StudioDeviceQueueStateCandidate&){assert(false);}
};
struct Device{
 std::vector<unsigned> trace;HRESULT result;
 template<class T>HRESULT check(unsigned op,UINT start,const T* values,UINT count,unsigned width){assert(start==0);for(unsigned i=0;i<count*width;++i)assert(values[i]==0);trace.push_back(op);return result;}
 HRESULT SetVertexShaderConstantF(UINT start,const float* p,UINT n){return check(10,start,p,n,4);}HRESULT SetPixelShaderConstantF(UINT start,const float* p,UINT n){return check(7,start,p,n,4);}
 HRESULT SetVertexShaderConstantB(UINT start,const BOOL* p,UINT n){return check(11,start,p,n,1);}HRESULT SetPixelShaderConstantB(UINT start,const BOOL* p,UINT n){return check(8,start,p,n,1);}
 HRESULT SetVertexShaderConstantI(UINT start,const int* p,UINT n){return check(12,start,p,n,4);}HRESULT SetPixelShaderConstantI(UINT start,const int* p,UINT n){return check(9,start,p,n,4);}
};
void compileCompleteSdkBridge(IDirect3DDevice9*& device,uint32_t& queued,StudioDeviceQueueStateCandidate& queue,Lifecycle& lifecycle){NativeCompleteConstantInitializationServicesCandidateT<IDirect3DDevice9,Lifecycle> services{device,queued,queue,lifecycle};studioConstantStorageInitializeGroupCandidate(false,services);studioConstantStorageInitializeGroupCandidate(true,services);}
int main(){unsigned cases=0;for(bool vertex:{false,true})for(uint32_t count:{0u,1u,4u})for(bool queuedMode:{false,true})for(HRESULT result:{S_OK,E_FAIL}){
 Lifecycle lifecycle(count);Device object{{},result};Device* device=&object;uint32_t queued=queuedMode?1u:0u;std::array<uint32_t,128> block{};StudioDeviceQueueStateCandidate queue{block.data(),block.data()+1,127};NativeCompleteConstantInitializationServicesCandidateT<Device,Lifecycle> services{device,queued,queue,lifecycle};studioConstantStorageInitializeGroupCandidate(vertex,services);
 const unsigned first=vertex?3:0;const bool scalar=vertex||count!=0;const std::array<unsigned,3> opcodes=vertex?std::array<unsigned,3>{10,11,12}:std::array<unsigned,3>{7,8,9};unsigned consumed=0;
 for(unsigned kind=0;kind<(scalar?3u:1u);++kind){const unsigned width=kind==1?1:4;const unsigned words=count*width;for(unsigned i=0;i<words;++i)assert(lifecycle.caches[first+kind][i]==0);if(words<16)assert(lifecycle.caches[first+kind][words]==0xcafebabe);
  if(queuedMode){const auto* command=block.data()+1+consumed;assert(command[0]==opcodes[kind]&&command[1]==0&&command[2]==count);for(unsigned i=0;i<words;++i)assert(command[i+3]==0);consumed+=words+3;}
 }
 if(queuedMode){assert(object.trace.empty()&&queue.remaining==127-consumed&&queue.cursor==block.data()+1+consumed);}else{assert(object.trace.size()==(scalar?3u:1u));for(unsigned i=0;i<object.trace.size();++i)assert(object.trace[i]==opcodes[i]);}++cases;
 }std::printf("complete_constant_initialization_cases=%u passed\n",cases);}

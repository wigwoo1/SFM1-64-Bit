#include "native_shader_binding_state_candidate.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <vector>
struct Shader{};
struct Device{
 unsigned calls=0;bool lastVertex=false;Shader* value=nullptr;
 HRESULT SetVertexShader(Shader* p){++calls;lastVertex=true;value=p;return E_FAIL;}
 HRESULT SetPixelShader(Shader* p){++calls;lastVertex=false;value=p;return E_FAIL;}
};
struct Lifecycle{
 Device first,second;Device* current=&first;bool queued,swap;unsigned counters=0,captures=0;
 struct Entry{bool vertex;Shader* pointer;};std::vector<Entry> commands;
 Lifecycle(bool q,bool s):queued(q),swap(s){}
 void recordBind(bool){++counters;if(swap)current=&second;}
 Device** commandState(){++captures;return &current;}
 bool queueEnabled(Device**){return queued;}Device* device(Device** state){return *state;}
 void queueShader(Device**,bool vertex,Shader* value){commands.push_back({vertex,value});}
};
struct SdkLifecycle{
 IDirect3DDevice9* current;void recordBind(bool){}IDirect3DDevice9** commandState(){return &current;}
 bool queueEnabled(IDirect3DDevice9**){return false;}IDirect3DDevice9* device(IDirect3DDevice9** state){return *state;}
 template<class Shader>void queueShader(IDirect3DDevice9**,bool,Shader*){}
};
void compileSdkBinding(NativeShaderBindingStateCandidateT<IDirect3DVertexShader9>& vertex,NativeShaderBindingStateCandidateT<IDirect3DPixelShader9>& pixel,SdkLifecycle& lifecycle){nativeShaderRestoreUnbindCandidate(vertex,pixel,lifecycle);}
int main(){unsigned cases=0;auto* high=reinterpret_cast<Shader*>(uintptr_t{0x100001234ull});
 for(bool vertex:{false,true})for(bool queued:{false,true})for(bool swap:{false,true})for(unsigned initial=0;initial<3;++initial){
  NativeShaderBindingStateCandidateT<Shader> cache{initial==1?high:nullptr,initial!=2};Lifecycle lifecycle(queued,swap);const bool expected=initial!=1;const bool submitted=vertex?nativeShaderBindingCandidate<true>(cache,high,lifecycle):nativeShaderBindingCandidate<false>(cache,high,lifecycle);assert(submitted==expected&&cache.valid&&cache.shader==high);assert(lifecycle.counters==(expected?1u:0u)&&lifecycle.captures==(expected?1u:0u));
  if(expected&&queued){assert(lifecycle.commands.size()==1&&lifecycle.commands[0].vertex==vertex&&lifecycle.commands[0].pointer==high);}else if(expected){auto& device=swap?lifecycle.second:lifecycle.first;assert(device.calls==1&&device.lastVertex==vertex&&device.value==high);}assert(lifecycle.first.calls+lifecycle.second.calls==(expected&&!queued?1u:0u));++cases;
 }
 for(bool queued:{false,true}){NativeShaderBindingStateCandidateT<Shader> vertex{nullptr,true},pixel{nullptr,true};Lifecycle lifecycle(queued,false);nativeShaderRestoreUnbindCandidate(vertex,pixel,lifecycle);assert(lifecycle.counters==2&&vertex.valid&&pixel.valid&&vertex.shader==nullptr&&pixel.shader==nullptr);if(queued)assert(lifecycle.commands.size()==2&&lifecycle.commands[0].vertex&&!lifecycle.commands[1].vertex);else assert(lifecycle.first.calls==2&&!lifecycle.first.lastVertex);}
 std::printf("native_shader_binding_cases=%u restore_cases=2 passed\n",cases);
}

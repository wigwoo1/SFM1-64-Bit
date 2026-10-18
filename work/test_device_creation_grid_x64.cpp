#include "studio_d3d9_creation_policy_candidate.h"
#include "native_d3d9_creation_attempts_candidate.h"
#include <array>
#include <fstream>
#include <cstdio>
struct Ops{
 int mode;unsigned cpu,success,calls=0,waits=0,cpuCalls=0,flags=0;bool initialized=false;
 D3DPRESENT_PARAMETERS parameters{};unsigned token=1;
 bool scripted=false,firstWrites=false,secondWrites=false;uint32_t firstResult=0,secondResult=0;
 bool cpuMutation=false;unsigned creationMutation=0;
 unsigned* initialOutput(){initialized=true;return nullptr;}
 int queueMode(){if(!initialized)throw 1;return mode;}
 unsigned cpuByte6(){++cpuCalls;if(cpuMutation){mode=-1;cpu=cpuCalls==1?1u:2u;}return cpu;}
 void configureFlags(uint32_t value){flags=value;}
 HRESULT create(unsigned*& output){++calls;
  if((creationMutation==1&&calls==1)||(creationMutation==2&&calls==2))parameters.Windowed=creationMutation==2?TRUE:FALSE;
  if(scripted){if(calls==1?firstWrites:secondWrites)output=&token;return static_cast<HRESULT>(calls==1?firstResult:secondResult);}
  if(calls==success){output=&token;return S_OK;}return E_FAIL;}
 void wait(unsigned ms){if(ms!=1000||calls!=1)throw 2;++waits;}
 bool fallbackEnabled(){return parameters.Windowed!=FALSE;}
 void configureFallback(){nativeD3d9PresentationFallbackCandidate(parameters);}
 void failure(){}
};
struct InterfaceState;
struct FakeApi{InterfaceState* state;unsigned identity;HRESULT CreateDevice(UINT,D3DDEVTYPE,HWND,DWORD,D3DPRESENT_PARAMETERS*,IDirect3DDevice9**);};
struct InterfaceState{FakeApi* current;FakeApi* replacement;unsigned success,calls=0,route=0,flags=0;bool replace;};
HRESULT FakeApi::CreateDevice(UINT adapter,D3DDEVTYPE type,HWND window,DWORD flags,D3DPRESENT_PARAMETERS*,IDirect3DDevice9** output){
 if(adapter!=3||type!=D3DDEVTYPE_HAL||window!=reinterpret_cast<HWND>(uintptr_t{0x100001234}))throw 11;
 auto& s=*state;++s.calls;s.route=s.route*10+identity;s.flags=flags;
 if(s.replace&&s.calls==1)s.current=s.replacement;
 if(s.calls==s.success){*output=reinterpret_cast<IDirect3DDevice9*>(uintptr_t{0x100005678});return S_OK;}return E_FAIL;
}
struct ControlledBridge:NativeD3d9CreationAttemptsBackendCandidateT<FakeApi>{
 void wait(unsigned ms){if(ms!=1000||attempts!=1)throw 12;++waits;}
};
struct BridgeFlagServices{int mode;unsigned cpuCalls=0;int queueMode(){return mode;}unsigned cpuByte6(){++cpuCalls;return 2;}};
int main(){static_assert(sizeof(void*)==8,"x64 required");
 std::ifstream oracle("outputs/native-device-creation-grid.bin",std::ios::binary);if(!oracle)return 1;
 unsigned cases=0;
 for(int mode:{-2,-1,0,1,2})for(unsigned cpu:{0u,1u,2u,255u})for(unsigned success=0;success<4;++success)for(unsigned fallback=0;fallback<2;++fallback){
  Ops ops{mode,cpu,success};ops.parameters.SwapEffect=D3DSWAPEFFECT_DISCARD;ops.parameters.BackBufferCount=1;ops.parameters.Windowed=fallback;
  auto* result=studioD3D9CreationPolicyCandidate(0x40,ops,ops);
  std::array<unsigned,7> record{ops.calls,ops.waits,ops.cpuCalls,ops.flags,result?1u:0u,static_cast<unsigned>(ops.parameters.SwapEffect),ops.parameters.BackBufferCount},native{};
  oracle.read(reinterpret_cast<char*>(native.data()),sizeof(native));if(!oracle)return 2;
  if(record!=native){std::printf("creation_native_mismatch_case=%u\n",cases);return 3;}++cases;
 }
 if(oracle.peek()!=std::char_traits<char>::eof())return 4;
 std::printf("x64_native_device_creation_record_matches=%u output_init_before_services=passed\n",cases);
 std::ifstream boundary("outputs/native-device-creation-boundary-grid.bin",std::ios::binary);if(!boundary)return 5;
 unsigned boundaries=0;
 for(uint32_t first:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu})for(bool firstWrites:{false,true})
 for(uint32_t second:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu})for(bool secondWrites:{false,true}){
  Ops ops{0,0,0};ops.scripted=true;ops.firstResult=first;ops.secondResult=second;ops.firstWrites=firstWrites;ops.secondWrites=secondWrites;
  ops.parameters.SwapEffect=D3DSWAPEFFECT_DISCARD;ops.parameters.BackBufferCount=1;
  auto* result=studioD3D9CreationPolicyCandidate(0x40,ops,ops);
  std::array<unsigned,7> record{ops.calls,ops.waits,ops.cpuCalls,ops.flags,result?1u:0u,static_cast<unsigned>(ops.parameters.SwapEffect),ops.parameters.BackBufferCount},native{};
  boundary.read(reinterpret_cast<char*>(native.data()),sizeof(native));if(!boundary||record!=native)return 6;++boundaries;
 }
 if(boundary.peek()!=std::char_traits<char>::eof())return 7;
 std::printf("x64_native_hresult_output_boundary_matches=%u\n",boundaries);
 std::ifstream mutations("outputs/native-device-creation-mutation-grid.bin",std::ios::binary);if(!mutations)return 8;
 unsigned mutationCases=0;
 for(unsigned cpuChange=0;cpuChange<2;++cpuChange)for(unsigned change=0;change<3;++change)for(unsigned windowed=0;windowed<2;++windowed)for(unsigned success=0;success<4;++success){
  Ops ops{-2,1,success};ops.cpuMutation=cpuChange!=0;ops.creationMutation=change;
  ops.parameters.SwapEffect=D3DSWAPEFFECT_DISCARD;ops.parameters.BackBufferCount=1;ops.parameters.Windowed=windowed;
  auto* result=studioD3D9CreationPolicyCandidate(0x40,ops,ops);
  std::array<unsigned,9> record{ops.calls,ops.waits,ops.cpuCalls,ops.flags,result?1u:0u,static_cast<unsigned>(ops.parameters.SwapEffect),ops.parameters.BackBufferCount,static_cast<unsigned>(ops.parameters.Windowed),static_cast<unsigned>(ops.mode)},native{};
  mutations.read(reinterpret_cast<char*>(native.data()),sizeof(native));if(!mutations||record!=native)return 9;++mutationCases;
 }
 if(mutations.peek()!=std::char_traits<char>::eof())return 10;
 std::printf("x64_native_creation_callback_mutation_matches=%u\n",mutationCases);
 std::ifstream interfaces("outputs/native-device-creation-interface-grid.bin",std::ios::binary);if(!interfaces)return 11;
 unsigned interfaceCases=0;
 for(int mode:{-2,-1,0,2})for(bool replace:{false,true})for(unsigned success=0;success<4;++success){
  InterfaceState state{};FakeApi first{&state,1},second{&state,2};state.current=&first;state.replacement=&second;state.success=success;state.replace=replace;
  D3DPRESENT_PARAMETERS parameters{};parameters.SwapEffect=D3DSWAPEFFECT_DISCARD;parameters.BackBufferCount=1;parameters.Windowed=TRUE;
  ControlledBridge bridge{{state.current,3,D3DDEVTYPE_HAL,reinterpret_cast<HWND>(uintptr_t{0x100001234}),0x40,parameters}};
  BridgeFlagServices services{mode};auto* result=studioD3D9CreationPolicyCandidate(0x40,services,bridge);
  if(result&&result!=reinterpret_cast<IDirect3DDevice9*>(uintptr_t{0x100005678}))return 12;
  std::array<unsigned,8> record{bridge.attempts,bridge.waits,services.cpuCalls,state.flags,result?1u:0u,static_cast<unsigned>(parameters.SwapEffect),parameters.BackBufferCount,state.route},native{};
  interfaces.read(reinterpret_cast<char*>(native.data()),sizeof(native));if(!interfaces||record!=native)return 13;++interfaceCases;
 }
 if(interfaces.peek()!=std::char_traits<char>::eof())return 14;
 std::printf("x64_native_interface_replacement_bridge_matches=%u fullwidth_transport=passed gpu_executed=0\n",interfaceCases);
}

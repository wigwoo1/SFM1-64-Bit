#include "studio_d3d9_factory_lifecycle_candidate.h"
#include "studio_d3d9_creation_attempts_candidate.h"
#include "native_d3d9_creation_attempts_candidate.h"
#include "studio_d3d9_creation_flags_candidate.h"
#include <cstring>
#include <array>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
struct Interface{unsigned id;};
struct Backend{
 Interface*& current;Interface& result;Interface& replacement;
 unsigned setupResult=1,creates=0,warnings=0,releases=0,cleanups=0;
 bool fail=false,mutate=false;
 unsigned baseSetup(){return setupResult;}
 Interface* create(unsigned version){if(version!=32)throw 1;++creates;return fail?nullptr:&result;}
 void factoryFailure(){if(current)throw 2;++warnings;}
 void release(Interface* value){if(value!=&result||current!=value)throw 3;++releases;if(mutate)current=&replacement;}
 void baseCleanup(){if(current)throw 4;++cleanups;}
};
int main(){static_assert(sizeof(void*)==8,"x64 required");unsigned cases=0;
 for(unsigned setup:{0u,0x100u,1u,0x101u})for(bool fail:{false,true}){
  Interface result{1},replacement{2};Interface* current=&replacement;
  Backend backend{current,result,replacement};backend.setupResult=setup;backend.fail=fail;
  const bool success=studioD3D9FactorySetupCandidate(current,backend);
  const bool entered=static_cast<unsigned char>(setup)!=0;
  if(success!=(entered&&!fail)||backend.creates!=unsigned(entered)||backend.warnings!=unsigned(entered&&fail))return 1;
  if(current!=(entered?(fail?nullptr:&result):&replacement))return 2;
  ++cases;
 }
 for(bool present:{false,true})for(bool mutate:{false,true}){
  Interface result{1},replacement{2};Interface* current=present?&result:nullptr;
  Backend backend{current,result,replacement};backend.mutate=mutate;
  studioD3D9FactoryReleaseCandidate(current,backend);
  if(current||backend.releases!=unsigned(present)||backend.cleanups!=1)return 3;++cases;
 }
 std::printf("x64_factory_lifecycle_cases=%u lowbyte_setup_publish_before_warning_release_before_clear=1\n",cases);
 unsigned attemptsCases=0;
 for(unsigned success=0;success<4;++success)for(bool fallback:{false,true}){
  struct Attempts{
   Interface value{3};unsigned success,calls=0,waits=0,configured=0,failures=0;bool fallback;
   Interface* initialOutput(){return nullptr;}
   int32_t create(Interface*& output){++calls;if(calls==success){output=&value;return 0;}return -1;}
   void wait(unsigned ms){if(ms!=1000||calls!=1)throw 5;++waits;}
   bool fallbackEnabled(){return fallback;}
   void configureFallback(){if(calls!=2)throw 6;++configured;}
   void failure(){++failures;}
  }backend{{3},success,0,0,0,0,fallback};
  auto* result=studioD3D9CreationAttemptsCandidate(backend);
  const bool passed=success==1||success==2||(success==3&&fallback);
  const unsigned expected=success==1?1u:success==2?2u:fallback?3u:2u;
  if(bool(result)!=passed||backend.calls!=expected||backend.waits!=unsigned(success!=1)||backend.configured!=unsigned(success!=1&&success!=2&&fallback)||backend.failures!=unsigned(!passed))return 4;
  ++attemptsCases;
 }
 std::printf("x64_creation_attempt_policy_cases=%u\n",attemptsCases);
 unsigned boundaryCases=0;
 for(uint32_t first:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu})for(bool firstWrites:{false,true})
 for(uint32_t second:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu})for(bool secondWrites:{false,true}){
  struct Boundary{
   Interface value{4};uint32_t first,second;bool firstWrites,secondWrites;unsigned calls=0,waits=0,failures=0;
   Interface* initialOutput(){return nullptr;}
   uint32_t create(Interface*& output){++calls;if(calls==1?firstWrites:secondWrites)output=&value;return calls==1?first:second;}
   void wait(unsigned ms){if(ms!=1000)throw 7;++waits;}
   bool fallbackEnabled(){return false;}void configureFallback(){throw 8;}
   void failure(){++failures;}
  }backend{{4},first,second,firstWrites,secondWrites};
  auto* result=studioD3D9CreationAttemptsCandidate(backend);
  const bool immediate=first<0x80000000u&&firstWrites;
  const bool passed=immediate||(second<0x80000000u&&(firstWrites||secondWrites));
  if(bool(result)!=passed||backend.calls!=(immediate?1u:2u)||backend.waits!=unsigned(!immediate)||backend.failures!=unsigned(!passed))return 5;
  ++boundaryCases;
 }
 std::printf("creation_hresult_and_retained_output_cases=%u\n",boundaryCases);
 D3DPRESENT_PARAMETERS parameters{};parameters.BackBufferWidth=256;parameters.BackBufferFormat=D3DFMT_X8R8G8B8;
 parameters.BackBufferCount=1;parameters.SwapEffect=D3DSWAPEFFECT_DISCARD;parameters.Windowed=TRUE;
 parameters.hDeviceWindow=reinterpret_cast<HWND>(uintptr_t(0x100001234ull));
 auto expected=parameters;expected.BackBufferCount=0;expected.SwapEffect=D3DSWAPEFFECT_COPY;
 nativeD3d9PresentationFallbackCandidate(parameters);
 if(std::memcmp(&parameters,&expected,sizeof(parameters)))return 6;
 std::puts("x64_presentation_fallback_preserves_fullwidth_window_and_other_fields=passed");
 unsigned flagCases=0;
 for(int mode:{-2,-1,0,1,2})for(unsigned cpu:{0u,1u,2u,255u})for(uint32_t flags:{0u,4u,0x40u}){
  struct Services{int mode;unsigned cpu,modeReads=0,cpuReads=0;
   int queueMode(){++modeReads;return mode;}unsigned cpuByte6(){++cpuReads;return cpu;}
  }services{mode,cpu};
  const auto result=studioD3D9CreationFlagsCandidate(flags,services);
  const bool threaded=mode==2||((mode==-2||mode==-1)&&cpu>=2);
  const unsigned reads=mode==-2||mode==-1?1u:0u;
  if(result!=(threaded?flags|4u:flags)||services.cpuReads!=reads||services.modeReads!=(mode==2||(mode==-2&&cpu>=2)?1u:2u))return 7;
  ++flagCases;
 }
 struct MutatingFlags{int mode=-2;unsigned calls=0;int queueMode(){return mode;}
  unsigned cpuByte6(){++calls;mode=-1;return calls==1?1u:2u;}
 }mutating;
 if(studioD3D9CreationFlagsCandidate(0x40,mutating)!=0x44||mutating.calls!=2)return 8;
 std::printf("creation_multithread_flag_cases=%u live_mode_mutation=passed\n",flagCases);
 struct FakeApi{
  FakeApi*& slot;FakeApi* replacement=nullptr;bool succeed=false;unsigned calls=0;
  HRESULT CreateDevice(UINT adapter,D3DDEVTYPE type,HWND window,DWORD flags,D3DPRESENT_PARAMETERS*,IDirect3DDevice9** output){
   if(adapter!=3||type!=D3DDEVTYPE_HAL||reinterpret_cast<uintptr_t>(window)!=0x100001234ull||flags!=0x44)throw 9;
   ++calls;if(replacement)slot=replacement;
   if(succeed){*output=reinterpret_cast<IDirect3DDevice9*>(uintptr_t(0x100005678ull));return S_OK;}return E_FAIL;
  }
 };
 FakeApi* slot=nullptr;FakeApi first{slot},second{slot};first.replacement=&second;second.succeed=true;slot=&first;
 NativeD3d9CreationAttemptsBackendCandidateT<FakeApi> bridge{slot,3,D3DDEVTYPE_HAL,parameters.hDeviceWindow,0x44,parameters};
 auto* device=bridge.initialOutput();
 if(SUCCEEDED(bridge.create(device))||slot!=&second||device)return 9;
 if(FAILED(bridge.create(device))||first.calls!=1||second.calls!=1||bridge.attempts!=2||reinterpret_cast<uintptr_t>(device)!=0x100005678ull)return 10;
 std::puts("creation_bridge_live_interface_replacement_fullwidth_transport=passed");
}

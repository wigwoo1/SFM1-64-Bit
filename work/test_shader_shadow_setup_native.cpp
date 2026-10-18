#include <windows.h>
#include <iostream>
#include <vector>
#include <array>
#include "studio_shader_shadow_setup_candidate.h"
#include "shader_shadow_setup_native_bytes.h"
struct Fixture {
 StudioBaseShaderDrawStateCandidate state;uintptr_t objects[10]{},table[25]{};
 uint32_t parameterA[3]{},parameterB[3]{};uintptr_t paramsA=0,paramsB=0;
 std::vector<std::array<uint32_t,4>> calls;unsigned current=0;unsigned char* globals=nullptr;
 void sync(){if(globals){*reinterpret_cast<uint32_t*>(globals)=uint32_t(state.shadow);*reinterpret_cast<uint32_t*>(globals+4)=uint32_t(state.parameters);}}
 void reset(uintptr_t p){if(p!=reinterpret_cast<uintptr_t>(&objects[0]))throw 1;calls.push_back({0,0,0,0});current=1;state.shadow=reinterpret_cast<uintptr_t>(&objects[current]);state.parameters=reinterpret_cast<uintptr_t>(&paramsB);sync();}
 uint32_t primaryFlags(uintptr_t p){if(p!=reinterpret_cast<uintptr_t>(&paramsB))throw 2;return parameterB[2];}
 void record(uintptr_t p,uint32_t slot,uint32_t a,uint32_t b){if(p!=reinterpret_cast<uintptr_t>(&objects[current]))throw 3;calls.push_back({slot,a,b,current});++current;state.shadow=reinterpret_cast<uintptr_t>(&objects[current]);parameterB[2]=~parameterB[2];sync();}
 void one(uintptr_t p,uint32_t slot,uint32_t a){record(p,slot,a,0xffffffff);}
 void two(uintptr_t p,uint32_t slot,uint32_t a,uint32_t b){record(p,slot,a,b);}
 void initialize(uint32_t flags){calls.clear();current=0;parameterA[2]=~flags;parameterB[2]=flags;paramsA=reinterpret_cast<uintptr_t>(parameterA);paramsB=reinterpret_cast<uintptr_t>(parameterB);for(auto& object:objects)object=reinterpret_cast<uintptr_t>(table);state.shadow=reinterpret_cast<uintptr_t>(&objects[0]);state.parameters=reinterpret_cast<uintptr_t>(&paramsA);sync();}
};
#ifdef _M_IX86
static Fixture* active;
static void __fastcall reset(void* p,void*){active->reset(reinterpret_cast<uintptr_t>(p));}
template<unsigned Slot>static void __fastcall one(void* p,void*,uint32_t a){active->one(reinterpret_cast<uintptr_t>(p),Slot,a);}
static void __fastcall two(void* p,void*,uint32_t a,uint32_t b){active->two(reinterpret_cast<uintptr_t>(p),14,a,b);}
#endif
int main(){
#ifdef _M_IX86
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,8192,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 1;
 memcpy(code,shaderShadowSetupNativeBytes,sizeof(shaderShadowSetupNativeBytes));
 for(auto& patch:shaderShadowSetupPatches){auto pointer=uint32_t(reinterpret_cast<uintptr_t>(code+patch[1]));memcpy(code+patch[0],&pointer,4);}
 DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 2;FlushInstructionCache(GetCurrentProcess(),code,4096);
 auto native=reinterpret_cast<void(__cdecl*)()>(code);
#endif
 uint64_t digest=1469598103934665603ull;unsigned cases=0;
 for(unsigned combo=0;combo<64;++combo)for(unsigned seed=0;seed<1024;++seed){
  uint32_t flags=seed;unsigned masks[]={0x8000,0x10000,0x2000,0x400,0x10000000,0x20000000};for(unsigned i=0;i<6;++i)if(combo&(1u<<i))flags|=masks[i];
  Fixture candidate;candidate.initialize(flags);studioShaderShadowSetupCandidate(candidate.state,candidate);
#ifdef _M_IX86
  Fixture original;original.globals=code+4096;
  original.table[0]=reinterpret_cast<uintptr_t>(reset);original.table[1]=reinterpret_cast<uintptr_t>(one<1>);original.table[2]=reinterpret_cast<uintptr_t>(one<2>);original.table[3]=reinterpret_cast<uintptr_t>(one<3>);original.table[4]=reinterpret_cast<uintptr_t>(one<4>);original.table[15]=reinterpret_cast<uintptr_t>(one<15>);original.table[14]=reinterpret_cast<uintptr_t>(two);original.table[24]=reinterpret_cast<uintptr_t>(one<24>);
  original.initialize(flags);active=&original;native();
  if(candidate.calls!=original.calls||candidate.current!=original.current||candidate.parameterB[2]!=original.parameterB[2]||*reinterpret_cast<uint32_t*>(code+4096)!=original.state.shadow||*reinterpret_cast<uint32_t*>(code+4100)!=original.state.parameters)return 3;
#endif
  for(auto& call:candidate.calls)for(auto v:call){digest^=v;digest*=1099511628211ull;}++cases;
 }
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);
#endif
 std::cout<<"shadow_setup_native_cases="<<cases<<" digest="<<digest<<" receiver_mutations=1 flags_after_reset=1 cached_flags=1\n";
}

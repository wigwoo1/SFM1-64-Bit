#include <windows.h>
#include <array>
#include <cstring>
#include <iostream>
#include "native_srgb_sampler_state_candidate.h"
#include "srgb_sampler_oracle_bytes.h"
static bool supported;
static unsigned capabilityCalls,countCalls;
static bool __fastcall capability(void*,void*){++capabilityCalls;return supported;}
static int __fastcall count(void*,void*){++countCalls;return 3;}
int main(){
 void*code=VirtualAlloc(nullptr,sizeof(originalSrgbSamplerCode),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!code)return 2;
 std::memcpy(code,originalSrgbSamplerCode,sizeof(originalSrgbSamplerCode));DWORD old;
 if(!VirtualProtect(code,sizeof(originalSrgbSamplerCode),PAGE_EXECUTE_READ,&old)||!FlushInstructionCache(GetCurrentProcess(),code,sizeof(originalSrgbSamplerCode))){VirtualFree(code,0,MEM_RELEASE);return 3;}
 // Invalid-stage logging branch is deliberately unreachable; its external import
 // is not relocated. All tested stages are allocated and hardware-valid.
 auto native=reinterpret_cast<void(__thiscall*)(void*,int,unsigned char)>(code);
 std::array<uintptr_t,27>vtable{};vtable[1]=reinterpret_cast<uintptr_t>(&count);vtable[26]=reinterpret_cast<uintptr_t>(&capability);
 uintptr_t hardware=reinterpret_cast<uintptr_t>(vtable.data());
 std::array<unsigned char,256>receiver{};auto hardwarePointer=reinterpret_cast<uintptr_t>(&hardware);std::memcpy(receiver.data()+4,&hardwarePointer,4);
 unsigned cases=0;int result=0;
 for(unsigned initial=0;initial<256;++initial)for(unsigned enabled=0;enabled<256;++enabled)for(bool value:{false,true}){
  supported=value;capabilityCalls=countCalls=0;
  receiver[0x58]=0x55;receiver[0x59]=static_cast<unsigned char>(initial);receiver[0x5a]=0xaa;
  auto reference=std::array<uint8_t,3>{0x55,static_cast<uint8_t>(initial),0xaa};
  setNativeSrgbSamplerStateCandidate(reference.data(),reference.size(),1,static_cast<uint8_t>(enabled),supported,3);
  native(receiver.data(),1,static_cast<unsigned char>(enabled));
  if(std::memcmp(receiver.data()+0x58,reference.data(),3)||capabilityCalls!=1||countCalls!=(supported?1u:0u)){result=1;break;}++cases;
 }
 VirtualFree(code,0,MEM_RELEASE);
 if(!result)std::cout<<"PASS original x86 SRGBRead code versus candidate: "<<cases<<" valid bit/capability cases; capability queried once, count only when supported, adjacent bytes preserved; invalid logging untested\n";
 return result;
}

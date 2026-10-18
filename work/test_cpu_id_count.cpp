#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstdio>
#include <cstring>
#include "studio_cpu_id_count_candidate.h"
#include "cpu_id_count_native_init.h"
#if defined(_M_IX86)
static uint32_t cookie=0x12345678;
static void __fastcall checkCookie(uint32_t value,void*){if(value!=cookie)ExitProcess(20);}
#endif
int main(){
#if defined(_M_IX86)
 std::ifstream dll(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tier0.dll)",std::ios::binary);
 unsigned char installed[sizeof(nativeCpuIdBytes)]{};dll.seekg(nativeCpuIdOffset);dll.read(reinterpret_cast<char*>(installed),sizeof(installed));if(!dll||std::memcmp(installed,nativeCpuIdBytes,sizeof(installed)))return 1;
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,installed,sizeof(installed));
 uintptr_t cookieAddress=reinterpret_cast<uintptr_t>(&cookie);std::memcpy(code+7,&cookieAddress,4);
 int32_t delta=static_cast<int32_t>(reinterpret_cast<uintptr_t>(&checkCookie)-reinterpret_cast<uintptr_t>(code+146));std::memcpy(code+142,&delta,4);
 DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 3;FlushInstructionCache(GetCurrentProcess(),code,sizeof(installed));auto native=reinterpret_cast<uint32_t(__thiscall*)(void*)>(code);
 std::ofstream oracle("outputs/native-cpu-id-count.bin",std::ios::binary|std::ios::trunc);if(!oracle)return 4;
#else
 std::ifstream oracle("outputs/native-cpu-id-count.bin",std::ios::binary);if(!oracle)return 4;
#endif
 unsigned cases=0;
 for(unsigned count:{0u,1u,2u,16u,31u,32u})for(unsigned pattern=0;pattern<4;++pattern)
 for(unsigned maskA:{0u,1u,15u,128u,255u})for(unsigned maskB:{0u,3u,240u})for(unsigned shift:{0u,1u,7u,8u,31u,32u,33u,255u}){
  std::array<uint8_t,0x30> object{};object[0x24]=static_cast<uint8_t>(count);object[0x2a]=static_cast<uint8_t>(maskA);object[0x2c]=static_cast<uint8_t>(maskB);object[0x27]=static_cast<uint8_t>(shift);
  for(unsigned i=0;i<count;++i)object[4+i]=static_cast<uint8_t>(pattern==0?0:pattern==1?i:pattern==2?i*7:i%3);
  uint32_t result;
#if defined(_M_IX86)
  result=native(object.data());oracle.write(reinterpret_cast<const char*>(&result),4);if(!oracle)return 5;
#else
  oracle.read(reinterpret_cast<char*>(&result),4);if(!oracle||result!=studioCpuIdCountCandidate(object.data()+4,static_cast<uint8_t>(count),static_cast<uint8_t>(maskA),static_cast<uint8_t>(maskB),static_cast<uint8_t>(shift))){std::printf("cpu_id_mismatch_case=%u\n",cases);return 6;}
#endif
  ++cases;
 }
#if defined(_M_IX86)
 VirtualFree(code,0,MEM_RELEASE);std::printf("native_cpu_id_count_cases=%u\n",cases);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 7;std::printf("x64_native_cpu_id_count_matches=%u\n",cases);
#endif
}

#define NOMINMAX
#include <Windows.h>
#include <fstream>
#include <cstdio>
#include <cstring>
#include "studio_cpu_id_masks_candidate.h"
#include "cpu_id_masks_native_init.h"
int main(){
#if defined(_M_IX86)
 std::ifstream dll(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tier0.dll)",std::ios::binary);unsigned char installed[sizeof(nativeCpuMaskBytes)]{};dll.seekg(nativeCpuMaskOffset);dll.read(reinterpret_cast<char*>(installed),sizeof(installed));if(!dll||std::memcmp(installed,nativeCpuMaskBytes,sizeof(installed)))return 1;
 auto* code=VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!code)return 2;std::memcpy(code,installed,sizeof(installed));DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 3;FlushInstructionCache(GetCurrentProcess(),code,sizeof(installed));auto native=reinterpret_cast<void(__thiscall*)(void*,unsigned,unsigned)>(code);
 std::ofstream oracle("outputs/native-cpu-id-masks.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-cpu-id-masks.bin",std::ios::binary);
#endif
 if(!oracle)return 4;unsigned cases=0;
 for(unsigned total=1;total<256;++total)for(unsigned cores=1;cores<256;++cores){std::array<uint8_t,8> result{};
#if defined(_M_IX86)
  native(result.data(),total,cores);oracle.write(reinterpret_cast<char*>(result.data()),8);if(!oracle)return 5;
#else
  std::array<uint8_t,8> expected{};oracle.read(reinterpret_cast<char*>(result.data()),8);if(!oracle||!studioCpuIdMasksCandidate(static_cast<uint8_t>(total),static_cast<uint8_t>(cores),expected)||result!=expected)return 6;
#endif
  ++cases;
 }
#if defined(_M_IX86)
 VirtualFree(code,0,MEM_RELEASE);std::printf("native_cpu_id_masks_cases=%u\n",cases);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 7;std::printf("x64_native_cpu_id_masks_matches=%u\n",cases);
#endif
}

#define NOMINMAX
#include <Windows.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <cstdio>
#include "multisample_selector_native_init.h"
#include "studio_multisample_selector_candidate.h"
static unsigned candidate(int32_t request){return studioMultisampleSelectorCandidate(request);}
int main(){
#ifdef _M_IX86
 std::ifstream dll(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);unsigned char installed[sizeof(selectorBytes)]{};dll.seekg(selectorOffset);dll.read(reinterpret_cast<char*>(installed),sizeof(installed));if(!dll||std::memcmp(installed,selectorBytes,sizeof(installed)))return 1;
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,installed,sizeof(installed));uintptr_t table=reinterpret_cast<uintptr_t>(code+168);std::memcpy(code+21,&table,4);for(unsigned i=0;i<15;++i){uintptr_t target=reinterpret_cast<uintptr_t>(code+25+9*i);std::memcpy(code+168+4*i,&target,4);}DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 3;FlushInstructionCache(GetCurrentProcess(),code,sizeof(installed));auto native=reinterpret_cast<unsigned(__thiscall*)(void*,int32_t)>(code);std::ofstream oracle("outputs/native-multisample-selector.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-multisample-selector.bin",std::ios::binary);
#endif
 unsigned cases=0;auto check=[&](int32_t request){
#ifdef _M_IX86
 unsigned result=native(nullptr,request);if(result!=candidate(request))return false;oracle.write(reinterpret_cast<const char*>(&result),4);
#else
 unsigned result=0;oracle.read(reinterpret_cast<char*>(&result),4);if(result!=candidate(request))return false;
#endif
 ++cases;return static_cast<bool>(oracle);};
 for(int32_t request=-32768;request<=32767;++request)if(!check(request))return 4;for(int32_t request:{INT32_MIN,INT32_MAX,-65536,65536})if(!check(request))return 5;
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 6;
#endif
 std::printf("multisample_selector_cases=%u matched=passed gpu_executed=0\n",cases);
}

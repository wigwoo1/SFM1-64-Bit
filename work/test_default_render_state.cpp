#define NOMINMAX
#include <Windows.h>
#include <array>
#include <vector>
#include <fstream>
#include <cstring>
#include <cstdio>
#include "default_render_state_native_init.h"
#include "studio_default_render_state_candidate.h"
static std::array<unsigned char,0x600> capabilityA{},capabilityB{};static void* currentCapability;static bool replace;static std::vector<uint32_t> trace;
static void submit(unsigned key,unsigned value){trace.push_back(key);trace.push_back(value);if(trace.size()==86&&replace)currentCapability=capabilityB.data();}
#ifdef _M_IX86
static void __fastcall stateCall(void*,void*,unsigned key,unsigned value){submit(key,value);}
#endif
struct Services{void submit(unsigned key,unsigned value){::submit(key,value);}void* capability(){return currentCapability;}unsigned flags(void* value){return static_cast<unsigned char*>(value)[0x5a9];}unsigned vendor(void* value){uint32_t result;std::memcpy(&result,static_cast<unsigned char*>(value)+0x4dc,4);return result;}};
int main(){
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);std::array<unsigned char,sizeof(nativeDefaultRenderStateBytes)> bytes{};dll.seekg(nativeDefaultRenderStateOffset);dll.read(reinterpret_cast<char*>(bytes.data()),bytes.size());if(!dll||std::memcmp(bytes.data(),nativeDefaultRenderStateBytes,bytes.size()))return 1;auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,bytes.data(),bytes.size());uint32_t one=0x3f800000,other;// Constants below follow the installed-byte sequence audit.
 other=studioDefaultRenderStateEntriesCandidate[55].value;
 for(auto& fix:nativeDefaultRenderStateFixups){uintptr_t target=fix[1]==0x1016d130?reinterpret_cast<uintptr_t>(&currentCapability):fix[1]==0x100fa5f8?reinterpret_cast<uintptr_t>(&one):reinterpret_cast<uintptr_t>(&other);std::memcpy(code+fix[0],&target,4);}for(auto& call:nativeDefaultRenderStateCalls){uint32_t delta=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&stateCall)-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&delta,4);}DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 3;FlushInstructionCache(GetCurrentProcess(),code,4096);auto native=reinterpret_cast<void(__thiscall*)(void*)>(code);std::ofstream oracle("outputs/native-default-render-state.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-default-render-state.bin",std::ios::binary);
#endif
 unsigned cases=0;for(unsigned flags:{0u,0x10u,0x20u,0x30u})for(uint32_t vendor:{0x1002u,0x10deu})for(bool mutation:{false,true}){capabilityA.fill(0);capabilityB.fill(0);capabilityA[0x5a9]=static_cast<unsigned char>(flags);capabilityB[0x5a9]=0x20;std::memcpy(capabilityA.data()+0x4dc,&vendor,4);currentCapability=capabilityA.data();replace=mutation;trace.clear();
#ifdef _M_IX86
 unsigned owner=0;native(&owner);
#else
 Services services;studioDefaultRenderStateCandidate(services);
#endif
 std::array<uint32_t,184> record{static_cast<uint32_t>(trace.size()),currentCapability==capabilityB.data()?1u:0u};for(unsigned i=0;i<trace.size();++i)record[2+i]=trace[i];
#ifdef _M_IX86
 oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
 std::array<uint32_t,184> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(record!=expected)return 4;
#endif
 if(!oracle)return 5;++cases;}
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 6;
#endif
 std::printf("default_render_state_native_cases=%u passed\n",cases);
}


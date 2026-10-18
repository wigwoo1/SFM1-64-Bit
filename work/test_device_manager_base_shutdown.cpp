#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "studio_device_manager_base_shutdown_candidate.h"
#if !defined(_WIN64)
#include "device_manager_base_shutdown_native_init.h"
#endif
static void check(bool b){if(!b)std::abort();}
static std::array<uintptr_t,12> object;
static uintptr_t global54,global58,tree;
static unsigned trace,deleted,mutation;
static void resource(){check(global54==0&&global58==0);trace=trace*10+1;global54=99;}
static void cvar(){check(global54==99&&global58==0);trace=trace*10+2;global58=77;}
static void interfaces(){trace=trace*10+3;if(mutation&1)object[11]=tree;if(mutation&2)object[11]=0;}
static void __fastcall destroy(void* pointer,void*){check(reinterpret_cast<uintptr_t>(pointer)==tree&&object[11]==tree);trace=trace*10+4;++deleted;object[11]=0xabcd;}
struct Ops {
 StudioDeviceManagerBaseShutdownCandidate& state;
 void resourceCleanup(){global54=state.service54;global58=state.service58;resource();state.service54=global54;state.service58=global58;}
 void cvarCleanup(){cvar();state.service54=global54;state.service58=global58;}
 void interfaceCleanup(){object[11]=state.ownedTree;interfaces();state.ownedTree=object[11];}
 void deleteOwnedTree(uintptr_t p){object[11]=state.ownedTree;destroy(reinterpret_cast<void*>(p),nullptr);state.ownedTree=object[11];}
};
int main(){unsigned payload=0;tree=reinterpret_cast<uintptr_t>(&payload);unsigned cases=0;
#if !defined(_WIN64)
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code);std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);dll.seekg(shutdownOffset);dll.read(reinterpret_cast<char*>(code),sizeof(shutdownBytes));check(bool(dll)&&!std::memcmp(code,shutdownBytes,sizeof(shutdownBytes)));
 auto fix=[&](unsigned off,uintptr_t p){unsigned value=static_cast<unsigned>(p);std::memcpy(code+off,&value,4);};auto call=[&](unsigned off,uintptr_t p){fix(off+1,p-reinterpret_cast<uintptr_t>(code+off+5));};
 fix(5,reinterpret_cast<uintptr_t>(&global54));fix(15,reinterpret_cast<uintptr_t>(&global58));call(23,reinterpret_cast<uintptr_t>(&resource));call(28,reinterpret_cast<uintptr_t>(&cvar));call(33,reinterpret_cast<uintptr_t>(&interfaces));call(45,reinterpret_cast<uintptr_t>(&destroy));DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old)&&FlushInstructionCache(GetCurrentProcess(),code,4096));auto shutdown=reinterpret_cast<void(__thiscall*)(void*)>(code);std::ofstream output("outputs/native-base-shutdown.bin",std::ios::binary|std::ios::trunc);
#else
 check(tree>UINT32_MAX);std::ifstream input("outputs/native-base-shutdown.bin",std::ios::binary);
#endif
 for(bool seeded:{false,true})for(unsigned mode:{0u,1u,2u,3u}){mutation=mode;trace=deleted=0;global54=55;global58=66;object.fill(0);object[11]=seeded?tree:0;
#if !defined(_WIN64)
 shutdown(object.data());
#else
 StudioDeviceManagerBaseShutdownCandidate state{global54,global58,object[11]};Ops ops{state};studioDeviceManagerBaseDisconnectCandidate(state,ops);global54=state.service54;global58=state.service58;object[11]=state.ownedTree;
#endif
 bool shouldDelete=!(mode&2)&&(seeded||(mode&1));check(global54==99&&global58==77&&object[11]==0&&trace==(shouldDelete?1234u:123u)&&deleted==unsigned(shouldDelete));std::array<unsigned,5> wire{static_cast<unsigned>(global54),static_cast<unsigned>(global58),static_cast<unsigned>(object[11]),trace,deleted};
#if !defined(_WIN64)
 output.write(reinterpret_cast<char*>(wire.data()),sizeof(wire));check(bool(output));
#else
 std::array<unsigned,5> original{};input.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(bool(input)&&wire==original);
#endif
 ++cases;}
#if !defined(_WIN64)
 check(VirtualFree(code,0,MEM_RELEASE));
#else
 check(input.peek()==std::char_traits<char>::eof());
#endif
 std::printf("device_manager_base_shutdown_%zu_bit_cases=%u passed\n",sizeof(void*)*8,cases);
}

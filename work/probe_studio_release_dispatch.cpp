#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "studio_decal_release_dispatch_candidate.h"
static std::vector<uint32_t> events;static bool hasQueue,enabled;static uint32_t ownerIdentity,argumentIdentity;
static void check(bool v){if(!v)throw std::runtime_error("dispatch parity");}
#ifndef _WIN64
#include "studio_release_dispatch_native_init.h"
static uint32_t service[1],context[1],queue[1];
static void* __fastcall getContext(void* receiver,void*){check(receiver==service);events.push_back(1);return context;}
static void __fastcall enter(void* receiver,void*){check(receiver==context);events.push_back(2);}
static void* __fastcall getQueue(void* receiver,void*){check(receiver==context);events.push_back(3);return hasQueue?queue:nullptr;}
static void __fastcall leave(void* receiver,void*){check(receiver==context);events.push_back(8);}
static void __fastcall releaseContext(void* receiver,void*){check(receiver==context);events.push_back(9);}
static void __fastcall cleanup(void* receiver,void*,uint32_t argument){check(uint32_t(receiver)==ownerIdentity&&argument==argumentIdentity);events.push_back(6);events.push_back(uint32_t(receiver));events.push_back(argument);}
static void __fastcall enqueue(void* receiver,void*,uint32_t owner,uint32_t method,uint32_t* argument){check(receiver==queue&&owner==ownerIdentity&&method==uint32_t(&cleanup)&&*argument==argumentIdentity);events.push_back(7);events.push_back(owner);events.push_back(*argument);}
#endif
int main(int argc,char**argv){try{check(argc==2);
#ifdef _WIN64
std::ifstream file(argv[1],std::ios::binary);
#else
std::ofstream file(argv[1],std::ios::binary);auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,8192,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code);uint32_t serviceTable[104]={};serviceTable[103]=uint32_t(&getContext);service[0]=uint32_t(serviceTable);uint32_t contextTable[144]={};contextTable[2]=uint32_t(&enter);contextTable[143]=uint32_t(&getQueue);contextTable[3]=uint32_t(&leave);contextTable[1]=uint32_t(&releaseContext);context[0]=uint32_t(contextTable);uint32_t config[13]={};initializeStudioReleaseDispatchNative(code,uint32_t(service),uint32_t(config),0,uint32_t(&cleanup),uint32_t(&enqueue));DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old));FlushInstructionCache(GetCurrentProcess(),code,4096);using Dispatch=void(__thiscall*)(void*,uint32_t);auto dispatch=reinterpret_cast<Dispatch>(code);
#endif
uint32_t random=0x12345678;for(unsigned i=0;i<2048;++i)for(unsigned mode=0;mode<4;++mode){events.clear();hasQueue=bool(mode&1);enabled=bool(mode&2);random=random*1664525u+1013904223u;ownerIdentity=random;random=random*1664525u+1013904223u;argumentIdentity=random;
#ifdef _WIN64
studioDecalReleaseDispatchCandidate(argumentIdentity,[]{events.push_back(1);return uintptr_t(1);},[](uintptr_t){events.push_back(2);},[](uintptr_t){events.push_back(3);return hasQueue?uintptr_t(2):uintptr_t(0);},[]{return enabled;},[]{return uintptr_t(ownerIdentity);},[](uintptr_t,uintptr_t owner,const uintptr_t* argument){events.push_back(7);events.push_back(uint32_t(owner));events.push_back(uint32_t(*argument));},[](uintptr_t owner,uintptr_t argument){events.push_back(6);events.push_back(uint32_t(owner));events.push_back(uint32_t(argument));},[](uintptr_t){events.push_back(8);},[](uintptr_t){events.push_back(9);});std::vector<uint32_t> expected(events.size());check(bool(file.read(reinterpret_cast<char*>(expected.data()),expected.size()*4)));check(expected==events);
#else
config[12]=enabled;std::memcpy(code+4104,&ownerIdentity,4);dispatch(nullptr,argumentIdentity);file.write(reinterpret_cast<char*>(events.data()),events.size()*4);
#endif
}
#ifdef _WIN64
check(file.peek()==std::char_traits<char>::eof());
#else
file.flush();check(bool(file));VirtualFree(code,0,MEM_RELEASE);
#endif
std::cout<<"release_dispatch_native_cases=8192\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}

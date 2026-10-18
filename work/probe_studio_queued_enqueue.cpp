#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "studio_decal_queued_call_candidate.h"
static bool fail;static uint32_t stats[8];
static void check(bool v){if(!v)throw std::runtime_error("enqueue parity");}
#ifndef _WIN64
#include "studio_queued_enqueue_native_init.h"
static uint32_t object[9],queue[1];
static void* __cdecl allocate(uint32_t bytes){check(bytes==28);++stats[0];return fail?nullptr:object;}
static void __fastcall enqueue(void* receiver,void*,uint32_t* call){check(receiver==queue);++stats[1];stats[2]=call!=nullptr;if(call){check(call==object&&call[0]==0x100713ec&&call[2]==0x100713e0);stats[3]=call[3];stats[4]=call[4];stats[5]=call[5];stats[6]=call[6];check(call[1]==0xa5a5a5a5&&call[7]==0xa5a5a5a5&&call[8]==0xa5a5a5a5);stats[7]=1;}}
#endif
int main(int argc,char**argv){try{check(argc==2);
#ifdef _WIN64
std::ifstream file(argv[1],std::ios::binary);
#else
std::ofstream file(argv[1],std::ios::binary);auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code);initializeStudioQueuedEnqueueNative(code,uint32_t(&allocate));DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old));FlushInstructionCache(GetCurrentProcess(),code,4096);using Enqueue=void(__thiscall*)(void*,uint32_t,uint32_t,uint32_t*);auto construct=reinterpret_cast<Enqueue>(code);uint32_t table[1]={uint32_t(&enqueue)};queue[0]=uint32_t(table);
#endif
uint32_t random=0x12345678;for(unsigned i=0;i<4096;++i)for(unsigned mode=0;mode<2;++mode){std::memset(stats,0,sizeof(stats));fail=mode!=0;random=random*1664525u+1013904223u;uint32_t owner=random;random=random*1664525u+1013904223u;uint32_t method=random;random=random*1664525u+1013904223u;uint32_t argument=random;
#ifdef _WIN64
StudioDecalQueuedCallCandidate call;uintptr_t local=argument;studioDecalQueueCallCandidate(owner,reinterpret_cast<StudioDecalQueuedCallCandidate::Method>(uintptr_t(method)),fail?nullptr:&local,[&]()->StudioDecalQueuedCallCandidate*{++stats[0];return fail?nullptr:&call;},[&](auto* p){++stats[1];stats[2]=p!=nullptr;if(p){stats[3]=p->references.load();stats[4]=uint32_t(reinterpret_cast<uintptr_t>(p->method));stats[5]=uint32_t(p->owner);stats[6]=uint32_t(p->argument);stats[7]=1;}});local=0;if(!fail)check(call.argument==argument);uint32_t expected[8];check(bool(file.read(reinterpret_cast<char*>(expected),sizeof(expected))));check(std::memcmp(stats,expected,sizeof(stats))==0);
#else
std::memset(object,0xa5,sizeof(object));construct(queue,owner,method,fail?nullptr:&argument);argument=0;file.write(reinterpret_cast<char*>(stats),sizeof(stats));
#endif
}
#ifdef _WIN64
check(file.peek()==std::char_traits<char>::eof());
#else
file.flush();check(bool(file));VirtualFree(code,0,MEM_RELEASE);
#endif
std::cout<<"queued_enqueue_cases=8192\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}

#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "studio_decal_resource_release_candidate.h"
static uint32_t stats[10];
#ifndef _WIN64
#include "studio_decal_release_native_init.h"
static uint32_t owner[22],node[5];
static void __fastcall associations(void* supplied,void*,void* resource){if(supplied!=owner||resource!=node)throw std::runtime_error("association receiver");++stats[0];}
static void __fastcall unexpectedChainService(void*,void*){throw std::runtime_error("unexpectedChainService populated chain service");}
static void __fastcall releaseArray(void* entries,void*,uint32_t kind){if(entries!=reinterpret_cast<void*>(node[1])||kind!=3||owner[21])throw std::runtime_error("array contract");++stats[1];}
static bool __fastcall classify(void* pool,void*,void* resource,void* frontier){if(pool!=owner+9||resource!=node||frontier!=owner+17||owner[20]!=42||owner[21]!=1)throw std::runtime_error("classification contract");++stats[2];return false;}
static uint32_t __cdecl identity(){++stats[3];return 42;}
static void __fastcall contention(void*,void*,uint32_t,int){throw std::runtime_error("unexpectedChainService contention");}
#endif
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("oracle path");
#ifdef _WIN64
std::ifstream file(argv[1],std::ios::binary);
#else
std::ofstream file(argv[1],std::ios::binary);auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,studioDecalReleaseNativeAllocationSize,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)throw std::runtime_error("code allocation");
initializeStudioDecalReleaseNative(code,uint32_t(&associations),uint32_t(&unexpectedChainService),uint32_t(&unexpectedChainService),uint32_t(&unexpectedChainService),uint32_t(&releaseArray),uint32_t(&classify),123,0,uint32_t(&identity),uint32_t(&contention));DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protect");FlushInstructionCache(GetCurrentProcess(),code,4096);using Release=void(__thiscall*)(void*,void*);auto release=reinterpret_cast<Release>(code);
// Exercise the writable data page after protecting the code page.
*reinterpret_cast<uint32_t*>(code+studioDecalReleaseNativeCounterOffset)=456;
#endif
for(unsigned count=0;count<=128;++count){std::memset(stats,0,sizeof(stats));std::vector<unsigned char> entries(count*32,0xa5);for(unsigned i=0;i<count;++i){uint16_t head=0xffff;std::memcpy(entries.data()+i*32,&head,2);}
#ifdef _WIN64
StudioDecalPointerPoolCandidate pool;StudioDecalPointerNodeCandidate logical;logical.descriptor=123;logical.entries=count?reinterpret_cast<uintptr_t>(entries.data()):0;logical.entryCount=count;logical.previous=logical.next=nullptr;pool.head=pool.tail=&logical;pool.linkedCount=1;uint32_t thread=0,depth=0;
studioDecalResourceReleaseCandidate(pool,&logical,[&](auto*){++stats[0];},[](uint16_t){throw std::runtime_error("nonempty chain");},[&](uintptr_t value,int kind){if(value!=reinterpret_cast<uintptr_t>(entries.data())||kind!=3||depth)throw std::runtime_error("x64 array contract");++stats[1];},[&]{++stats[3];thread=42;depth=1;},[&]{thread=depth=0;},[&](auto&,auto*){if(thread!=42||depth!=1)throw std::runtime_error("x64 classify lock");++stats[2];return false;});
stats[4]=!pool.head&&!pool.tail;stats[5]=pool.linkedCount;stats[6]=pool.freeHead==&logical;stats[7]=logical.entries==0&&logical.entryCount==int(count)&&logical.descriptor==123;stats[8]=thread;stats[9]=depth;uint32_t expected[10];if(!file.read(reinterpret_cast<char*>(expected),sizeof(expected))||std::memcmp(expected,stats,sizeof(stats)))throw std::runtime_error("native mismatch");
#else
std::memset(owner,0,sizeof(owner));node[0]=123;node[1]=count?uint32_t(entries.data()):0;node[2]=count;node[3]=node[4]=0;owner[12]=owner[13]=uint32_t(node);owner[15]=1;release(owner,node);stats[4]=!owner[12]&&!owner[13];stats[5]=owner[15];stats[6]=owner[14]==uint32_t(node);stats[7]=node[1]==0&&node[2]==count&&node[0]==123;stats[8]=owner[20];stats[9]=owner[21];file.write(reinterpret_cast<char*>(stats),sizeof(stats));
#endif
}
#ifdef _WIN64
if(file.peek()!=std::char_traits<char>::eof())throw std::runtime_error("extra oracle");
#else
file.flush();if(!file)throw std::runtime_error("oracle write");VirtualFree(code,0,MEM_RELEASE);
#endif
std::cout<<"release_cases=129\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}


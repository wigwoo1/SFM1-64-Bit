#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>
#include "studio_decal_resource_create_candidate.h"
static uint32_t stats[8],descriptor[2];static std::vector<unsigned char> buffer;static bool shrink;
#ifndef _WIN64
#include "studio_decal_create_native_init.h"
static uint32_t owner[22],node[5];
static uint32_t __cdecl identity(){++stats[4];return 42;}
static void __fastcall contended(void*,void*,uint32_t,int){throw std::runtime_error("unexpected contention");}
static void* __fastcall allocateNode(void* pool,void*,uint32_t flag){if(pool!=owner+9||flag||owner[20]!=42||owner[21]!=1)throw std::runtime_error("node allocation contract");++stats[2];node[3]=node[4]=uint32_t(reinterpret_cast<uintptr_t>(node));return node;}
static void __fastcall insertNode(void* pool,void*,void* before,void* supplied){if(pool!=owner+9||before||supplied!=node||owner[21]!=1)throw std::runtime_error("insert contract");++stats[3];node[3]=node[4]=0;}
static void* __cdecl allocateBytes(uint32_t bytes){if(owner[20]||owner[21])throw std::runtime_error("heap under lock");stats[1]=bytes;if(bytes!=buffer.size())throw std::runtime_error("byte count");if(shrink)descriptor[1]/=2;return buffer.data();}
#endif
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("oracle path");unsigned cases=0;
#ifdef _WIN64
std::ifstream f(argv[1],std::ios::binary);
#else
std::ofstream f(argv[1],std::ios::binary);auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,studioDecalCreateNativeAllocationSize,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)throw std::runtime_error("allocate code");initializeStudioDecalCreateNative(code,uint32_t(reinterpret_cast<uintptr_t>(&allocateNode)),uint32_t(reinterpret_cast<uintptr_t>(&insertNode)),uint32_t(reinterpret_cast<uintptr_t>(&allocateBytes)),uint32_t(reinterpret_cast<uintptr_t>(&identity)),uint32_t(reinterpret_cast<uintptr_t>(&contended)));DWORD old;if(!VirtualProtect(code,studioDecalCreateNativeAllocationSize,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protect");FlushInstructionCache(GetCurrentProcess(),code,studioDecalCreateNativeAllocationSize);using Create=void*(__thiscall*)(void*,void*);auto create=reinterpret_cast<Create>(code);
#endif
for(unsigned count=1;count<=128;++count)for(unsigned mode=0;mode<2;++mode){std::memset(stats,0,sizeof(stats));descriptor[0]=0x12345678;descriptor[1]=count;shrink=mode!=0;buffer.assign(4+count*32,0xa5);
#ifdef _WIN64
StudioDecalPointerPoolCandidate pool;StudioDecalPointerNodeCandidate logical;StudioDecalPointerBlockCandidate block{nullptr,1,&logical};uint32_t thread=0,depth=0;
auto* created=studioDecalResourceCreateLiveCandidate(pool,reinterpret_cast<uintptr_t>(descriptor),[]{return int32_t(descriptor[1]);},[&]{++stats[4];thread=42;depth=1;},[&]{thread=depth=0;},[&](auto& state,int additional){if(additional!=1||thread!=42||depth!=1)throw std::runtime_error("x64 node allocation");++stats[2];state.first=&block;},[]{throw std::runtime_error("unexpected allocation warning");},[&](auto&,auto*){if(depth!=1)throw std::runtime_error("insert unlocked");++stats[3];return false;},[&](int n){if(thread||depth)throw std::runtime_error("heap locked");stats[1]=uint32_t(4+n*32);std::memcpy(buffer.data(),&n,4);if(shrink)descriptor[1]/=2;return buffer.data()+4;});stats[0]=created!=nullptr;stats[5]=uint32_t(created->entryCount);stats[6]=thread;stats[7]=depth;
uint32_t expected[8];std::vector<unsigned char> expectedBytes(buffer.size());if(!f.read(reinterpret_cast<char*>(expected),sizeof(expected))||!f.read(reinterpret_cast<char*>(expectedBytes.data()),expectedBytes.size())||std::memcmp(stats,expected,sizeof(stats))||buffer!=expectedBytes)throw std::runtime_error("native constructor mismatch "+std::to_string(cases));
#else
std::memset(owner,0,sizeof(owner));std::memset(node,0xa5,sizeof(node));auto* created=create(owner,descriptor);if(created!=node||node[0]!=uint32_t(reinterpret_cast<uintptr_t>(descriptor))||node[1]!=uint32_t(reinterpret_cast<uintptr_t>(buffer.data()+4)))throw std::runtime_error("native resource identity");stats[0]=created!=nullptr;stats[5]=node[2];stats[6]=owner[20];stats[7]=owner[21];f.write(reinterpret_cast<char*>(stats),sizeof(stats));f.write(reinterpret_cast<char*>(buffer.data()),buffer.size());
#endif
++cases;}
#ifdef _WIN64
if(f.peek()!=std::char_traits<char>::eof())throw std::runtime_error("extra oracle");
#else
f.flush();if(!f)throw std::runtime_error("write");VirtualFree(code,0,MEM_RELEASE);
#endif
std::cout<<"create_cases="<<cases<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}

#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include "manager_append_native_init.h"
#include "studio_manager_append_candidate.h"
struct Manager{uintptr_t* callbacks;int32_t capacity,count;uintptr_t* cachedBase;};static Manager manager{};static std::array<uint32_t,12> owner{};static std::array<uintptr_t,8> first{},second{};static unsigned extra=0,grows=0,moves=0;
static void growData(){++grows;second=first;manager.callbacks=second.data();manager.capacity=8;manager.count+=static_cast<int32_t>(extra);}
#ifdef _M_IX86
static void __fastcall growNative(void* value,void*,int32_t amount){if(value!=owner.data()+6||amount!=1)ExitProcess(20);growData();owner[6]=reinterpret_cast<uintptr_t>(second.data());owner[7]=8;owner[9]+=extra;}
#endif
static void* __cdecl moveNative(void* to,const void* from,unsigned bytes){++moves;return std::memmove(to,from,bytes);}
struct Services{void grow(Manager&,int32_t amount){if(amount!=1)ExitProcess(21);growData();}void move(uintptr_t* to,uintptr_t* from,int32_t count){moveNative(to,from,static_cast<unsigned>(count)*sizeof(uintptr_t));}};
int main(){
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);unsigned char installed[sizeof(appendBytes)]{};dll.seekg(appendOffset);dll.read(reinterpret_cast<char*>(installed),sizeof(installed));if(!dll||std::memcmp(installed,appendBytes,sizeof(installed)))return 1;auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,installed,sizeof(installed));for(unsigned offset:{29u,65u}){uintptr_t target=offset==29?reinterpret_cast<uintptr_t>(&growNative):reinterpret_cast<uintptr_t>(&moveNative);int32_t delta=static_cast<int32_t>(target-reinterpret_cast<uintptr_t>(code+offset+5));std::memcpy(code+offset+1,&delta,4);}DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 3;FlushInstructionCache(GetCurrentProcess(),code,sizeof(installed));auto native=reinterpret_cast<void(__thiscall*)(void*,uintptr_t)>(code);std::ofstream oracle("outputs/native-manager-append.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-manager-append.bin",std::ios::binary);
#endif
 unsigned cases=0;for(int32_t count:{0,1,3})for(bool grow:{false,true})for(unsigned mutation:{0u,2u}){
 for(unsigned i=0;i<8;++i)first[i]=1000+i;second.fill(0);manager={first.data(),grow?count:count+2,count,nullptr};extra=mutation;grows=moves=0;
#ifdef _M_IX86
 owner.fill(0);owner[6]=reinterpret_cast<uintptr_t>(first.data());owner[7]=static_cast<uint32_t>(manager.capacity);owner[9]=static_cast<uint32_t>(count);native(owner.data(),0x1234);manager.callbacks=reinterpret_cast<uintptr_t*>(owner[6]);manager.capacity=static_cast<int32_t>(owner[7]);manager.count=static_cast<int32_t>(owner[9]);manager.cachedBase=reinterpret_cast<uintptr_t*>(owner[10]);
#else
 Services services;studioManagerAppendCandidate(manager,uintptr_t{0x1234},services);
#endif
 std::array<unsigned,13> result{static_cast<unsigned>(manager.count),static_cast<unsigned>(manager.capacity),grows,moves,manager.cachedBase==manager.callbacks?1u:0u};for(unsigned i=0;i<8;++i)result[5+i]=static_cast<unsigned>(manager.callbacks[i]);
#ifdef _M_IX86
 oracle.write(reinterpret_cast<const char*>(result.data()),sizeof(result));
#else
 std::array<unsigned,13> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(result!=expected)return 4;
#endif
 if(!oracle)return 5;++cases;
 }
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 6;
 first.fill(0);manager={first.data(),8,0,nullptr};Services services;studioManagerAppendCandidate(manager,uintptr_t{0x100001234ull},services);if(first[0]!=0x100001234ull)return 7;
#endif
 std::printf("manager_append_native_cases=%u matched=passed\n",cases);}


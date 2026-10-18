#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include "manager_growth_native_init.h"
#include "studio_manager_growth_candidate.h"
struct Manager{uintptr_t* callbacks;int32_t capacity,growBy;};static Manager manager{};static uint32_t owner[3]{};static uintptr_t storage[128]{};static bool fail=false;static unsigned calls=0,kind=0,units=0,observed=0;
static uintptr_t* allocation(size_t bytes,unsigned route){++calls;kind=route;units=static_cast<unsigned>(bytes/sizeof(uintptr_t));
#ifdef _M_IX86
 observed=owner[1];
#else
 observed=static_cast<unsigned>(manager.capacity);
#endif
 return fail?nullptr:storage;}
#ifdef _M_IX86
static void* __fastcall allocateNative(void*,void*,unsigned bytes){return allocation(bytes,1);}
static void* __fastcall reallocateNative(void*,void*,void* previous,unsigned bytes){if(previous!=storage)ExitProcess(20);return allocation(bytes,2);}
#endif
struct Services{uintptr_t* allocate(size_t bytes){return allocation(bytes,1);}uintptr_t* reallocate(uintptr_t* previous,size_t bytes){if(previous!=storage)ExitProcess(21);return allocation(bytes,2);}};
int main(){
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);unsigned char installed[sizeof(growthBytes)]{};dll.seekg(growthOffset);dll.read(reinterpret_cast<char*>(installed),sizeof(installed));if(!dll||std::memcmp(installed,growthBytes,sizeof(installed)))return 1;auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,installed,sizeof(installed));uintptr_t table[4]{};table[1]=reinterpret_cast<uintptr_t>(&allocateNative);table[3]=reinterpret_cast<uintptr_t>(&reallocateNative);uintptr_t object=reinterpret_cast<uintptr_t>(table);void* allocator=&object;void** imported=&allocator;uintptr_t reference=reinterpret_cast<uintptr_t>(&imported);std::memcpy(code+98,&reference,4);DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 3;FlushInstructionCache(GetCurrentProcess(),code,sizeof(installed));auto native=reinterpret_cast<void(__thiscall*)(void*,int32_t)>(code);std::ofstream oracle("outputs/native-manager-growth.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-manager-growth.bin",std::ios::binary);
#endif
 unsigned cases=0;for(int32_t capacity:{0,1,7,8,16})for(int32_t step:{-1,0,1,3})for(int32_t additional:{1,9})for(bool existing:{false,true})for(bool failure:{false,true}){manager={existing?storage:nullptr,capacity,step};fail=failure;calls=kind=units=observed=0;
#ifdef _M_IX86
 owner[0]=reinterpret_cast<uintptr_t>(manager.callbacks);owner[1]=static_cast<uint32_t>(capacity);owner[2]=static_cast<uint32_t>(step);native(owner,additional);manager.callbacks=reinterpret_cast<uintptr_t*>(owner[0]);manager.capacity=static_cast<int32_t>(owner[1]);
#else
 Services services;studioManagerGrowthCandidate(manager,additional,services);
#endif
 std::array<unsigned,6> result{static_cast<unsigned>(manager.capacity),manager.callbacks?1u:0u,calls,kind,units,observed};
#ifdef _M_IX86
 oracle.write(reinterpret_cast<const char*>(result.data()),sizeof(result));
#else
 std::array<unsigned,6> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(result!=expected)return 4;
#endif
 if(!oracle)return 5;++cases;}
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 6;
#endif
 std::printf("manager_growth_cases=%u matched=passed pointer_width_allocation_units=normalized\n",cases);}


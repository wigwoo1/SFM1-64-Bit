#define NOMINMAX
#include <Windows.h>
#include <d3d9.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "queued_query_native_init.h"
#include "studio_queued_query_recreation_candidate.h"
struct Query{unsigned id;};struct Device{void** table;unsigned id;};static void* table[119]{};static Device first{table,1},second{table,2};static Query query{7};static Device* current;static bool replace;static unsigned captures,flushes,creates,receivers;static int fail;static HRESULT failure;
#ifdef _M_IX86
static unsigned char* active;
#endif
static HRESULT __stdcall create(Device* device,unsigned type,Query** output){if(type!=8||flushes!=creates+1)std::abort();receivers+=device->id;*output=&query;const unsigned index=creates++;return static_cast<int>(index)==fail?failure:S_OK;}
#ifdef _M_IX86
static Device** __cdecl getter(){++captures;return &current;}static void __fastcall flush(Device**,void*){++flushes;if(replace)current=&second;}
#endif
struct Owner{int32_t queryCapability;unsigned char resourceQueueActive;std::array<Query*,20> queuedResources;};struct Services{Device** commandState(){++captures;return &current;}void flush(Device**){++flushes;if(replace)current=&second;}Device* device(Device** s){return *s;}HRESULT createQuery(Device* device,unsigned type,Query*& output){return create(device,type,&output);}};
int main(){
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);std::array<unsigned char,sizeof(nativeQueuedQueryBytes)> bytes{};dll.seekg(nativeQueuedQueryOffset);dll.read(reinterpret_cast<char*>(bytes.data()),bytes.size());if(!dll||std::memcmp(bytes.data(),nativeQueuedQueryBytes,bytes.size()))return 1;auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,bytes.data(),bytes.size());for(auto& call:nativeQueuedQueryCalls){uintptr_t target=call[1]==0x1000df50?reinterpret_cast<uintptr_t>(&getter):reinterpret_cast<uintptr_t>(&flush);uint32_t delta=static_cast<uint32_t>(target-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&delta,4);}DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 3;FlushInstructionCache(GetCurrentProcess(),code,4096);auto native=reinterpret_cast<void(__thiscall*)(void*)>(code);std::ofstream oracle("outputs/native-queued-query-recreation.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-queued-query-recreation.bin",std::ios::binary);
#endif
 table[118]=reinterpret_cast<void*>(&create);unsigned cases=0;for(int32_t cap:{-1,0,1,2})for(bool swap:{false,true})for(int failed:{-1,0,9,19})for(HRESULT hr:{D3DERR_NOTAVAILABLE,E_FAIL}){replace=swap;fail=failed;failure=hr;current=&first;captures=flushes=creates=receivers=0;Owner owner{cap,7,{}};owner.queuedResources.fill(&query);
#ifdef _M_IX86
 std::array<uint32_t,180> original{};original[0x6c/4]=static_cast<uint32_t>(cap);active=reinterpret_cast<unsigned char*>(original.data())+0xc8;*active=7;auto* slots=reinterpret_cast<Query**>(original.data()+0xcc/4);for(unsigned i=0;i<20;++i)slots[i]=&query;native(original.data());owner.resourceQueueActive=*active;for(unsigned i=0;i<20;++i)owner.queuedResources[i]=slots[i];
#else
 Services services;studioQueuedQueryRecreationCandidate(owner,services);
#endif
 std::array<uint32_t,25> record{owner.resourceQueueActive,captures,flushes,creates,receivers};for(unsigned i=0;i<20;++i)record[5+i]=owner.queuedResources[i]?owner.queuedResources[i]->id:0;
#ifdef _M_IX86
 oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
 std::array<uint32_t,25> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(record!=expected){std::printf("mismatch=%u\n",cases);return 4;}
#endif
 if(!oracle)return 5;++cases;}
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 6;
#endif
 std::printf("queued_query_native_cases=%u passed\n",cases);
}



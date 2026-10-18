#define NOMINMAX
#include <Windows.h>
#include <d3d9.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "pair_query_native_init.h"
#include "studio_pair_query_recreation_candidate.h"
static void* queryTable[7]{};struct Query{void** table;unsigned id;};struct Device{void** table;unsigned id;};static void* table[119]{};static Device first{table,1},second{table,2};static Query query{queryTable,7};static Device* current;static bool replace;static unsigned captures,flushes,creates,receivers,issues,warnings,textures;static int32_t publishedCapability;static int fail;static HRESULT failure;
#ifdef _M_IX86
static unsigned char* active;
#endif
static HRESULT __stdcall create(Device* device,unsigned type,Query** output){if(type!=8||flushes!=creates+1)std::abort();receivers+=device->id;*output=&query;const unsigned index=creates++;return static_cast<int>(index)==fail?failure:S_OK;}
static HRESULT __stdcall issue(Query* q,unsigned flags){if(q!=&query||flags!=1)std::abort();++issues;return S_OK;}
static void __cdecl warning(const char*){++warnings;}
#ifdef _M_IX86
static void __fastcall texture(void* owner,void*){++textures;*reinterpret_cast<int32_t*>(static_cast<unsigned char*>(owner)+0x6c)=publishedCapability;}
static Device** __cdecl getter(){++captures;return &current;}static void __fastcall flush(Device**,void*){++flushes;if(replace)current=&second;}
#endif
struct Owner{int32_t queryCapability;std::array<Query*,2> queries;std::array<unsigned char,2> active;};struct Services{bool diagnosticEnabled(){return false;}void diagnostic(){}void queryUnavailable(){warning("");}void recreateTexture(Owner& owner){++textures;owner.queryCapability=publishedCapability;}void issue(Query* q,unsigned flags){::issue(q,flags);}Device** commandState(){++captures;return &current;}void flush(Device**){++flushes;if(replace)current=&second;}Device* device(Device** s){return *s;}HRESULT createQuery(Device* device,unsigned type,Query*& output){return create(device,type,&output);}};
int main(){
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);std::array<unsigned char,sizeof(nativePairQueryBytes)> bytes{};dll.seekg(nativePairQueryOffset);dll.read(reinterpret_cast<char*>(bytes.data()),bytes.size());if(!dll||std::memcmp(bytes.data(),nativePairQueryBytes,bytes.size()))return 1;auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,bytes.data(),bytes.size());std::array<uint32_t,16> config{};void* configPointer=config.data();void* warningPointer=reinterpret_cast<void*>(&warning);const char* label="controlled";for(auto& fix:nativePairQueryFixups){uintptr_t value=fix[1]==0x10183c64?reinterpret_cast<uintptr_t>(&configPointer):fix[1]==0x100fa244?reinterpret_cast<uintptr_t>(&warningPointer):reinterpret_cast<uintptr_t>(label);std::memcpy(code+fix[0],&value,4);}for(auto& call:nativePairQueryCalls){uintptr_t target=call[1]==0x1002b1e0?reinterpret_cast<uintptr_t>(&texture):call[1]==0x1000df50?reinterpret_cast<uintptr_t>(&getter):reinterpret_cast<uintptr_t>(&flush);uint32_t delta=static_cast<uint32_t>(target-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&delta,4);}DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 3;FlushInstructionCache(GetCurrentProcess(),code,4096);auto native=reinterpret_cast<void(__thiscall*)(void*)>(code);std::ofstream oracle("outputs/native-queued-query-recreation.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-queued-query-recreation.bin",std::ios::binary);
#endif
 queryTable[6]=reinterpret_cast<void*>(&issue);table[118]=reinterpret_cast<void*>(&create);unsigned cases=0;for(int32_t cap:{-1,0,1,2})for(bool swap:{false,true})for(int failed:{-1,0,1})for(HRESULT hr:{D3DERR_NOTAVAILABLE,E_FAIL}){replace=swap;fail=failed;failure=hr;current=&first;captures=flushes=creates=receivers=issues=warnings=textures=0;publishedCapability=cap;Owner owner{99,{&query,&query},{7,8}};
#ifdef _M_IX86
 std::array<uint32_t,180> original{};original[0x6c/4]=99;active=reinterpret_cast<unsigned char*>(original.data())+0xbc;active[0]=7;active[1]=8;auto* slots=reinterpret_cast<Query**>(original.data()+0xb4/4);for(unsigned i=0;i<2;++i)slots[i]=&query;native(original.data());owner.active[0]=active[0];owner.active[1]=active[1];for(unsigned i=0;i<2;++i)owner.queries[i]=slots[i];
#else
 Services services;studioPairQueryRecreationCandidate(owner,services);
#endif
 std::array<uint32_t,11> record{owner.active[0],owner.active[1],captures,flushes,creates,receivers,issues,warnings,textures};for(unsigned i=0;i<2;++i)record[9+i]=owner.queries[i]?owner.queries[i]->id:0;
#ifdef _M_IX86
 oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
 std::array<uint32_t,11> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(record!=expected){std::printf("mismatch=%u\n",cases);return 4;}
#endif
 if(!oracle)return 5;++cases;}
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 6;
#endif
 std::printf("pair_query_native_cases=%u passed\n",cases);
}



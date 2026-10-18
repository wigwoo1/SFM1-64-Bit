#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstdio>
#include <cstring>
#include "device_event_query_native_init.h"
#include "studio_device_event_query_candidate.h"
#include "native_device_event_query_candidate.h"
static int32_t* cached=nullptr;static uint32_t hresult=0,calls=0,releases=0,observed=0,typeSeen=0;
static bool writes=false,mutateCreate=false,mutateRelease=false;static void* queryObject=nullptr;
static HRESULT __stdcall createQuery(void*,unsigned type,void** output){if(type!=8||*output)ExitProcess(20);++calls;typeSeen=type;if(mutateCreate)*cached=-2;if(writes)*output=queryObject;return static_cast<HRESULT>(hresult);}
static ULONG __stdcall releaseQuery(void* object){if(object!=queryObject)ExitProcess(21);++releases;observed=static_cast<uint32_t>(*cached);if(mutateRelease)*cached=7;return 0;}
struct Backend{void* initialOutput(){return nullptr;}HRESULT create(unsigned type,void*& output){return createQuery(nullptr,type,&output);}void release(void* object){releaseQuery(object);}};
struct Query{ULONG Release(){return releaseQuery(this);}};
struct Device{HRESULT CreateQuery(D3DQUERYTYPE type,Query** output){if(*output)ExitProcess(22);void* value=nullptr;auto result=createQuery(this,static_cast<unsigned>(type),&value);*output=static_cast<Query*>(value);return result;}};
int main(){
#if defined(_M_IX86)
 std::ifstream dll(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);unsigned char installed[sizeof(nativeEventQueryBytes)]{};dll.seekg(nativeEventQueryOffset);dll.read(reinterpret_cast<char*>(installed),sizeof(installed));if(!dll||std::memcmp(installed,nativeEventQueryBytes,sizeof(installed)))return 1;
 auto* code=VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!code)return 2;std::memcpy(code,installed,sizeof(installed));DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 3;FlushInstructionCache(GetCurrentProcess(),code,sizeof(installed));auto native=reinterpret_cast<void(__thiscall*)(void*,void*)>(code);
 std::array<uintptr_t,119> deviceVtable{};deviceVtable[118]=reinterpret_cast<uintptr_t>(&createQuery);uintptr_t deviceObject=reinterpret_cast<uintptr_t>(deviceVtable.data());
 std::array<uintptr_t,3> queryVtable{};queryVtable[2]=reinterpret_cast<uintptr_t>(&releaseQuery);uintptr_t queryOwner=reinterpret_cast<uintptr_t>(queryVtable.data());queryObject=&queryOwner;
 std::ofstream oracle("outputs/native-device-event-query-grid.bin",std::ios::binary|std::ios::trunc);
#else
 Query query;Device device;queryObject=&query;if(reinterpret_cast<uintptr_t>(queryObject)<=0xffffffffull)return 8;std::ifstream oracle("outputs/native-device-event-query-grid.bin",std::ios::binary);
#endif
 if(!oracle)return 4;unsigned cases=0;
 for(int32_t initial:{-2,-1,0,1,2})for(uint32_t result:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu})for(bool write:{false,true})for(bool changeCreate:{false,true})for(bool changeRelease:{false,true}){
  std::array<int32_t,28> owner{};owner[27]=initial;cached=&owner[27];hresult=result;writes=write;mutateCreate=changeCreate;mutateRelease=changeRelease;calls=releases=typeSeen=0;observed=0xdeadbeefu;
#if defined(_M_IX86)
  native(owner.data(),&deviceObject);
#else
  NativeDeviceEventQueryBackendCandidateT<Device,Query> backend{device};studioDeviceEventQueryCandidate(*cached,backend);
#endif
  std::array<uint32_t,5> record{static_cast<uint32_t>(*cached),calls,releases,observed,typeSeen};
#if defined(_M_IX86)
  oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));if(!oracle)return 5;
#else
  std::array<uint32_t,5> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!oracle||record!=expected){std::printf("event_query_mismatch=%u\n",cases);return 6;}
#endif
  ++cases;
 }
#if defined(_M_IX86)
 VirtualFree(code,0,MEM_RELEASE);std::printf("native_event_query_controlled_cases=%u gpu_executed=0\n",cases);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 7;std::printf("x64_native_event_query_matches=%u fullwidth_query_transport=passed gpu_executed=0\n",cases);
#endif
}

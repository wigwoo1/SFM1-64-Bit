#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include "query_cleanup_native_init.h"
#include "native_query_cleanup_candidate.h"
struct Object{uintptr_t table;unsigned id;ULONG Release();HRESULT GetData(void* output,DWORD bytes,DWORD flags);};static std::array<unsigned,3> releases{},polls{};static unsigned pollMode=0;static unsigned times=0,logs=0;static bool diagnosticFlag=false;
static ULONG __stdcall releaseObject(Object* value){++releases[value->id];return 0;}
static HRESULT __stdcall getData(Object* value,void* output,unsigned bytes,unsigned flags){if(bytes!=4||flags!=1||*static_cast<unsigned*>(output)!=0)ExitProcess(20);const unsigned count=++polls[value->id];if(pollMode==3)return E_FAIL;if(pollMode==2||pollMode==4||(pollMode==1&&count<=2))return S_FALSE;return S_OK;}
ULONG Object::Release(){return releaseObject(this);}HRESULT Object::GetData(void* output,DWORD bytes,DWORD flags){return ::getData(this,output,bytes,flags);}
static double __cdecl timer(){const unsigned tick=times++;return static_cast<double>(tick)*(pollMode==2?1.0:pollMode==4?2.0:pollMode==3?3.0:0.0);}
static void __cdecl logMessage(const char*){++logs;}
struct Owner{Object* resource;std::array<Object*,2> queries;std::array<unsigned char,2> active;};
struct Lifecycle{bool diagnosticEnabled(){return diagnosticFlag;}void diagnostic(){logMessage(nullptr);}double time(){return timer();}double limit(){return 2.0;}};
void compileActualQueryCleanupBinding(NativeQueryCleanupOwnerCandidate owner,Lifecycle& lifecycle){NativeQueryCleanupServicesCandidateT<Lifecycle> services{lifecycle};studioQueryCleanupCandidate(owner,services);}
int main(){uintptr_t table[8]{};table[2]=reinterpret_cast<uintptr_t>(&releaseObject);table[7]=reinterpret_cast<uintptr_t>(&getData);Object resource{reinterpret_cast<uintptr_t>(table),0},one{reinterpret_cast<uintptr_t>(table),1},two{reinterpret_cast<uintptr_t>(table),2};
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);unsigned char installed[sizeof(nativeQueryCleanupBytes)]{};dll.seekg(nativeQueryCleanupOffset);dll.read(reinterpret_cast<char*>(installed),sizeof(installed));if(!dll||std::memcmp(installed,nativeQueryCleanupBytes,sizeof(installed)))return 1;auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,installed,sizeof(installed));unsigned config[13]{};void* configObject=config;uintptr_t logImport=reinterpret_cast<uintptr_t>(&logMessage),timerImport=reinterpret_cast<uintptr_t>(&timer);double limit=2;const char message[]="controlled";
 for(auto& fix:nativeQueryCleanupFixups){const void* target=nullptr;switch(fix[1]){case 0x10183c64:target=&configObject;break;case 0x100fe764:target=message;break;case 0x100fa244:target=&logImport;break;case 0x100fa210:target=&timerImport;break;case 0x100fc8c8:target=&limit;break;default:return 3;}uintptr_t address=reinterpret_cast<uintptr_t>(target);std::memcpy(code+fix[0],&address,4);}DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 4;FlushInstructionCache(GetCurrentProcess(),code,sizeof(installed));auto native=reinterpret_cast<void(__thiscall*)(void*)>(code);std::ofstream oracle("outputs/native-query-cleanup.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-query-cleanup.bin",std::ios::binary);
#endif
 unsigned cases=0;for(unsigned mode:{0u,1u,2u,3u,4u})for(bool present:{false,true})for(unsigned mask:{0u,1u,2u,3u})for(unsigned active:{0u,1u,2u,3u})for(bool log:{false,true}){
 Owner owner{present?&resource:nullptr,{mask&1?&one:nullptr,mask&2?&two:nullptr},{static_cast<unsigned char>(active&1),static_cast<unsigned char>((active>>1)&1)}};releases.fill(0);polls.fill(0);times=logs=0;diagnosticFlag=log;pollMode=mode;
#ifdef _M_IX86
 std::array<uint32_t,50> raw{};raw[49]=reinterpret_cast<uintptr_t>(owner.resource);raw[45]=reinterpret_cast<uintptr_t>(owner.queries[0]);raw[46]=reinterpret_cast<uintptr_t>(owner.queries[1]);auto* bytes=reinterpret_cast<unsigned char*>(raw.data());bytes[188]=owner.active[0];bytes[189]=owner.active[1];config[12]=log?1u:0u;native(raw.data());owner.resource=reinterpret_cast<Object*>(raw[49]);owner.queries[0]=reinterpret_cast<Object*>(raw[45]);owner.queries[1]=reinterpret_cast<Object*>(raw[46]);owner.active={bytes[188],bytes[189]};
#else
 Lifecycle lifecycle;NativeQueryCleanupServicesCandidateT<Lifecycle> services{lifecycle};NativeQueryCleanupOwnerCandidateT<Object,Object> view{owner.resource,owner.queries,owner.active};studioQueryCleanupCandidate(view,services);
#endif
 const unsigned expectedPolls=mode==1||mode==2?3u:mode==4?2u:1u;for(unsigned index=0;index<2;++index){const unsigned expected=(mask&(1u<<index))&&(active&(1u<<index))?expectedPolls:0u;if(polls[index+1]!=expected)return 8;}
 std::array<unsigned,12> record{releases[0],releases[1],releases[2],polls[1],polls[2],times,logs,owner.resource?1u:0u,owner.queries[0]?1u:0u,owner.queries[1]?1u:0u,owner.active[0],owner.active[1]};
#ifdef _M_IX86
 oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
 std::array<unsigned,12> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(record!=expected)return 5;
#endif
 if(!oracle)return 6;++cases;}
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 7;
#endif
 std::printf("query_cleanup_native_cases=%u matched=passed gpu_executed=0\n",cases);}




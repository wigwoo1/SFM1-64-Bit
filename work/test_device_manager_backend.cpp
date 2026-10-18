#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include "studio_device_manager_backend_candidate.h"
#if !defined(_WIN64)
#include "device_manager_backend_native_init.h"
#endif
static void check(bool b){if(!b)std::abort();}
static bool baseSuccess,createSuccess,releaseMutation;static unsigned trace,releaseCount;
static void event(unsigned n){trace=trace*10+n;}
static std::array<uintptr_t,13> receiver;
static uintptr_t fakeObject,otherObject;
static bool __fastcall baseConnect(void*,void*,uintptr_t factory){check(factory==0x1234);event(1);return baseSuccess;}
static uintptr_t __stdcall createD3D(unsigned version){check(version==32);event(2);return createSuccess?fakeObject:0;}
static void __cdecl warning(const char* text,...){check(!std::strcmp(text,"Failed to create D3D9!\n"));event(3);}
static unsigned __stdcall release(void* object){check(reinterpret_cast<uintptr_t>(object)==fakeObject);check(receiver[12]==fakeObject);event(4);++releaseCount;if(releaseMutation)receiver[12]=otherObject;return 0;}
static void __fastcall baseDisconnect(void*,void*){check(receiver[12]==0);event(5);}
struct Ops {
 StudioDeviceManagerBackendCandidate* state;
 bool baseConnect(uintptr_t f){return ::baseConnect(nullptr,nullptr,f);}
 uintptr_t createDirect3D(unsigned v){return createD3D(v);}
 void warning(const char* t){::warning(t);}
 void release(uintptr_t p){receiver[12]=state->direct3D;::release(reinterpret_cast<void*>(p));state->direct3D=receiver[12];}
 void baseDisconnect(){receiver[12]=state->direct3D;::baseDisconnect(nullptr,nullptr);}
};
int main(){
 std::array<uintptr_t,3> table{0,0,reinterpret_cast<uintptr_t>(&release)};uintptr_t object=reinterpret_cast<uintptr_t>(table.data()),other=0;fakeObject=reinterpret_cast<uintptr_t>(&object);otherObject=reinterpret_cast<uintptr_t>(&other);
#if !defined(_WIN64)
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);
 auto verify=[&](unsigned offset,const unsigned char* expected,unsigned size){std::array<unsigned char,61> raw{};dll.seekg(offset);dll.read(reinterpret_cast<char*>(raw.data()),size);check(bool(dll)&&!std::memcmp(raw.data(),expected,size));};
 verify(manager_connect_offset,manager_connect,sizeof(manager_connect));verify(manager_disconnect_offset,manager_disconnect,sizeof(manager_disconnect));verify(manager_allocation_thunk_offset,manager_allocation_thunk,sizeof(manager_allocation_thunk));
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,1024,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code);std::memcpy(code,manager_connect,sizeof(manager_connect));std::memcpy(code+128,manager_disconnect,sizeof(manager_disconnect));std::memcpy(code+256,manager_allocation_thunk,sizeof(manager_allocation_thunk));auto creator=&createD3D;auto warn=&warning;
 auto fix=[&](unsigned offset,uintptr_t value){unsigned v=static_cast<unsigned>(value);std::memcpy(code+offset,&v,4);};
 auto call=[&](unsigned offset,uintptr_t target){fix(offset+1,target-reinterpret_cast<uintptr_t>(code+offset+5));};
 call(10,reinterpret_cast<uintptr_t>(&baseConnect));call(21,reinterpret_cast<uintptr_t>(code+256));fix(34,reinterpret_cast<uintptr_t>("Failed to create D3D9!\n"));fix(40,reinterpret_cast<uintptr_t>(&warn));fix(258,reinterpret_cast<uintptr_t>(&creator));call(128+28,reinterpret_cast<uintptr_t>(&baseDisconnect));DWORD old;check(VirtualProtect(code,1024,PAGE_EXECUTE_READ,&old)&&FlushInstructionCache(GetCurrentProcess(),code,1024));
 using Connect=bool(__thiscall*)(void*,uintptr_t);using Disconnect=void(__thiscall*)(void*);auto connect=reinterpret_cast<Connect>(code);auto disconnect=reinterpret_cast<Disconnect>(code+128);std::ofstream records("outputs/native-device-manager-backend.bin",std::ios::binary|std::ios::trunc);
#else
 check(fakeObject>UINT32_MAX&&otherObject>UINT32_MAX);std::ifstream records("outputs/native-device-manager-backend.bin",std::ios::binary);
#endif
 unsigned cases=0;
 for(bool base:{false,true})for(bool created:{false,true})for(bool seeded:{false,true})for(bool mutate:{false,true}){
 baseSuccess=base;createSuccess=created;releaseMutation=mutate;trace=releaseCount=0;receiver.fill(0);receiver[12]=seeded?fakeObject:0;
#if !defined(_WIN64)
 bool result=connect(receiver.data(),0x1234);uintptr_t retained=receiver[12];disconnect(receiver.data());
#else
 StudioDeviceManagerBackendCandidate state{receiver[12]};Ops ops{&state};bool result=studioDeviceManagerConnectCandidate(state,0x1234,ops);uintptr_t retained=state.direct3D;studioDeviceManagerDisconnectCandidate(state,ops);receiver[12]=state.direct3D;
#endif
 check(result==(base&&created));check(retained==(base?(created?fakeObject:0):(seeded?fakeObject:0)));check(receiver[12]==0);unsigned expected=base?(created?12:123):1;if(retained)expected=expected*10+4;expected=expected*10+5;check(trace==expected&&releaseCount==unsigned(retained!=0));std::array<unsigned,5> wire{unsigned(result),unsigned(retained!=0),unsigned(receiver[12]!=0),trace,releaseCount};
#if !defined(_WIN64)
 records.write(reinterpret_cast<const char*>(wire.data()),sizeof(wire));check(bool(records));
#else
 std::array<unsigned,5> original{};records.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(bool(records)&&wire==original);
#endif
 ++cases;
 }
#if !defined(_WIN64)
 check(VirtualFree(code,0,MEM_RELEASE));
#else
 check(records.peek()==std::char_traits<char>::eof());
#endif
 std::printf("device_manager_backend_%zu_bit_cases=%u passed\n",sizeof(void*)*8,cases);
}


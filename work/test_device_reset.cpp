#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include "device_reset_native_init.h"
#include "studio_device_reset_candidate.h"
struct State{void* current;};static State state{};
static unsigned char* owner=nullptr;static void* firstDevice=nullptr;static void* secondDevice=nullptr;
static bool mutate=false,replace=false;static uint32_t resultBits=0,secondWindow=0,secondAdapter=0,receiver=0,parameter=0,resetCalls=0,logs=0,posts=0;static uint64_t trace=0;
static void step(unsigned value){trace=trace*10+value;}
static uint32_t read(unsigned offset){uint32_t value;std::memcpy(&value,owner+offset,4);return value;}
static void write(unsigned offset,uint32_t value){std::memcpy(owner+offset,&value,4);}
static void __fastcall rebuild(void* object,void*,uint32_t window,unsigned adapter,void*,bool initial){if(object!=owner)ExitProcess(20);step(initial?1:4);if(!initial){secondWindow=window;secondAdapter=adapter;}}
static void __fastcall manager(void*,void*){step(2);}
static void __fastcall hook(void* object,void*,bool enabled){if(object!=owner||!enabled)ExitProcess(21);step(3);if(mutate){write(8,0x5678);write(0x24,9);write(0x30,0xabcdef);}}
static State* __cdecl getter(){step(5);return &state;}
static void __fastcall flush(void* object,void*){if(object!=&state)ExitProcess(22);step(6);if(replace)state.current=secondDevice;}
static HRESULT __stdcall reset(void* object,void* parameters){if(parameters!=owner+0x30)ExitProcess(23);if(object!=firstDevice&&object!=secondDevice)ExitProcess(24);step(7);++resetCalls;receiver=object==firstDevice?1u:2u;parameter=read(0x30);return static_cast<HRESULT>(resultBits);}
static void __cdecl logFailure(const char*,uint32_t value){if(value!=resultBits)ExitProcess(25);step(8);++logs;}
static void __fastcall post(void* object,void*,bool a,bool b,const char*){if(object!=owner||!a||!b)ExitProcess(26);step(9);++posts;}
struct Owner{uint8_t& flags;uint32_t& window;uint32_t& adapter;uint32_t& parameters;};
struct Services{
 unsigned descriptorFlags(std::array<uint32_t,14>& descriptor){return descriptor[13];}
 void rebuild(uint32_t window,unsigned adapter,std::array<uint32_t,14>& descriptor,bool initial){::rebuild(owner,nullptr,window,adapter,descriptor.data(),initial);}
 void managerCleanup(){manager(nullptr,nullptr);}void ownerHook(bool enabled){hook(owner,nullptr,enabled);}
 State& commandState(){return *getter();}void flush(State& value){::flush(&value,nullptr);}void* device(State& value){return value.current;}
 HRESULT reset(void* device,uint32_t& parameters){return ::reset(device,&parameters);}void resetFailure(HRESULT value){logFailure(nullptr,static_cast<uint32_t>(value));}void postReset(bool a,bool b){post(owner,nullptr,a,b,nullptr);}
};
int main(){std::array<uintptr_t,17> deviceVtable{};deviceVtable[16]=reinterpret_cast<uintptr_t>(&reset);uintptr_t device1=reinterpret_cast<uintptr_t>(deviceVtable.data()),device2=device1;firstDevice=&device1;secondDevice=&device2;
 std::array<uintptr_t,1> ownerVtable{reinterpret_cast<uintptr_t>(&hook)};
#if defined(_M_IX86)
 std::ifstream dll(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);unsigned char installed[sizeof(nativeDeviceResetBytes)]{};dll.seekg(nativeDeviceResetOffset);dll.read(reinterpret_cast<char*>(installed),sizeof(installed));if(!dll||std::memcmp(installed,nativeDeviceResetBytes,sizeof(installed)))return 1;
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,installed,sizeof(installed));void* managerObject=nullptr;uintptr_t logImport=reinterpret_cast<uintptr_t>(&logFailure);const char message[]="controlled";
 for(auto& fix:nativeDeviceResetFixups){const void* target=nullptr;switch(fix[1]){case 0x10183b54:target=&managerObject;break;case 0x100fe948:case 0x100fe938:target=message;break;case 0x100fa244:target=&logImport;break;default:return 3;}uintptr_t address=reinterpret_cast<uintptr_t>(target);std::memcpy(code+fix[0],&address,4);}
 for(auto& call:nativeDeviceResetCalls){uintptr_t target=0;switch(call[1]){case 0x10029960:target=reinterpret_cast<uintptr_t>(&rebuild);break;case 0x10028a80:target=reinterpret_cast<uintptr_t>(&manager);break;case 0x1000df50:target=reinterpret_cast<uintptr_t>(&getter);break;case 0x10002e50:target=reinterpret_cast<uintptr_t>(&flush);break;case 0x1002b4c0:target=reinterpret_cast<uintptr_t>(&post);break;default:return 4;}int32_t delta=static_cast<int32_t>(target-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&delta,4);}
 DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 5;FlushInstructionCache(GetCurrentProcess(),code,sizeof(installed));auto native=reinterpret_cast<unsigned char(__thiscall*)(void*,void*)>(code);std::ofstream oracle("outputs/native-device-reset-grid.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-device-reset-grid.bin",std::ios::binary);
#endif
 if(!oracle)return 6;unsigned cases=0;
 for(unsigned gate:{0u,2u})for(bool change:{false,true})for(bool replacement:{false,true})for(uint32_t hresult:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu}){
  std::array<uint32_t,44> storage{};owner=reinterpret_cast<unsigned char*>(storage.data());uintptr_t table=reinterpret_cast<uintptr_t>(ownerVtable.data());std::memcpy(owner,&table,sizeof(table));write(8,0x1234);write(0x24,3);write(0x30,0x123);write(0x6c,100);owner[0xac]=0xff;
  std::array<uint32_t,14> descriptor{};descriptor[13]=gate;state.current=firstDevice;mutate=change;replace=replacement;resultBits=hresult;trace=0;secondWindow=secondAdapter=receiver=parameter=resetCalls=logs=posts=0;bool success;
#if defined(_M_IX86)
  success=native(owner,descriptor.data())!=0;
#else
  Owner view{owner[0xac],storage[2],storage[9],storage[12]};Services services;success=studioDeviceResetCandidate(view,descriptor,services);
#endif
  std::array<uint32_t,12> record{static_cast<uint32_t>(trace),static_cast<uint32_t>(trace>>32),resetCalls,logs,posts,receiver,secondWindow,secondAdapter,owner[0xac],success?1u:0u,parameter,read(0x6c)};
#if defined(_M_IX86)
  oracle.write(reinterpret_cast<char*>(record.data()),sizeof(record));if(!oracle)return 7;
#else
  std::array<uint32_t,12> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!oracle||record!=expected){std::printf("reset_mismatch=%u\n",cases);return 8;}
#endif
  ++cases;
 }
#if defined(_M_IX86)
 VirtualFree(code,0,MEM_RELEASE);std::printf("native_device_reset_controlled_cases=%u gpu_executed=0\n",cases);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 9;std::printf("x64_native_device_reset_matches=%u live_owner_and_post_flush_device_reload=passed gpu_executed=0\n",cases);
#endif
}

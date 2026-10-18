#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstdio>
#include <cstring>
#include "device_creation_native_init.h"
static std::array<unsigned char,0x34> convar{};
static std::array<unsigned char,16> cpu{};
static unsigned calls=0,waits=0,cpuCalls=0,flagsSeen=0,successAttempt=0;
static bool scripted=false,firstWrites=false,secondWrites=false;static uint32_t firstResult=0,secondResult=0;
static bool cpuMutation=false;static unsigned creationMutation=0;
static unsigned char* serviceStorage=nullptr;static uintptr_t replacementApi=0;
static void* primaryApiObject=nullptr;static void* secondaryApiObject=nullptr;
static bool replaceInterface=false;static unsigned apiRoute=0;
static void __fastcall reference(void* object,void*,const char* name){if(std::strcmp(name,"mat_queue_mode"))ExitProcess(20);void* value=convar.data();std::memcpy(static_cast<unsigned char*>(object)+4,&value,4);}
static const void* __cdecl cpuInfo(){++cpuCalls;if(cpuMutation){int mode=-1;std::memcpy(convar.data()+0x30,&mode,4);cpu[6]=cpuCalls==1?1:2;}return cpu.data();}
static void __stdcall waitCallback(DWORD ms){if(ms!=1000||calls!=1)ExitProcess(21);++waits;}
static bool __cdecl logEnabled(unsigned,unsigned){return false;}
static void forbidden(){ExitProcess(22);}
static HRESULT __stdcall create(void* object,unsigned adapter,unsigned type,void* window,unsigned flags,void* presentation,void** output){
 if(adapter!=3||type!=1||window!=reinterpret_cast<void*>(0x1234))ExitProcess(23);
 if(object!=primaryApiObject&&object!=secondaryApiObject)ExitProcess(24);
 apiRoute=apiRoute*10+(object==primaryApiObject?1u:2u);
 ++calls;flagsSeen=flags;
 if(replaceInterface&&calls==1)std::memcpy(serviceStorage+0x30,&replacementApi,4);
 if((creationMutation==1&&calls==1)||(creationMutation==2&&calls==2)){unsigned windowed=creationMutation==2?1:0;std::memcpy(static_cast<unsigned char*>(presentation)+0x20,&windowed,4);}
 if(scripted){if(calls==1?firstWrites:secondWrites)*output=reinterpret_cast<void*>(0x87654321);return static_cast<HRESULT>(calls==1?firstResult:secondResult);}
 if(calls==successAttempt){*output=reinterpret_cast<void*>(0x87654321);return S_OK;}return E_FAIL;
}
int main(){static_assert(sizeof(void*)==4,"x86 required");
 std::ifstream dll("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\shaderapidx9.dll",std::ios::binary);
 std::array<unsigned char,332> original{};dll.seekg(nativeDeviceCreationFileOffset);dll.read(reinterpret_cast<char*>(original.data()),332);
 if(!dll||std::memcmp(original.data(),nativeDeviceCreationBytes,332))return 1;
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!code)return 2;
 std::memcpy(code,original.data(),332);
 std::array<uintptr_t,17> vtable{};vtable[16]=reinterpret_cast<uintptr_t>(&create);uintptr_t api=reinterpret_cast<uintptr_t>(vtable.data());
 std::array<unsigned char,0x34> service{};uintptr_t apiPointer=reinterpret_cast<uintptr_t>(&api);std::memcpy(service.data()+0x30,&apiPointer,4);void* servicePointer=service.data();
 uintptr_t api2=reinterpret_cast<uintptr_t>(vtable.data());primaryApiObject=&api;secondaryApiObject=&api2;serviceStorage=service.data();replacementApi=reinterpret_cast<uintptr_t>(&api2);
 uintptr_t cpuImport=reinterpret_cast<uintptr_t>(&cpuInfo),waitImport=reinterpret_cast<uintptr_t>(&waitCallback),logImport=reinterpret_cast<uintptr_t>(&logEnabled),forbiddenImport=reinterpret_cast<uintptr_t>(&forbidden);
 unsigned channel=0;const char name[]="mat_queue_mode",unused[]="controlled failure";
 for(auto& fix:nativeDeviceCreationFixups){const void* target=nullptr;switch(fix[1]){
 case 0x100fe6b8:target=name;break;case 0x100fa1fc:target=&cpuImport;break;
 case 0x1016dfb8:target=&servicePointer;break;case 0x100fa0d4:target=&waitImport;break;
 case 0x10183bec:target=&channel;break;case 0x100fa200:target=&logImport;break;case 0x100fa204:target=&forbiddenImport;break;
 case 0x100fe6b4:case 0x100fe610:target=unused;break;default:return 3;}
 uintptr_t address=reinterpret_cast<uintptr_t>(target);std::memcpy(code+fix[0],&address,4);}
 for(auto& call:nativeDeviceCreationCalls){int32_t delta=static_cast<int32_t>(reinterpret_cast<uintptr_t>(&reference)-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&delta,4);}
 DWORD previous;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&previous))return 4;FlushInstructionCache(GetCurrentProcess(),code,332);
 auto native=reinterpret_cast<void*(__thiscall*)(void*,unsigned,unsigned,unsigned)>(code);
 std::ofstream output("outputs/native-device-creation-grid.bin",std::ios::binary|std::ios::trunc);if(!output)return 5;
 unsigned cases=0;
 for(int mode:{-2,-1,0,1,2})for(unsigned processors:{0u,1u,2u,255u})for(successAttempt=0;successAttempt<4;++successAttempt)for(unsigned fallback=0;fallback<2;++fallback){
  std::memcpy(convar.data()+0x30,&mode,4);cpu[6]=static_cast<unsigned char>(processors);
  std::array<unsigned char,0x70> owner{};unsigned one=1;std::memcpy(owner.data()+0x48,&one,4);std::memcpy(owner.data()+0x3c,&one,4);std::memcpy(owner.data()+0x50,&fallback,4);
  calls=waits=cpuCalls=flagsSeen=0;auto* result=native(owner.data(),0x1234,3,0x40);
  unsigned swap=0,count=0;std::memcpy(&swap,owner.data()+0x48,4);std::memcpy(&count,owner.data()+0x3c,4);
  std::array<unsigned,7> record{calls,waits,cpuCalls,flagsSeen,result?1u:0u,swap,count};
  output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));if(!output)return 6;++cases;
 }
 std::ofstream boundary("outputs/native-device-creation-boundary-grid.bin",std::ios::binary|std::ios::trunc);if(!boundary)return 7;
 unsigned boundaries=0;scripted=true;int mode=0;std::memcpy(convar.data()+0x30,&mode,4);
 for(auto first:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu})for(bool writeFirst:{false,true})
 for(auto second:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu})for(bool writeSecond:{false,true}){
  firstResult=first;secondResult=second;firstWrites=writeFirst;secondWrites=writeSecond;
  std::array<unsigned char,0x70> owner{};unsigned one=1;std::memcpy(owner.data()+0x48,&one,4);std::memcpy(owner.data()+0x3c,&one,4);
  calls=waits=cpuCalls=flagsSeen=0;auto* result=native(owner.data(),0x1234,3,0x40);
  unsigned swap=0,count=0;std::memcpy(&swap,owner.data()+0x48,4);std::memcpy(&count,owner.data()+0x3c,4);
  std::array<unsigned,7> record{calls,waits,cpuCalls,flagsSeen,result?1u:0u,swap,count};
  boundary.write(reinterpret_cast<const char*>(record.data()),sizeof(record));if(!boundary)return 8;++boundaries;
 }
 std::ofstream mutations("outputs/native-device-creation-mutation-grid.bin",std::ios::binary|std::ios::trunc);if(!mutations)return 9;
 unsigned mutationCases=0;scripted=false;
 for(unsigned cpuChange=0;cpuChange<2;++cpuChange)for(creationMutation=0;creationMutation<3;++creationMutation)for(unsigned windowed=0;windowed<2;++windowed)for(successAttempt=0;successAttempt<4;++successAttempt){
  mode=-2;std::memcpy(convar.data()+0x30,&mode,4);cpu[6]=1;cpuMutation=cpuChange!=0;
  std::array<unsigned char,0x70> owner{};unsigned one=1;std::memcpy(owner.data()+0x48,&one,4);std::memcpy(owner.data()+0x3c,&one,4);std::memcpy(owner.data()+0x50,&windowed,4);
  calls=waits=cpuCalls=flagsSeen=0;auto* result=native(owner.data(),0x1234,3,0x40);
  unsigned swap=0,count=0,liveWindowed=0;std::memcpy(&swap,owner.data()+0x48,4);std::memcpy(&count,owner.data()+0x3c,4);std::memcpy(&liveWindowed,owner.data()+0x50,4);std::memcpy(&mode,convar.data()+0x30,4);
  std::array<unsigned,9> record{calls,waits,cpuCalls,flagsSeen,result?1u:0u,swap,count,liveWindowed,static_cast<unsigned>(mode)};
  mutations.write(reinterpret_cast<const char*>(record.data()),sizeof(record));if(!mutations)return 10;++mutationCases;
 }
 std::ofstream interfaces("outputs/native-device-creation-interface-grid.bin",std::ios::binary|std::ios::trunc);if(!interfaces)return 11;
 unsigned interfaceCases=0;cpuMutation=false;creationMutation=0;mode=0;std::memcpy(convar.data()+0x30,&mode,4);
 for(int interfaceMode:{-2,-1,0,2})for(bool replace:{false,true})for(successAttempt=0;successAttempt<4;++successAttempt){
  std::memcpy(convar.data()+0x30,&interfaceMode,4);cpu[6]=2;
  replaceInterface=replace;std::memcpy(service.data()+0x30,&apiPointer,4);
  std::array<unsigned char,0x70> owner{};unsigned one=1;std::memcpy(owner.data()+0x48,&one,4);std::memcpy(owner.data()+0x3c,&one,4);std::memcpy(owner.data()+0x50,&one,4);
  calls=waits=cpuCalls=flagsSeen=apiRoute=0;auto* result=native(owner.data(),0x1234,3,0x40);
  unsigned swap=0,count=0;std::memcpy(&swap,owner.data()+0x48,4);std::memcpy(&count,owner.data()+0x3c,4);
  std::array<unsigned,8> record{calls,waits,cpuCalls,flagsSeen,result?1u:0u,swap,count,apiRoute};
  interfaces.write(reinterpret_cast<const char*>(record.data()),sizeof(record));if(!interfaces)return 12;++interfaceCases;
 }
 VirtualFree(code,0,MEM_RELEASE);std::printf("native_device_creation_controlled_cases=%u native_hresult_output_boundary_cases=%u native_callback_mutation_cases=%u native_interface_replacement_cases=%u gpu_executed=0\n",cases,boundaries,mutationCases,interfaceCases);
}

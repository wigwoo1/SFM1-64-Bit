#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include "shader_manager_restore_reset_native_init.h"
#include "studio_shader_manager_restore_reset_candidate.h"
static void* firstTable[11]{};static void* secondTable[11]{};
static uint32_t* field9c;static uint32_t* fieldA0;static void* receiver;
static bool mutate,replace;static unsigned firstCalls,secondCalls,targetId;static uint32_t observed9c,observedA0;
using Callback=void(__fastcall*)(void*,void*,uint32_t);
static void __fastcall first(void* self,void*,uint32_t value){if(self!=receiver||value!=0xffffffff||*field9c!=0xffffffff||*fieldA0!=0xffffffff)std::abort();++firstCalls;if(mutate){*field9c=42;*fieldA0=77;}if(replace)*static_cast<void***>(self)=secondTable;}
static void observe(void* self,uint32_t value,unsigned id){if(self!=receiver||value!=0xffffffff)std::abort();++secondCalls;targetId=id;observed9c=*field9c;observedA0=*fieldA0;}
static void __fastcall second1(void* self,void*,uint32_t value){observe(self,value,1);}
static void __fastcall second2(void* self,void*,uint32_t value){observe(self,value,2);}
struct Owner{uint32_t& cached9c;uint32_t& cachedA0;void* object;};
struct Ops{
 Callback captureSlot9(Owner& owner){auto* table=*static_cast<void***>(owner.object);return reinterpret_cast<Callback>(table[9]);}
 void invokeSlot9(Callback callback,Owner& owner,uint32_t value){callback(owner.object,nullptr,value);}
 void invokeSlot10(Owner& owner,uint32_t value){auto* table=*static_cast<void***>(owner.object);reinterpret_cast<Callback>(table[10])(owner.object,nullptr,value);}
};
int main(){
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);std::array<unsigned char,sizeof(managerRestoreResetBytes)> bytes{};dll.seekg(managerRestoreResetFileOffset);dll.read(reinterpret_cast<char*>(bytes.data()),bytes.size());if(!dll||std::memcmp(bytes.data(),managerRestoreResetBytes,bytes.size()))return 1;
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,bytes.data(),bytes.size());DWORD previous;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&previous))return 3;FlushInstructionCache(GetCurrentProcess(),code,4096);auto native=reinterpret_cast<void(__thiscall*)(void*)>(code);std::ofstream oracle("outputs/native-shader-manager-restore-reset.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-shader-manager-restore-reset.bin",std::ios::binary);
#endif
 firstTable[9]=reinterpret_cast<void*>(&first);firstTable[10]=reinterpret_cast<void*>(&second1);secondTable[10]=reinterpret_cast<void*>(&second2);unsigned cases=0;
 for(uint32_t initial:{0u,123u})for(bool changeFields:{false,true})for(bool changeTable:{false,true}){mutate=changeFields;replace=changeTable;firstCalls=secondCalls=targetId=0;observed9c=observedA0=0;
#ifdef _M_IX86
  std::array<uint32_t,42> original{};original[0]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(firstTable));field9c=original.data()+0x9c/4;fieldA0=original.data()+0xa0/4;receiver=original.data();*field9c=*fieldA0=initial;native(receiver);
#else
  struct Object{void** table;uint32_t a,b;};Object object{firstTable,initial,initial};receiver=&object;field9c=&object.a;fieldA0=&object.b;Owner owner{object.a,object.b,&object};Ops ops;studioShaderManagerRestoreResetCandidate(owner,ops);
#endif
  const std::array<uint32_t,8> record{firstCalls,secondCalls,targetId,observed9c,observedA0,*field9c,*fieldA0,*static_cast<void***>(receiver)==secondTable?2u:1u};
#ifdef _M_IX86
  oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
  std::array<uint32_t,8> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(record!=expected)return 4;
#endif
  if(!oracle)return 5;++cases;
 }
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 6;
#endif
 std::printf("shader_manager_restore_native_cases=%u passed\n",cases);
}

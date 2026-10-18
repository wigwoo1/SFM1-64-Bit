#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "profile_counter_native_init.h"
static void check(bool b){if(!b)std::abort();}
static unsigned threadValue,threadCalls,allocations,releases,arrayFrees,fallback=99;
static std::array<unsigned,1664> profile;
static std::array<char,64> nameStorage;
static const char existingName[]="existing";
static unsigned liveCount;
static unsigned __stdcall threadId(){++threadCalls;return threadValue;}
static int __cdecl compare(const char* a,const char* b){return std::strcmp(a,b);}
static void* __cdecl allocate(unsigned bytes){check(bytes<=nameStorage.size()&&++allocations==1);return nameStorage.data();}
static void __cdecl release(void* pointer){check(releases<liveCount);auto* names=profile.data()+0x15c8/4;check(pointer==reinterpret_cast<void*>(names[releases]));if(releases)check(names[releases-1]==0);++releases;}
static void __cdecl freeArray(void* pointer){check(pointer==nullptr&&++arrayFrees==1);}
static void __cdecl forbidden(){std::abort();}
int main(){static_assert(sizeof(void*)==4,"original oracle");std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tier0.dll)",std::ios::binary);auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code!=nullptr);auto threadFunction=&threadId;
 for(unsigned i=0;i<3;++i){auto& body=profileCounterBodies[i];unsigned char raw[250]{};dll.seekg(body.fileOffset);dll.read(reinterpret_cast<char*>(raw),body.size);check(bool(dll)&&!std::memcmp(raw,body.bytes,body.size));auto* target=code+i*512;std::memcpy(target,raw,body.size);
  for(unsigned n=0;n<body.fixupCount;++n){auto fix=body.fixups[n];uintptr_t address=fix[1]==0x1002f040?reinterpret_cast<uintptr_t>(&threadFunction):reinterpret_cast<uintptr_t>(&fallback);unsigned value=static_cast<unsigned>(address);std::memcpy(target+fix[0],&value,4);}
  for(unsigned n=0;n<body.callCount;++n){auto call=body.calls[n];uintptr_t address=call[1]==0x1001b1b1?reinterpret_cast<uintptr_t>(&compare):call[1]==0x1001e482?reinterpret_cast<uintptr_t>(&allocate):call[1]==0x1001e4d9?reinterpret_cast<uintptr_t>(&release):call[1]==0x1001b026?reinterpret_cast<uintptr_t>(&freeArray):reinterpret_cast<uintptr_t>(&forbidden);unsigned delta=static_cast<unsigned>(address-reinterpret_cast<uintptr_t>(target+call[0]+5));std::memcpy(target+call[0]+1,&delta,4);}
 }
 DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old)!=0&&FlushInstructionCache(GetCurrentProcess(),code,4096)!=0);auto create=reinterpret_cast<unsigned*(__thiscall*)(void*,const char*,unsigned)>(code);auto reset=reinterpret_cast<void(__thiscall*)(void*,int)>(code+512);auto cleanup=reinterpret_cast<void(__thiscall*)(void*)>(code+1024);unsigned cases=0;
 std::ofstream records("outputs/native-profile-counter-lifecycle.bin",std::ios::binary|std::ios::trunc);
 for(unsigned count:{0u,1u,254u,255u})for(bool sameThread:{false,true})for(bool sameName:{false,true})for(unsigned group:{2u,255u}){
 profile.fill(0xabababab);profile[0x10b4/4]=profile[0x10b8/4]=profile[0x10bc/4]=profile[0x1094/4]=0;profile[0x19c8/4]=count;profile[0x19cc/4]=42;for(unsigned i=0;i<256;++i){profile[0x10c8/4+i]=7+i;reinterpret_cast<unsigned char*>(profile.data())[0x14c8+i]=2;profile[0x15c8/4+i]=i<count?static_cast<unsigned>(reinterpret_cast<uintptr_t>(existingName)):0;}auto expected=profile;threadValue=sameThread?42u:43u;threadCalls=allocations=releases=arrayFrees=0;nameStorage.fill('!');fallback=99;const char* name=sameName?existingName:"new";
 bool rejected=count==255||!sameThread;bool insert=!rejected&&!(sameName&&count>0);unsigned index=insert?count:0;liveCount=count+unsigned(insert);
 if(insert){expected[0x10c8/4+count]=0;reinterpret_cast<unsigned char*>(expected.data())[0x14c8+count]=static_cast<unsigned char>(group);expected[0x15c8/4+count]=static_cast<unsigned>(reinterpret_cast<uintptr_t>(nameStorage.data()));expected[0x19c8/4]=liveCount;}
 auto returned=create(profile.data(),name,group);check(returned==(rejected?&fallback:profile.data()+0x10c8/4+index)&&profile==expected&&allocations==unsigned(insert)&&threadCalls==(count==255?0u:1u));if(insert)check(!std::strcmp(nameStorage.data(),name));
 int resetGroup=group==255?-1:2;for(unsigned i=0;i<liveCount;++i){auto byte=reinterpret_cast<unsigned char*>(expected.data())[0x14c8+i];if(static_cast<signed char>(byte)==resetGroup)expected[0x10c8/4+i]=0;}reset(profile.data(),resetGroup);check(profile==expected);
 for(unsigned i=0;i<liveCount;++i)expected[0x15c8/4+i]=0;expected[0x19c8/4]=0;cleanup(profile.data());check(profile==expected&&releases==liveCount&&arrayFrees==1&&fallback==99);
 std::array<unsigned,1670> record{};std::copy(profile.begin(),profile.end(),record.begin());record[1664]=rejected?0u:index+1;record[1665]=allocations;record[1666]=releases;record[1667]=threadCalls;record[1668]=arrayFrees;record[1669]=fallback;records.write(reinterpret_cast<const char*>(record.data()),sizeof(record));check(bool(records));++cases;
 }
 check(VirtualFree(code,0,MEM_RELEASE)!=0);std::printf("original_profile_counter_lifecycle_cases=%u passed\n",cases);
}

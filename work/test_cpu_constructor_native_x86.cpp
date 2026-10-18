#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstdio>
#include <cstring>
#include "cpu_constructor_native_init.h"
#include "cpu_id_masks_native_init.h"
static std::array<uint32_t,8> cache{};
static unsigned total=16,cores=4;static bool intel=true;
static DWORD processMask=1,systemMask=3,threadMask=4,processCalls=0,threadCalls=0,sleeps=0;
static HANDLE __stdcall process(){return reinterpret_cast<HANDLE>(0x111);}
static HANDLE __stdcall thread(){return reinterpret_cast<HANDLE>(0x222);}
static BOOL __stdcall masks(HANDLE handle,PDWORD p,PDWORD s){if(handle!=process())ExitProcess(20);*p=processMask;*s=systemMask;return TRUE;}
static BOOL __stdcall setProcess(HANDLE handle,DWORD value){if(handle!=process())ExitProcess(21);++processCalls;processMask=value;return TRUE;}
static DWORD __stdcall setThread(HANDLE handle,DWORD value){if(handle!=thread())ExitProcess(22);++threadCalls;DWORD previous=threadMask;threadMask=value;return previous;}
static void __stdcall sleepCallback(DWORD value){if(value)ExitProcess(23);++sleeps;}
static void __fastcall cpuid(void* object,void*,unsigned a,unsigned b){
 unsigned leaf=a|b;std::array<uint32_t,4> result{};
 if(leaf==0)result=intel?std::array<uint32_t,4>{4,0x756e6547,0x6c65746e,0x49656e69}:std::array<uint32_t,4>{4,0x68747541,0x444d4163,0x69746e65};
 else if(leaf==0x80000000u)result[0]=0x80000008u;
 else if(leaf==1){unsigned bit=0;DWORD value=threadMask;while(value>1){++bit;value>>=1;}result[1]=(total<<16)|((bit+10u)<<24);result[3]=0x10000000u;}
 else if(leaf==4)result[0]=(cores-1u)<<26;
 else if(leaf==0x80000008u)result[2]=cores-1u;
 else ExitProcess(24);
 std::memcpy(object,result.data(),16);
}
int main(){static_assert(sizeof(void*)==4,"x86 required");
 std::ifstream dll(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tier0.dll)",std::ios::binary);
 std::array<unsigned char,817> original{};dll.seekg(nativeCpuConstructorFileOffset);dll.read(reinterpret_cast<char*>(original.data()),original.size());if(!dll||std::memcmp(original.data(),nativeCpuConstructorBytes,original.size()))return 1;
 std::array<unsigned char,130> maskBytes{};dll.seekg(nativeCpuMaskOffset);dll.read(reinterpret_cast<char*>(maskBytes.data()),maskBytes.size());if(!dll||std::memcmp(maskBytes.data(),nativeCpuMaskBytes,maskBytes.size()))return 2;
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 3;std::memcpy(code,original.data(),original.size());std::memcpy(code+1024,maskBytes.data(),maskBytes.size());
 uintptr_t imports[]{reinterpret_cast<uintptr_t>(&masks),reinterpret_cast<uintptr_t>(&process),reinterpret_cast<uintptr_t>(&setThread),reinterpret_cast<uintptr_t>(&thread),reinterpret_cast<uintptr_t>(&setProcess),reinterpret_cast<uintptr_t>(&sleepCallback)};unsigned vtable=0;
 for(auto& fix:nativeCpuConstructorFixups){const void* target=nullptr;
  if(fix[1]>=0x1003bce4&&fix[1]<=0x1003bd00)target=reinterpret_cast<unsigned char*>(cache.data())+fix[1]-0x1003bce4;
  else if(fix[1]>=0x1002f068&&fix[1]<=0x1002f07c&&(fix[1]-0x1002f068)%4==0)target=&imports[(fix[1]-0x1002f068)/4];
  else if(fix[1]==0x1002f590)target=&vtable;else return 4;
  uintptr_t address=reinterpret_cast<uintptr_t>(target);std::memcpy(code+fix[0],&address,4);
 }
 for(auto& call:nativeCpuConstructorCalls){uintptr_t target=call[1]==0x10002ec0?reinterpret_cast<uintptr_t>(&cpuid):call[1]==0x100030e0?reinterpret_cast<uintptr_t>(code+1024):0;if(!target)return 5;int32_t delta=static_cast<int32_t>(target-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&delta,4);}
 DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 6;FlushInstructionCache(GetCurrentProcess(),code,4096);auto native=reinterpret_cast<void*(__thiscall*)(void*)>(code);
 std::ofstream output("outputs/native-cpu-constructor-grid.bin",std::ios::binary|std::ios::trunc);if(!output)return 7;unsigned cases=0;
 for(bool vendor:{false,true})for(unsigned logical:{1u,16u,32u})for(unsigned coreCount:{1u,4u,8u})for(DWORD system:{1u,3u,5u,0x80000001u})for(bool restricted:{false,true}){
  intel=vendor;total=logical;cores=coreCount;cache.fill(0);systemMask=system;processMask=restricted?1:system;DWORD initialProcess=processMask;threadMask=4;processCalls=threadCalls=sleeps=0;
  std::array<unsigned char,0x30> owner{};if(native(owner.data())!=owner.data())return 8;
  if(processMask!=initialProcess||threadMask!=4)return 9;
  std::array<uint32_t,16> record{};std::memcpy(record.data(),owner.data()+0x25,8);std::memcpy(record.data()+2,owner.data()+4,32);record[10]=owner[0x24];record[11]=processMask;record[12]=threadMask;record[13]=processCalls;record[14]=threadCalls;record[15]=sleeps;
  output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));if(!output)return 10;++cases;
 }
 VirtualFree(code,0,MEM_RELEASE);std::printf("native_cpu_constructor_controlled_cases=%u live_affinity_changed=0\n",cases);
}

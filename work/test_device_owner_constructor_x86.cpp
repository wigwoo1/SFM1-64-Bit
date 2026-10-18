#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstdio>
#include <cstring>
#include "device_owner_constructor_native_init.h"
static unsigned char* owner=nullptr;static unsigned baseCalls=0,clearCalls=0;
static void __fastcall base(void* object,void*){if(object!=owner)ExitProcess(20);++baseCalls;uint32_t marker=0x12345678;std::memcpy(owner+8,&marker,4);int cache=42;std::memcpy(owner+0x6c,&cache,4);}
static void* __cdecl clear(void* object,int value,size_t size){if(object!=owner+0x70||value||size!=0x38||baseCalls!=1)ExitProcess(21);++clearCalls;uint32_t second=0;std::memcpy(&second,owner+0x74,4);if(second!=1)ExitProcess(22);return std::memset(object,value,size);}
int main(){static_assert(sizeof(void*)==4,"x86 required");std::ifstream dll(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);unsigned char installed[sizeof(nativeOwnerConstructorBytes)]{};dll.seekg(nativeOwnerConstructorOffset);dll.read(reinterpret_cast<char*>(installed),sizeof(installed));if(!dll||std::memcmp(installed,nativeOwnerConstructorBytes,sizeof(installed)))return 1;
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,installed,sizeof(installed));unsigned vtable=0;uintptr_t address=reinterpret_cast<uintptr_t>(&vtable);std::memcpy(code+15,&address,4);
 for(unsigned offset:{5u,51u}){uintptr_t target=offset==5?reinterpret_cast<uintptr_t>(&base):reinterpret_cast<uintptr_t>(&clear);int32_t delta=static_cast<int32_t>(target-reinterpret_cast<uintptr_t>(code+offset+5));std::memcpy(code+offset+1,&delta,4);}
 DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 3;FlushInstructionCache(GetCurrentProcess(),code,sizeof(installed));auto native=reinterpret_cast<void*(__thiscall*)(void*)>(code);
 std::ofstream output("outputs/native-device-owner-constructor.bin",std::ios::binary|std::ios::trunc);if(!output)return 4;
 for(unsigned pattern=0;pattern<256;++pattern){std::array<unsigned char,0x2d0> storage;storage.fill(static_cast<unsigned char>(pattern));owner=storage.data();baseCalls=clearCalls=0;if(native(owner)!=owner||baseCalls!=1||clearCalls!=1)return 5;int cache=0;uint32_t marker=0;std::memcpy(&cache,owner+0x6c,4);std::memcpy(&marker,owner+8,4);if(cache!=-1||marker!=0x12345678||owner[0xac]!=(pattern&0xc0))return 6;std::memset(owner,0,4);output.write(reinterpret_cast<char*>(owner),storage.size());if(!output)return 7;}
 VirtualFree(code,0,MEM_RELEASE);std::printf("native_device_owner_constructor_cases=256 controlled_base_and_clear=passed gpu_executed=0\n");
}

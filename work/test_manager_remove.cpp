#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include "manager_remove_native_init.h"
#include "studio_manager_remove_candidate.h"
static unsigned moves=0;static void* __cdecl moveData(void* to,const void* from,unsigned bytes){++moves;return std::memmove(to,from,bytes);}struct Manager{uintptr_t* callbacks;int32_t count;};struct Services{void move(uintptr_t* to,uintptr_t* from,int32_t count){moveData(to,from,static_cast<unsigned>(count)*sizeof(uintptr_t));}};
int main(){
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);unsigned char installed[sizeof(removeBytes)]{},wrapper[sizeof(removeWrapperBytes)]{};dll.seekg(removeOffset);dll.read(reinterpret_cast<char*>(installed),sizeof(installed));dll.seekg(removeWrapperOffset);dll.read(reinterpret_cast<char*>(wrapper),sizeof(wrapper));if(!dll||std::memcmp(installed,removeBytes,sizeof(installed))||std::memcmp(wrapper,removeWrapperBytes,sizeof(wrapper)))return 1;auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,installed,sizeof(installed));std::memcpy(code+256,wrapper,sizeof(wrapper));int32_t delta=static_cast<int32_t>(reinterpret_cast<uintptr_t>(&moveData)-reinterpret_cast<uintptr_t>(code+83));std::memcpy(code+79,&delta,4);delta=-271;std::memcpy(code+267,&delta,4);DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 3;FlushInstructionCache(GetCurrentProcess(),code,4096);auto native=reinterpret_cast<unsigned char(__thiscall*)(void*,uintptr_t)>(code+256);std::ofstream oracle("outputs/native-manager-remove.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-manager-remove.bin",std::ios::binary);
#endif
 unsigned cases=0;for(int32_t count:{0,1,4})for(bool duplicate:{false,true})for(uintptr_t value:{uintptr_t{999},uintptr_t{1000},uintptr_t{1002},uintptr_t{1003}}){
 std::array<uintptr_t,8> storage{};for(unsigned i=0;i<8;++i)storage[i]=duplicate?1000:1000+i;moves=0;Manager manager{storage.data(),count};bool result;
#ifdef _M_IX86
 std::array<uint32_t,12> owner{};owner[6]=reinterpret_cast<uintptr_t>(storage.data());owner[9]=static_cast<uint32_t>(count);result=native(owner.data(),value)!=0;manager.count=static_cast<int32_t>(owner[9]);
#else
 Services services;result=studioManagerRemoveCandidate(manager,value,services);
#endif
 std::array<unsigned,11> record{result?1u:0u,static_cast<unsigned>(manager.count),moves};for(unsigned i=0;i<8;++i)record[3+i]=static_cast<unsigned>(storage[i]);
#ifdef _M_IX86
 oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
 std::array<unsigned,11> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(record!=expected)return 4;
#endif
 if(!oracle)return 5;++cases;}
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 6;uintptr_t storage[]={0x100001234ull,0x100005678ull};Manager manager{storage,2};Services services;if(!studioManagerRemoveCandidate(manager,uintptr_t{0x100001234ull},services)||manager.count!=1||storage[0]!=0x100005678ull)return 7;
#endif
 std::printf("manager_remove_native_cases=%u matched=passed\n",cases);}

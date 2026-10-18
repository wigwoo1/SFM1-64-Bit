#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include "manager_cleanup_native_init.h"
#include "studio_manager_cleanup_candidate.h"
using Callback=void(__cdecl*)();struct Manager{Callback* callbacks;int32_t count;};static Manager manager{};static std::array<uint32_t,10> nativeOwner{};static bool mutate=false;static unsigned trace=0,calls=0;static Callback secondary[4];
static void record(unsigned value){trace=trace*10+value;++calls;}
static void __cdecl first(){record(1);if(mutate){manager.callbacks=secondary;manager.count=0;
#ifdef _M_IX86
 nativeOwner[6]=reinterpret_cast<uintptr_t>(secondary);nativeOwner[9]=0;
#endif
}}
static void __cdecl two(){record(2);}static void __cdecl three(){record(3);}static void __cdecl four(){record(4);}static void __cdecl five(){record(5);}static void __cdecl six(){record(6);}static void __cdecl seven(){record(7);}static void __cdecl eight(){record(8);}
int main(){Callback primary[]={first,two,three,four};secondary[0]=five;secondary[1]=six;secondary[2]=seven;secondary[3]=eight;
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);unsigned char installed[sizeof(cleanupBytes)]{};dll.seekg(cleanupOffset);dll.read(reinterpret_cast<char*>(installed),sizeof(installed));if(!dll||std::memcmp(installed,cleanupBytes,sizeof(installed)))return 1;auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,installed,sizeof(installed));DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 3;FlushInstructionCache(GetCurrentProcess(),code,sizeof(installed));auto native=reinterpret_cast<void(__thiscall*)(void*)>(code);std::ofstream oracle("outputs/native-manager-cleanup.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-manager-cleanup.bin",std::ios::binary);
#endif
 unsigned cases=0;for(int32_t count:{-1,0,1,2,4})for(bool change:{false,true}){manager={primary,count};mutate=change;trace=calls=0;
#ifdef _M_IX86
 nativeOwner[6]=reinterpret_cast<uintptr_t>(primary);nativeOwner[9]=static_cast<uint32_t>(count);native(nativeOwner.data());const int32_t finalCount=static_cast<int32_t>(nativeOwner[9]);
#else
 studioManagerCleanupCandidate(manager);const int32_t finalCount=manager.count;
#endif
 std::array<unsigned,3> result{trace,calls,static_cast<unsigned>(finalCount)};
#ifdef _M_IX86
 oracle.write(reinterpret_cast<const char*>(result.data()),sizeof(result));
#else
 std::array<unsigned,3> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(result!=expected)return 4;
#endif
 if(!oracle)return 5;++cases;
 }
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 6;
#endif
 std::printf("manager_cleanup_cases=%u captured_count_live_callback_array=passed\n",cases);}

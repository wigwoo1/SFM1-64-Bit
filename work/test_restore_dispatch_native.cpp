#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include "restore_dispatch_native_init.h"
#include "native_manager_callback_registry_candidate.h"
#include "native_restore_callback_lists_candidate.h"
using Callback=void(*)(void*,void*,void*,bool);
static bool flipSecondSuppression;static bool crossCategory;static void (*appendSecond)();static bool mutateLists;static unsigned appendedCalls;static void (*mutateList)(unsigned);
static void appended(void*,void*,void*,bool){++appendedCalls;}
struct MovingAllocator{std::size_t size=0;void* allocate(std::size_t n){size=n;return std::malloc(n);}void* reallocate(void* p,std::size_t n){void* q=std::malloc(n);if(!q)return nullptr;std::memcpy(q,p,size);std::free(p);size=n;return q;}void release(void* p){std::free(p);}};
using Registry=NativeManagerCallbackRegistryCandidateT<MovingAllocator,Callback>;
#ifdef _M_IX86
static uint32_t* fixtureOwner;static Callback* oldLists[2];static Callback replacementLists[2][16];
static void relocate(unsigned category){auto* replacement=replacementLists[category];const auto count=fixtureOwner[(category?0x2eac:0x2e98)/4];std::memcpy(replacement,oldLists[category],count*sizeof(Callback));replacement[count]=appended;fixtureOwner[(category?0x2ea0:0x2e8c)/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(replacement));fixtureOwner[(category?0x2eac:0x2e98)/4]=count+1;oldLists[category][1]=appended;}
static void addSecond(){auto& count=fixtureOwner[0x2eac/4];oldLists[1][count++]=appended;}
#else
static Registry* activeLists[2];static void relocate(unsigned category){activeLists[category]->append(appended);}
static void addSecond(){activeLists[1]->append(appended);}
#endif
static bool suppressed[2];static unsigned gates,calls[2];static uint8_t* flags[2];static void* expectedArguments[3];
static void observe(unsigned category,void* command,void* current,void* previous,bool force){if(command!=expectedArguments[0]||current!=expectedArguments[1]||previous!=expectedArguments[2]||!force)std::abort();++calls[category];*flags[category]|=0x40;if(calls[category]==1){if(flipSecondSuppression&&category==0)suppressed[1]=!suppressed[1];if(crossCategory&&category==0){appendSecond();*flags[1]|=0x20;}if(mutateLists)mutateList(category);}}
static void first(void* c,void* a,void* b,bool f){observe(0,c,a,b,f);}static void second(void* c,void* a,void* b,bool f){observe(1,c,a,b,f);}
#ifdef _M_IX86
static unsigned char* code;
static bool __fastcall gate(void*,void*){return suppressed[gates++];}
static void invoke(void* owner){
#ifdef RESTORE_DISPATCH_DIAGNOSTIC_NO_ASM
 (void)owner;
#else
 __asm{push ebx
 push esi
 push edi
 mov ebx,owner
 call code
 pop edi
 pop esi
 pop ebx}
#endif
}
#endif
int main(){
#ifdef _M_IX86
 std::ifstream verified("outputs/restore-dispatch-verified-bytes.bin",std::ios::binary);std::array<unsigned char,restoreDispatchByteCount> verifiedBytes{};verified.read(reinterpret_cast<char*>(verifiedBytes.data()),verifiedBytes.size());if(verified.peek()!=std::char_traits<char>::eof())return 7;std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);std::array<unsigned char,restoreDispatchByteCount> bytes{};dll.seekg(restoreDispatchFileOffset);dll.read(reinterpret_cast<char*>(bytes.data()),bytes.size());if(!dll||!verified||bytes!=verifiedBytes)return 1;code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,bytes.data(),bytes.size());code[bytes.size()]=0xc3;DWORD protection;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&protection))return 3;FlushInstructionCache(GetCurrentProcess(),code,4096);void* table[37]{};table[36]=reinterpret_cast<void*>(&gate);
 std::ofstream oracle("outputs/native-restore-dispatch.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-restore-dispatch.bin",std::ios::binary);
#endif
 unsigned cases=0;for(bool flip:{false,true})for(bool cross:{false,true})for(bool mutation:{false,true})for(bool zeroFirst:{false,true})for(bool zeroSecond:{false,true})for(bool suppressFirst:{false,true})for(bool suppressSecond:{false,true}){
  flipSecondSuppression=flip;crossCategory=cross;appendSecond=addSecond;mutateLists=mutation;mutateList=relocate;appendedCalls=0;suppressed[0]=suppressFirst;suppressed[1]=suppressSecond;gates=calls[0]=calls[1]=0;uint8_t firstFlags=0x82,secondFlags=0x81;uint32_t firstCount=zeroFirst?0:8,secondCount=zeroSecond?0:8;
#ifdef _M_IX86
  std::array<uint32_t,4096> owner{};owner[0]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(table));Callback firstList[16]{},secondList[16]{};for(unsigned i=0;i<8;++i){firstList[i]=first;secondList[i]=second;}fixtureOwner=owner.data();oldLists[0]=firstList;oldLists[1]=secondList;owner[0x2e8c/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(firstList));owner[0x2ea0/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(secondList));owner[0x2e98/4]=firstCount;owner[0x2eac/4]=secondCount;auto* base=reinterpret_cast<unsigned char*>(owner.data());flags[0]=base+0x2e88;flags[1]=base+0x2e89;*flags[0]=firstFlags;*flags[1]=secondFlags;expectedArguments[0]=base+0x648;expectedArguments[1]=base+0x2010;expectedArguments[2]=base+0x1198;invoke(owner.data());firstFlags=*flags[0];secondFlags=*flags[1];firstCount=owner[0x2e98/4];secondCount=owner[0x2eac/4];
#else
  flags[0]=&firstFlags;flags[1]=&secondFlags;uint32_t argumentObjects[3]{};for(unsigned i=0;i<3;++i)expectedArguments[i]=&argumentObjects[i];MovingAllocator allocatorA,allocatorB;Registry a{allocatorA},b{allocatorB};activeLists[0]=&a;activeLists[1]=&b;for(unsigned i=0;i<firstCount;++i)a.append(first);for(unsigned i=0;i<secondCount;++i)b.append(second);nativeRestoreCallbackListsCandidate(a,firstFlags,b,secondFlags,[]{return suppressed[gates++];},expectedArguments[0],expectedArguments[1],expectedArguments[2],true);firstCount=a.size();secondCount=b.size();
#endif
  const bool finalSecondSuppression=suppressSecond^(flip&&!zeroFirst&&!suppressFirst);const unsigned expectedAppended=cross&&!zeroFirst&&!suppressFirst&&!finalSecondSuppression?1u:0u;if(appendedCalls!=expectedAppended||calls[0]!=(zeroFirst||suppressFirst?0u:8u)||calls[1]!=(zeroSecond||finalSecondSuppression?0u:8u))return 8;std::array<uint32_t,8> record{gates,calls[0],calls[1],firstFlags,secondFlags,firstCount,secondCount,appendedCalls};
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
 std::printf("restore_dispatch_original_cases=%u passed\n",cases);
 return 0;
}



#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include "restore_tail_native_init.h"
#include "native_restore_dispatch_snapshot_finish_candidate.h"
#include "native_manager_callback_registry_candidate.h"
static constexpr unsigned slots[]{0x2858,0x285c,0x2860,0x2864,0x2868,0x286c};
using Owner=std::array<uint32_t,4096>;
static unsigned gates,callbackCalls[2];static bool suppressed[2];
static void* expectedArgs[3];
static void callback(unsigned category,void* command,void* current,void* previous,bool force){if(command!=expectedArgs[0]||current!=expectedArgs[1]||previous!=expectedArgs[2]||!force)std::abort();++callbackCalls[category];static_cast<uint32_t*>(previous)[17+category*2]=0xabc00000u+category;}
static void first(void* c,void* a,void* b,bool f){callback(0,c,a,b,f);}
static void second(void* c,void* a,void* b,bool f){callback(1,c,a,b,f);}
#ifdef _M_IX86
static bool __fastcall gate(void*,void*){return suppressed[gates++];}
#endif
static Owner* activeOwner;
#ifndef _M_IX86
using Registry=NativeManagerCallbackRegistryCandidateT<NativeManagerCallbackAllocatorCandidate,void(*)(void*,void*,void*,bool)>;static Registry* lists[2];
#endif
static Owner* expectedAtFinish;static unsigned finishCalls;static bool mutateFinish;
static void finish(bool value){if(gates!=2||!value||*activeOwner!=*expectedAtFinish)std::abort();++finishCalls;if(mutateFinish)reinterpret_cast<unsigned char*>(activeOwner->data())[0x30d8]=7;}
#ifdef _M_IX86
static void* expectedReceiver;static void __fastcall complete(void* receiver,void*,bool value){if(receiver!=expectedReceiver)std::abort();finish(value);}
#endif
struct Services{Owner& owner;
 void clearRestoreForce(){reinterpret_cast<unsigned char*>(owner.data())[0x30d8]=0;}
 void completeRestore(bool value){
#ifndef _M_IX86
 owner[0x2e98/4]=lists[0]->size();owner[0x2eac/4]=lists[1]->size();
#endif
 finish(value);}
 std::array<uint32_t,6> staging(){std::array<uint32_t,6> r{};for(unsigned i=0;i<6;++i)r[i]=owner[slots[i]/4];return r;}
 void copyState(){std::memcpy(reinterpret_cast<unsigned char*>(owner.data())+0x2010,reinterpret_cast<unsigned char*>(owner.data())+0x1198,3704);}
 void restoreStaging(const std::array<uint32_t,6>& r){for(unsigned i=0;i<6;++i)owner[slots[i]/4]=r[i];}
};
#ifdef _M_IX86
static unsigned char* code;
static __declspec(naked) void invoke(void*,void*){__asm{
 push ebp
 push ebx
 push esi
 push edi
 mov ebx,[esp+20]
 mov ebp,[esp+24]
 cld
 call code
 pop edi
 pop esi
 pop ebx
 pop ebp
 ret
}}
#endif
int main(){
#ifdef _M_IX86
 std::array<unsigned char,restoreTailByteCount> verified{},actual{};
 std::ifstream artifact("outputs/restore-tail-verified-bytes.bin",std::ios::binary);
 artifact.read(reinterpret_cast<char*>(verified.data()),verified.size());if(!artifact)return 1;
 if(artifact.peek()!=std::char_traits<char>::eof())return 1;
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);
 dll.seekg(restoreTailFileOffset);dll.read(reinterpret_cast<char*>(actual.data()),actual.size());if(!dll||actual!=verified)return 2;
 code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 3;
 std::memcpy(code,actual.data(),actual.size());code[actual.size()]=0xc3;DWORD old;
 if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 4;FlushInstructionCache(GetCurrentProcess(),code,4096);
 std::ofstream oracle("outputs/native-restore-tail.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-restore-tail.bin",std::ios::binary);
#endif
 for(unsigned seed=0;seed<512;++seed){Owner owner{};uint32_t random=seed+1;for(auto& word:owner){random=random*1664525u+1013904223u;word=random;}
  finishCalls=0;mutateFinish=(seed&1)!=0;activeOwner=&owner;gates=callbackCalls[0]=callbackCalls[1]=0;suppressed[0]=(seed&2)!=0;suppressed[1]=(seed&4)!=0;const unsigned counts[]{(seed&8)?0u:2u,(seed&16)?0u:2u};
  auto* base=reinterpret_cast<unsigned char*>(owner.data());expectedArgs[0]=base+0x648;expectedArgs[1]=base+0x2010;expectedArgs[2]=base+0x1198;
  owner[0]=owner[0x2e8c/4]=owner[0x2ea0/4]=0;owner[0x2e98/4]=counts[0];owner[0x2eac/4]=counts[1];base[0x2e88]=0x82;base[0x2e89]=0x81;
#ifdef _M_IX86
  void* ownerTable[37]{};ownerTable[36]=reinterpret_cast<void*>(&gate);void* list0[]{reinterpret_cast<void*>(&first),reinterpret_cast<void*>(&first)},*list1[]{reinterpret_cast<void*>(&second),reinterpret_cast<void*>(&second)};owner[0]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ownerTable));owner[0x2e8c/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(list0));owner[0x2ea0/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(list1));
#endif
  Owner before=owner;for(unsigned category=0;category<2;++category)if(counts[category]&&!suppressed[category]){before[(0x1198/4)+17+category*2]=0xabc00000u+category;reinterpret_cast<unsigned char*>(before.data())[0x2e88+category]=0;before[(category?0x2eac:0x2e98)/4]=0;}Owner expected=before;Services reference{expected};reference.copyState();for(unsigned i=0;i<6;++i)expected[slots[i]/4]=before[slots[i]/4];reinterpret_cast<unsigned char*>(expected.data())[0x30d8]=0;expectedAtFinish=&expected;
#ifdef _M_IX86
  void* table[184]{};table[183]=reinterpret_cast<void*>(&complete);uint32_t receiver=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(table));expectedReceiver=&receiver;std::array<uint32_t,32> locals{};auto* frame=locals.data()+24;frame[-0x40/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&receiver));invoke(owner.data(),frame);
#else
  Services services{owner};using Callback=void(*)(void*,void*,void*,bool);NativeManagerCallbackAllocatorCandidate allocator;NativeManagerCallbackRegistryCandidateT<NativeManagerCallbackAllocatorCandidate,Callback> a{allocator},b{allocator};for(unsigned i=0;i<counts[0];++i)a.append(first);for(unsigned i=0;i<counts[1];++i)b.append(second);
  // Read actual owned registry counts at completion for wire-layout comparison.
  lists[0]=&a;lists[1]=&b;auto liveGate=[&]{return suppressed[gates++];};
  nativeRestoreDispatchSnapshotFinishCandidate(a,base[0x2e88],b,base[0x2e89],liveGate,services,expectedArgs[0],expectedArgs[1],expectedArgs[2],true);
#endif
  if(finishCalls!=1)return 9;if(mutateFinish)reinterpret_cast<unsigned char*>(expected.data())[0x30d8]=7;if(owner!=expected)return 5;for(unsigned category=0;category<2;++category)if(callbackCalls[category]!=(suppressed[category]?0u:counts[category]))return 10;
  owner[0]=owner[0x2e8c/4]=owner[0x2ea0/4]=0;
#ifdef _M_IX86
  oracle.write(reinterpret_cast<const char*>(owner.data()),sizeof(owner));
#else
  Owner native{};oracle.read(reinterpret_cast<char*>(native.data()),sizeof(native));if(native!=owner)return 6;
#endif
  if(!oracle)return 7;
 }
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 8;
#endif
 std::puts("restore_tail_original_cases=512 passed");return 0;
}



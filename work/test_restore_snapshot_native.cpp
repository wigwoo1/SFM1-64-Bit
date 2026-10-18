#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include "restore_snapshot_native_init.h"
#include "native_restore_snapshot_finish_candidate.h"
static constexpr unsigned slots[]{0x2858,0x285c,0x2860,0x2864,0x2868,0x286c};
using Owner=std::array<uint32_t,4096>;
static Owner* activeOwner;static Owner* expectedAtFinish;static unsigned finishCalls;static bool mutateFinish;
static void finish(bool value){if(!value||*activeOwner!=*expectedAtFinish)std::abort();++finishCalls;if(mutateFinish)reinterpret_cast<unsigned char*>(activeOwner->data())[0x30d8]=7;}
#ifdef _M_IX86
static void* expectedReceiver;static void __fastcall complete(void* receiver,void*,bool value){if(receiver!=expectedReceiver)std::abort();finish(value);}
#endif
struct Services{Owner& owner;
 void clearRestoreForce(){reinterpret_cast<unsigned char*>(owner.data())[0x30d8]=0;}
 void completeRestore(bool value){finish(value);}
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
 std::array<unsigned char,restoreSnapshotByteCount> verified{},actual{};
 std::ifstream artifact("outputs/restore-snapshot-verified-bytes.bin",std::ios::binary);
 artifact.read(reinterpret_cast<char*>(verified.data()),verified.size());if(!artifact)return 1;
 if(artifact.peek()!=std::char_traits<char>::eof())return 1;
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);
 dll.seekg(restoreSnapshotFileOffset);dll.read(reinterpret_cast<char*>(actual.data()),actual.size());if(!dll||actual!=verified)return 2;
 code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 3;
 std::memcpy(code,actual.data(),actual.size());code[actual.size()]=0xc3;DWORD old;
 if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 4;FlushInstructionCache(GetCurrentProcess(),code,4096);
 std::ofstream oracle("outputs/native-restore-snapshot.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-restore-snapshot.bin",std::ios::binary);
#endif
 for(unsigned seed=0;seed<512;++seed){Owner owner{};uint32_t random=seed+1;for(auto& word:owner){random=random*1664525u+1013904223u;word=random;}
  finishCalls=0;mutateFinish=(seed&1)!=0;activeOwner=&owner;const Owner before=owner;Owner expected=before;Services reference{expected};reference.copyState();for(unsigned i=0;i<6;++i)expected[slots[i]/4]=before[slots[i]/4];reinterpret_cast<unsigned char*>(expected.data())[0x30d8]=0;expectedAtFinish=&expected;
#ifdef _M_IX86
  void* table[184]{};table[183]=reinterpret_cast<void*>(&complete);uint32_t receiver=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(table));expectedReceiver=&receiver;std::array<uint32_t,32> locals{};auto* frame=locals.data()+24;frame[-0x40/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&receiver));invoke(owner.data(),frame);
#else
  Services services{owner};nativeRestoreSnapshotFinishCandidate(services);
#endif
  if(finishCalls!=1)return 9;if(mutateFinish)reinterpret_cast<unsigned char*>(expected.data())[0x30d8]=7;if(owner!=expected)return 5;
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
 std::puts("restore_snapshot_original_cases=512 passed");return 0;
}

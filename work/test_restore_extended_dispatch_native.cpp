#define NOMINMAX
#include <Windows.h>
#include <array>
#include <algorithm>
#include <vector>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <xmmintrin.h>
#include "vertex_constant_runs_native_init.h"
#include "restore_extended_dispatch_loop_init.h"
#include "studio_extended_constants_callback_candidate.h"
#include "native_manager_callback_registry_candidate.h"
static std::vector<uint32_t> events;static bool mutate;static float* liveSource;
static void submitDirect(uint32_t start,const float* source,uint32_t count){events.insert(events.end(),{start,count});for(unsigned i=0;i<count*4;++i){uint32_t bits;std::memcpy(&bits,source+i,4);events.push_back(bits);}if(mutate)liveSource[start*4]=42;}
#ifdef _M_IX86
static unsigned char* runCode;static void* observerArguments[3];static unsigned observerCalls;
static void __cdecl observer(void* command,void* current,void* cache,uint32_t force){if(command!=observerArguments[0]||current!=observerArguments[1]||cache!=observerArguments[2]||force!=1||observerCalls!=0)std::abort();++observerCalls;}
static void __fastcall refill(void*,void*){std::abort();}
static void* __cdecl copy(void* d,const void* s,uint32_t n){return std::memmove(d,s,n);}
static HRESULT __stdcall direct(void*,uint32_t start,const float* source,uint32_t count){submitDirect(start,source,count);return E_FAIL;}
static void* loopCode;
static __declspec(naked) void invokeLoop(void*){__asm{
 push ebx
 push esi
 push edi
 mov ebx,[esp+16]
 mov esi,2
 xor edi,edi
 call loopCode
 pop edi
 pop esi
 pop ebx
 ret
}}
static void invoke(void* command,void* current,void* cache,uint32_t force){
 if(force!=1)std::abort();std::array<uint32_t,4096> owner{};owner.fill(0xabababab);
 std::memcpy(owner.data()+0x648/4,command,32);std::memcpy(owner.data()+0x2010/4,current,531*4);std::memcpy(owner.data()+0x1198/4,cache,531*4);
 observerArguments[0]=owner.data()+0x648/4;observerArguments[1]=owner.data()+0x2010/4;observerArguments[2]=owner.data()+0x1198/4;observerCalls=0;void* callbacks[]{runCode+1024,reinterpret_cast<void*>(&observer)};owner[0x2e8c/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(callbacks));auto expected=owner;
 if(owner[0x658/4]){expected[0x660/4]+=2051*4;expected[0x664/4]-=2051;}
 invokeLoop(owner.data());if(owner!=expected||observerCalls!=1)std::abort();std::memcpy(command,owner.data()+0x648/4,32);
}
#endif
struct Ops{
 float* source;float* target;bool queued;uint32_t*& cursor;uint32_t& remaining;
 float* current(){return source;}float* cache(){return target;}
 void submit(uint32_t start,uint32_t count,const float* data){if(!queued){submitDirect(start,data,count);return;}const unsigned words=count*4+3;if(remaining<words)std::abort();remaining-=words;*cursor++=10;*cursor++=start;*cursor++=count;std::memcpy(cursor,data,count*16);cursor+=count*4;}
};
#ifndef _M_IX86
static unsigned observerCalls;
static void ownedObserver(Ops* ops,float* current,float* cache,uint32_t force){if(!ops||current!=ops->source||cache!=ops->target||force!=1||observerCalls!=0)std::abort();++observerCalls;}
static void ownedCallback(Ops* ops,float* current,float* cache,uint32_t force){
 if(!ops||current!=ops->source||cache!=ops->target||force!=1)std::abort();
 studioExtendedConstantsCallbackCandidate(force,*ops);
}
#endif
int main(){const unsigned previousMxcsr=_mm_getcsr();_mm_setcsr(0x1f80);
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);std::array<unsigned char,sizeof(vertexConstantRunsBytes)> bytes{};dll.seekg(vertexConstantRunsFileOffset);dll.read(reinterpret_cast<char*>(bytes.data()),bytes.size());if(!dll||std::memcmp(bytes.data(),vertexConstantRunsBytes,bytes.size()))return 1;
 runCode=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!runCode)return 2;std::memcpy(runCode,bytes.data(),bytes.size());for(auto& call:vertexConstantRunsCalls){const uintptr_t target=call[1]==0x100df030?reinterpret_cast<uintptr_t>(&copy):reinterpret_cast<uintptr_t>(&refill);const uint32_t delta=static_cast<uint32_t>(target-reinterpret_cast<uintptr_t>(runCode+call[0]+5));std::memcpy(runCode+call[0]+1,&delta,4);}std::array<unsigned char,sizeof(vertexConstantWrapperBytes)> wrapper{};dll.seekg(vertexConstantWrapperFileOffset);dll.read(reinterpret_cast<char*>(wrapper.data()),wrapper.size());if(!dll||std::memcmp(wrapper.data(),vertexConstantWrapperBytes,wrapper.size()))return 8;std::memcpy(runCode+1024,wrapper.data(),wrapper.size());for(auto& call:vertexConstantWrapperCalls){if(call[1]!=0x1000f1b0)return 9;uint32_t delta=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(runCode)-reinterpret_cast<uintptr_t>(runCode+1024+call[0]+5));std::memcpy(runCode+1024+call[0]+1,&delta,4);}std::array<unsigned char,42> loop{};dll.seekg(dispatchLoopOffset);dll.read(reinterpret_cast<char*>(loop.data()),42);if(!dll||std::memcmp(loop.data(),dispatchLoopBytes,42))return 12;std::memcpy(runCode+2048,loop.data(),42);runCode[2090]=0xc3;loopCode=runCode+2048;DWORD protection;if(!VirtualProtect(runCode,4096,PAGE_EXECUTE_READ,&protection))return 3;FlushInstructionCache(GetCurrentProcess(),runCode,4096);void* table[95]{};table[94]=reinterpret_cast<void*>(&direct);void** object=table;
 std::ofstream oracle("outputs/native-restore-extended-dispatch.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-restore-extended-dispatch.bin",std::ios::binary);
#endif
 unsigned cases=0;for(bool queued:{false,true})for(bool mutation:{false,true})for(unsigned special=0;special<4;++special)for(uint32_t force:{1u})for(unsigned mask:{0u,1u,2u,3u,128u,129u,170u,255u}){
  mutate=mutation;events.clear();std::array<float,6152> source{},cached{};source.fill(17);cached.fill(17);for(unsigned i=0;i<512;++i)if(mask&(1u<<(i/64)))source[(1024+i)*4]=18;auto initialCache=cached;
  uint32_t bits=special==1?0x80000000u:special==2?0x7fc12345u:special==3?0x7f800000u:0;std::memcpy(&source[1024*4+1],&bits,4);if(special>=2)std::memcpy(&cached[1024*4+1],&bits,4);
  liveSource=source.data();std::array<uint32_t,4096> block{};uint32_t* cursor=block.data()+1;uint32_t remaining=4095;
#ifdef _M_IX86
  std::array<uint32_t,531> current{},cache{};current[0x848/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(source.data()));cache[0x848/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(cached.data()));std::array<uint32_t,8> command{};command[0]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&object));command[4]=queued?1:0;command[6]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(cursor));command[7]=remaining;invoke(command.data(),current.data(),cache.data(),force);cursor=reinterpret_cast<uint32_t*>(command[6]);remaining=command[7];
#else
  Ops ops{source.data(),cached.data(),queued,cursor,remaining};NativeManagerCallbackAllocatorCandidate allocator;NativeManagerCallbackRegistryCandidateT<NativeManagerCallbackAllocatorCandidate,decltype(&ownedCallback)> registry{allocator};registry.append(ownedCallback);registry.append(ownedObserver);observerCalls=0;registry.dispatchWith(&ops,source.data(),cached.data(),force);if(registry.size()!=2||observerCalls!=1)std::abort();
#endif
  for(unsigned i=0;i<1024*4;++i)if(cached[i]!=initialCache[i])return 10;for(unsigned i=1536*4;i<cached.size();++i)if(cached[i]!=initialCache[i])return 11;
  std::array<uint32_t,20499> record{};record[0]=remaining;record[1]=static_cast<uint32_t>(cursor-block.data());record[2]=static_cast<uint32_t>(events.size());std::memcpy(record.data()+3,source.data(),sizeof(source));std::memcpy(record.data()+6155,cached.data(),sizeof(cached));std::copy(block.begin(),block.end(),record.begin()+12307);if(events.size()>4096)return 4;std::copy(events.begin(),events.end(),record.begin()+16403);
#ifdef _M_IX86
  oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
  std::array<uint32_t,20499> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(record!=expected){std::printf("mismatch q=%u mutation=%u special=%u force=%u mask=%u\n",queued,mutation,special,force,mask);return 5;}
#endif
  if(!oracle)return 6;++cases;
 }
#ifdef _M_IX86
 VirtualFree(runCode,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 7;
#endif
 _mm_setcsr(previousMxcsr);std::printf("restore_extended_dispatch_cases=%u passed\n",cases);
}

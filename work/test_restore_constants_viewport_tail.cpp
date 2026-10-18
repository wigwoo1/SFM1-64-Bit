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
#include "restore_tail_native_init.h"
#include "viewport_callback_init.h"
#include "studio_shader_viewport_submit_candidate.h"
#include "restore_owner_gate_init.h"
#include "studio_restore_owner_gate_candidate.h"
#include "native_restore_dispatch_snapshot_finish_candidate.h"
#include "studio_extended_constants_callback_candidate.h"
#include "native_manager_callback_registry_candidate.h"
static std::array<uint32_t,8> viewportCommand;static std::array<uint32_t,16> viewportQueue;static unsigned viewportDirectCalls,viewportCookie;
static const StudioShaderViewportWordsCandidate viewportLive{9,7,64,32,0x80000000u,0x3f800000u};
static void viewportSubmit(const unsigned* words){if(std::memcmp(words,viewportLive.data(),24))std::abort();++viewportDirectCalls;}
static unsigned suppressionMode;static bool mutateGate;
static void configureGate(std::array<uint32_t,4096>& owner){owner[0xa8/4]=suppressionMode==1?256u:0u;reinterpret_cast<unsigned char*>(owner.data())[0xac]=suppressionMode==2?2u:0u;owner[0x2cc/4]=suppressionMode==3?256u:0u;}
static std::array<uint32_t,4096> ownerRecord;static std::vector<uint32_t> events;static bool mutate;static float* liveSource;
static void submitDirect(uint32_t start,const float* source,uint32_t count){events.insert(events.end(),{start,count});for(unsigned i=0;i<count*4;++i){uint32_t bits;std::memcpy(&bits,source+i,4);events.push_back(bits);}if(mutate)liveSource[start*4]=42;}
#ifdef _M_IX86
static unsigned char* runCode;static void* observerArguments[3];static unsigned observerCalls,secondCalls;
static void __cdecl observer(void* command,void* current,void* cache,uint32_t force){if(command!=observerArguments[0]||current!=observerArguments[1]||cache!=observerArguments[2]||force!=1||observerCalls!=0)std::abort();++observerCalls;if(mutateGate){auto owner=reinterpret_cast<unsigned char*>(command)-0x648;reinterpret_cast<uint32_t*>(owner)[0xa8/4]=256;}}
static void __cdecl secondObserver(void* command,void* current,void* cache,uint32_t force){if(command!=observerArguments[0]||current!=observerArguments[1]||cache!=observerArguments[2]||force!=1||secondCalls!=0)std::abort();++secondCalls;}
static HRESULT __stdcall viewportDirect(void*,const unsigned* words){viewportSubmit(words);return E_FAIL;}
static void __fastcall viewportCookieCheck(unsigned value,void*){if(value!=viewportCookie)std::abort();}
static void __fastcall refill(void*,void*){std::abort();}
static void* __cdecl copy(void* d,const void* s,uint32_t n){return std::memmove(d,s,n);}
static HRESULT __stdcall direct(void*,uint32_t start,const float* source,uint32_t count){submitDirect(start,source,count);return E_FAIL;}
static void* loopCode;
static unsigned finishes;static std::array<uint32_t,4096>* finishOwner;static std::array<uint32_t,4096>* finishExpected;static void* finishReceiver;
static void __fastcall complete(void* receiver,void*,bool value){if(receiver!=finishReceiver||!value||*finishOwner!=*finishExpected)std::abort();++finishes;}
static __declspec(naked) void invokeLoop(void*,void*){__asm{
 push ebp
 push ebx
 push esi
 push edi
 mov ebx,[esp+20]
 mov ebp,[esp+24]
 cld
 call loopCode
 pop edi
 pop esi
 pop ebx
 pop ebp
 ret
}}
static void invoke(void* command,void* current,void* cache,uint32_t force){
 if(force!=1)std::abort();std::array<uint32_t,4096> owner{};owner.fill(0xabababab);
 std::memcpy(owner.data()+0x648/4,command,32);std::memcpy(owner.data()+0x2010/4,current,531*4);std::memcpy(owner.data()+0x1198/4,cache,531*4);
 observerArguments[0]=owner.data()+0x648/4;observerArguments[1]=owner.data()+0x2010/4;observerArguments[2]=owner.data()+0x1198/4;observerCalls=secondCalls=0;void* secondCallbacks[]{reinterpret_cast<void*>(&secondObserver)};void* callbacks[]{runCode+1024,runCode+3072,reinterpret_cast<void*>(&observer)};owner[0x2e8c/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(callbacks));void* ownerTable[37]{};ownerTable[36]=runCode+1536;owner[0]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ownerTable));owner[0x2e98/4]=3;owner[0x2ea0/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(secondCallbacks));owner[0x2eac/4]=1;reinterpret_cast<unsigned char*>(owner.data())[0x2e88]=0x82;reinterpret_cast<unsigned char*>(owner.data())[0x2e89]=0x81;reinterpret_cast<unsigned char*>(owner.data())[0x30d8]=7;configureGate(owner);auto expected=owner;
 if(!suppressionMode&&owner[0x658/4]){expected[0x660/4]+=2051*4;expected[0x664/4]-=2051;}
 if(!suppressionMode)std::copy(viewportLive.begin(),viewportLive.end(),expected.begin()+0x1198/4);if(!suppressionMode&&mutateGate)expected[0xa8/4]=256;if(!suppressionMode&&!mutateGate){expected[0x2eac/4]=0;reinterpret_cast<unsigned char*>(expected.data())[0x2e89]=0;}if(!suppressionMode){expected[0x2e98/4]=0;reinterpret_cast<unsigned char*>(expected.data())[0x2e88]=0;}std::array<uint32_t,6> staging{};unsigned fields[]{0x2858,0x285c,0x2860,0x2864,0x2868,0x286c};for(unsigned i=0;i<6;++i)staging[i]=expected[fields[i]/4];std::memcpy(expected.data()+0x2010/4,expected.data()+0x1198/4,3704);for(unsigned i=0;i<6;++i)expected[fields[i]/4]=staging[i];reinterpret_cast<unsigned char*>(expected.data())[0x30d8]=0;
 void* finishTable[184]{};finishTable[183]=reinterpret_cast<void*>(&complete);uint32_t receiver=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(finishTable));finishReceiver=&receiver;std::array<uint32_t,32> locals{};auto frame=locals.data()+24;frame[-0x40/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&receiver));finishes=0;finishOwner=&owner;finishExpected=&expected;
 invokeLoop(owner.data(),frame);if(owner!=expected||observerCalls!=(suppressionMode?0u:1u)||secondCalls!=(!suppressionMode&&!mutateGate?1u:0u)||finishes!=1)std::abort();std::memcpy(command,owner.data()+0x648/4,32);
 ownerRecord=owner;ownerRecord[0]=ownerRecord[0x648/4]=ownerRecord[0x2e8c/4]=ownerRecord[0x2ea0/4]=0;ownerRecord[0x2858/4]=1;ownerRecord[0x19e0/4]=2;
}
#endif
struct Ops{
 float* source;float* target;bool queued;uint32_t*& cursor;uint32_t& remaining;
 float* current(){return source;}float* cache(){return target;}
 void submit(uint32_t start,uint32_t count,const float* data){if(!queued){submitDirect(start,data,count);return;}const unsigned words=count*4+3;if(remaining<words)std::abort();remaining-=words;*cursor++=10;*cursor++=start;*cursor++=count;std::memcpy(cursor,data,count*16);cursor+=count*4;}
};
#ifndef _M_IX86
struct Snapshot{
 uint32_t*& cursor;uint32_t* block;uint32_t& remaining;unsigned finishes=0;
 void clearRestoreForce(){reinterpret_cast<unsigned char*>(ownerRecord.data())[0x30d8]=0;}
 std::array<uint32_t,6> staging(){std::array<uint32_t,6> r{};unsigned f[]{0x2858,0x285c,0x2860,0x2864,0x2868,0x286c};for(unsigned i=0;i<6;++i)r[i]=ownerRecord[f[i]/4];return r;}
 void restoreStaging(const std::array<uint32_t,6>& r){unsigned f[]{0x2858,0x285c,0x2860,0x2864,0x2868,0x286c};for(unsigned i=0;i<6;++i)ownerRecord[f[i]/4]=r[i];}
 void copyState(){std::memcpy(ownerRecord.data()+0x2010/4,ownerRecord.data()+0x1198/4,3704);}
 void completeRestore(bool value){if(!value)std::abort();++finishes;ownerRecord[0x660/4]=static_cast<uint32_t>((cursor-block)*4);ownerRecord[0x664/4]=remaining;ownerRecord[0x2e98/4]=suppressionMode?3u:0u;ownerRecord[0x2eac/4]=suppressionMode||mutateGate?1u:0u;}
};
static unsigned observerCalls,secondCalls;
static void ownedObserver(Ops* ops,float* current,float* cache,uint32_t force){if(!ops||current!=ops->source||cache!=ops->target||force!=1||observerCalls!=0)std::abort();++observerCalls;if(mutateGate)ownerRecord[0xa8/4]=256;}
static void ownedSecondObserver(Ops* ops,float* current,float* cache,uint32_t force){if(!ops||current!=ops->source||cache!=ops->target||force!=1||secondCalls!=0)std::abort();++secondCalls;}
struct ViewportOps{
 bool queueEnabled(){return viewportCommand[4]!=0;}
 void queueViewport(const StudioShaderViewportWordsCandidate& words){viewportQueue[0]=20;std::copy(words.begin(),words.end(),viewportQueue.begin()+1);viewportCommand[6]+=28;viewportCommand[7]-=7;}
 void directViewport(const StudioShaderViewportWordsCandidate& words){viewportSubmit(words.data());}
};
static void ownedViewport(Ops*,float*,float*,uint32_t force){
 StudioShaderViewportWordsCandidate live{},cache{};std::copy_n(ownerRecord.data()+0x2010/4,6,live.begin());std::copy_n(ownerRecord.data()+0x1198/4,6,cache.begin());ViewportOps ops;studioShaderViewportSubmitCandidate(live,cache,force,ops);std::copy(live.begin(),live.end(),ownerRecord.begin()+0x2010/4);std::copy(cache.begin(),cache.end(),ownerRecord.begin()+0x1198/4);
}
static void ownedCallback(Ops* ops,float* current,float* cache,uint32_t force){
 if(!ops||current!=ops->source||cache!=ops->target||force!=1)std::abort();
 studioExtendedConstantsCallbackCandidate(force,*ops);
}
#endif
int main(){const unsigned previousMxcsr=_mm_getcsr();_mm_setcsr(0x1f80);
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);std::array<unsigned char,sizeof(vertexConstantRunsBytes)> bytes{};dll.seekg(vertexConstantRunsFileOffset);dll.read(reinterpret_cast<char*>(bytes.data()),bytes.size());if(!dll||std::memcmp(bytes.data(),vertexConstantRunsBytes,bytes.size()))return 1;
 runCode=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!runCode)return 2;std::memcpy(runCode,bytes.data(),bytes.size());for(auto& call:vertexConstantRunsCalls){const uintptr_t target=call[1]==0x100df030?reinterpret_cast<uintptr_t>(&copy):reinterpret_cast<uintptr_t>(&refill);const uint32_t delta=static_cast<uint32_t>(target-reinterpret_cast<uintptr_t>(runCode+call[0]+5));std::memcpy(runCode+call[0]+1,&delta,4);}std::array<unsigned char,sizeof(vertexConstantWrapperBytes)> wrapper{};dll.seekg(vertexConstantWrapperFileOffset);dll.read(reinterpret_cast<char*>(wrapper.data()),wrapper.size());if(!dll||std::memcmp(wrapper.data(),vertexConstantWrapperBytes,wrapper.size()))return 8;std::memcpy(runCode+1024,wrapper.data(),wrapper.size());for(auto& call:vertexConstantWrapperCalls){if(call[1]!=0x1000f1b0)return 9;uint32_t delta=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(runCode)-reinterpret_cast<uintptr_t>(runCode+1024+call[0]+5));std::memcpy(runCode+1024+call[0]+1,&delta,4);}std::array<unsigned char,333> loop{},verified{};dll.seekg(restoreTailFileOffset);dll.read(reinterpret_cast<char*>(loop.data()),333);std::ifstream tail("outputs/restore-tail-verified-bytes.bin",std::ios::binary);tail.read(reinterpret_cast<char*>(verified.data()),333);if(!dll||!tail||loop!=verified||tail.peek()!=std::char_traits<char>::eof())return 12;std::memcpy(runCode+2048,loop.data(),333);runCode[2381]=0xc3;loopCode=runCode+2048;unsigned char gateBytes[36]{};dll.seekg(ownerGateOffset);dll.read(reinterpret_cast<char*>(gateBytes),36);if(!dll||std::memcmp(gateBytes,ownerGateBytes,36))return 13;std::memcpy(runCode+1536,gateBytes,36);unsigned char viewportRaw[249]{};dll.seekg(viewportOffset);dll.read(reinterpret_cast<char*>(viewportRaw),249);if(!dll||std::memcmp(viewportRaw,viewportBytes,249))return 15;std::memcpy(runCode+3072,viewportRaw,249);for(auto fix:viewportFixups){auto address=fix[1]==0x1017c370?&viewportCookie:viewportCommand.data()+(fix[1]-0x10180aa8)/4;unsigned value=static_cast<unsigned>(reinterpret_cast<uintptr_t>(address));std::memcpy(runCode+3072+fix[0],&value,4);}for(auto call:viewportCalls){auto target=call[1]==0x100df5ad?reinterpret_cast<uintptr_t>(&viewportCookieCheck):reinterpret_cast<uintptr_t>(&refill);unsigned delta=static_cast<unsigned>(target-reinterpret_cast<uintptr_t>(runCode+3072+call[0]+5));std::memcpy(runCode+3072+call[0]+1,&delta,4);}DWORD protection;if(!VirtualProtect(runCode,4096,PAGE_EXECUTE_READ,&protection))return 3;FlushInstructionCache(GetCurrentProcess(),runCode,4096);void* table[95]{};table[94]=reinterpret_cast<void*>(&direct);table[47]=reinterpret_cast<void*>(&viewportDirect);void** object=table;
 std::ofstream oracle("outputs/native-restore-constants-viewport-tail.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-restore-constants-viewport-tail.bin",std::ios::binary);
#endif
 unsigned cases=0;for(bool queued:{false,true})for(bool mutation:{false,true})for(unsigned special=0;special<4;++special)for(uint32_t force:{1u})for(unsigned mask:{0u,1u,2u,3u,128u,129u,170u,255u})for(suppressionMode=0;suppressionMode<4;++suppressionMode)for(bool gateMutation:{false,true}){mutateGate=gateMutation;
  mutate=mutation;events.clear();std::array<float,6152> source{},cached{};source.fill(17);cached.fill(17);for(unsigned i=0;i<512;++i)if(mask&(1u<<(i/64)))source[(1024+i)*4]=18;auto initialCache=cached;
  uint32_t bits=special==1?0x80000000u:special==2?0x7fc12345u:special==3?0x7f800000u:0;std::memcpy(&source[1024*4+1],&bits,4);if(special>=2)std::memcpy(&cached[1024*4+1],&bits,4);
  viewportCommand.fill(0xabababab);viewportCommand[0]=0;viewportCommand[4]=queued?1u:0u;viewportCommand[6]=0;viewportCommand[7]=16;viewportQueue.fill(0xcccccccc);viewportDirectCalls=0;auto expectedViewportQueue=viewportQueue;auto sourceBefore=source;auto cacheBefore=cached;liveSource=source.data();std::array<uint32_t,4096> block{};uint32_t* cursor=block.data()+1;uint32_t remaining=4095;
#ifdef _M_IX86
  std::array<uint32_t,531> current{},cache{};std::copy(viewportLive.begin(),viewportLive.end(),current.begin());current[0x848/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(source.data()));cache[0x848/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(cached.data()));std::array<uint32_t,8> command{};command[0]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&object));command[4]=queued?1:0;command[6]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(cursor));command[7]=remaining;viewportCommand[0]=static_cast<unsigned>(reinterpret_cast<uintptr_t>(&object));viewportCommand[6]=static_cast<unsigned>(reinterpret_cast<uintptr_t>(viewportQueue.data()));invoke(command.data(),current.data(),cache.data(),force);viewportCommand[0]=0;viewportCommand[6]-=static_cast<unsigned>(reinterpret_cast<uintptr_t>(viewportQueue.data()));cursor=reinterpret_cast<uint32_t*>(command[6]);remaining=command[7];ownerRecord[0x660/4]=static_cast<uint32_t>((cursor-block.data())*4);
#else
  Ops ops{source.data(),cached.data(),queued,cursor,remaining};NativeManagerCallbackAllocatorCandidate allocator;NativeManagerCallbackRegistryCandidateT<NativeManagerCallbackAllocatorCandidate,decltype(&ownedCallback)> registry{allocator};registry.append(ownedCallback);registry.append(ownedViewport);registry.append(ownedObserver);observerCalls=secondCalls=0;ownerRecord.fill(0xabababab);std::memset(ownerRecord.data()+0x648/4,0,32);std::memset(ownerRecord.data()+0x2010/4,0,531*4);std::memset(ownerRecord.data()+0x1198/4,0,531*4);ownerRecord[0]=ownerRecord[0x2e8c/4]=ownerRecord[0x2ea0/4]=ownerRecord[0x2eac/4]=0;std::copy(viewportLive.begin(),viewportLive.end(),ownerRecord.begin()+0x2010/4);ownerRecord[0x2858/4]=1;ownerRecord[0x19e0/4]=2;ownerRecord[0x658/4]=queued?1:0;ownerRecord[0x660/4]=4;ownerRecord[0x664/4]=4095;ownerRecord[0x2e98/4]=3;ownerRecord[0x2eac/4]=1;auto bytes=reinterpret_cast<unsigned char*>(ownerRecord.data());bytes[0x2e88]=0x82;bytes[0x2e89]=0x81;bytes[0x30d8]=7;configureGate(ownerRecord);NativeManagerCallbackRegistryCandidateT<NativeManagerCallbackAllocatorCandidate,decltype(&ownedCallback)> second{allocator};second.append(ownedSecondObserver);Snapshot snapshot{cursor,block.data(),remaining};unsigned gates=0;auto gate=[&]{++gates;return studioRestoreOwnerGateCandidate(ownerRecord[0xa8/4],bytes[0xac],ownerRecord[0x2cc/4])!=0;};nativeRestoreDispatchSnapshotFinishCandidate(registry,bytes[0x2e88],second,bytes[0x2e89],gate,snapshot,&ops,source.data(),cached.data(),force);if(registry.size()!=(suppressionMode?3:0)||observerCalls!=(suppressionMode?0u:1u)||second.size()!=(suppressionMode||mutateGate?1:0)||secondCalls!=(!suppressionMode&&!mutateGate?1u:0u)||snapshot.finishes!=1||gates!=2)std::abort();
#endif
  if(!suppressionMode&&queued){expectedViewportQueue[0]=20;std::copy(viewportLive.begin(),viewportLive.end(),expectedViewportQueue.begin()+1);}if(viewportQueue!=expectedViewportQueue||viewportCommand[6]!=(!suppressionMode&&queued?28u:0u)||viewportCommand[7]!=(!suppressionMode&&queued?9u:16u)||viewportDirectCalls!=(!suppressionMode&&!queued?1u:0u))return 16;
  if(suppressionMode&&(std::memcmp(source.data(),sourceBefore.data(),sizeof(source))||std::memcmp(cached.data(),cacheBefore.data(),sizeof(cached))||!events.empty()||remaining!=4095||cursor!=block.data()+1))return 14;
  for(unsigned i=0;i<1024*4;++i)if(cached[i]!=initialCache[i])return 10;for(unsigned i=1536*4;i<cached.size();++i)if(cached[i]!=initialCache[i])return 11;
  std::array<uint32_t,24620> record{};record[0]=remaining;record[1]=static_cast<uint32_t>(cursor-block.data());record[2]=static_cast<uint32_t>(events.size());std::memcpy(record.data()+3,source.data(),sizeof(source));std::memcpy(record.data()+6155,cached.data(),sizeof(cached));std::copy(block.begin(),block.end(),record.begin()+12307);if(events.size()>4096)return 4;std::copy(events.begin(),events.end(),record.begin()+16403);std::copy(ownerRecord.begin(),ownerRecord.end(),record.begin()+20499);std::copy(viewportCommand.begin(),viewportCommand.end(),record.begin()+24595);std::copy(viewportQueue.begin(),viewportQueue.end(),record.begin()+24603);record[24619]=viewportDirectCalls;
#ifdef _M_IX86
  oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
  std::array<uint32_t,24620> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(record!=expected){std::printf("mismatch q=%u mutation=%u special=%u force=%u mask=%u\n",queued,mutation,special,force,mask);return 5;}
#endif
  if(!oracle)return 6;++cases;
 }
#ifdef _M_IX86
 VirtualFree(runCode,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 7;
#endif
 _mm_setcsr(previousMxcsr);std::printf("restore_constants_viewport_tail_cases=%u passed\n",cases);
}

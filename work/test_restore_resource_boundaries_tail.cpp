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
#include "studio_device_queue_refill_candidate.h"
#include "studio_device_queue_publish_candidate.h"
#include "studio_device_queue_acquire_candidate.h"
#include "restore_refill_body_init.h"
#include "restore_publish_init.h"
#include "restore_acquire_init.h"
#include "restore_shared_queue_refill_init.h"
#include "restore_tail_native_init.h"
#include "viewport_callback_init.h"
#include "vertex_texture_callback_native_init.h"
#include "studio_vertex_texture_submit_candidate.h"
#include "studio_shader_viewport_submit_candidate.h"
#include "restore_owner_gate_init.h"
#include "studio_restore_owner_gate_candidate.h"
#include "native_restore_dispatch_snapshot_finish_candidate.h"
#include "studio_extended_constants_callback_candidate.h"
#include "native_manager_callback_registry_candidate.h"
// Reserved zero record fields retained for the oracle format; viewport uses the owner queue below.
static std::array<uint32_t,8> viewportCommand;static std::array<uint32_t,16> viewportQueue;static unsigned viewportDirectCalls,viewportCookie;
static const StudioShaderViewportWordsCandidate viewportLive{9,7,64,32,0x80000000u,0x3f800000u};
static void viewportSubmit(const unsigned* words){if(std::memcmp(words,viewportLive.data(),24))std::abort();++viewportDirectCalls;}
static bool liveTexture,arrayTexture;static std::array<uint32_t,2> nativeTextureResources{0x1234,0x5678};static uintptr_t expectedTextureResource(){return liveTexture?(arrayTexture?0x5678u:0x1234u):0u;}static unsigned textureSamplerCalls;static std::array<uint32_t,20> textureRecord;static unsigned textureDirectCalls,textureCountCalls;static int32_t textureStageCount;static unsigned positiveTextureCount(){return textureStageCount>0?static_cast<unsigned>(textureStageCount):0u;}


static unsigned suppressionMode;static bool mutateGate;
static void configureGate(std::array<uint32_t,4096>& owner){owner[0xa8/4]=suppressionMode==1?256u:0u;reinterpret_cast<unsigned char*>(owner.data())[0xac]=suppressionMode==2?2u:0u;owner[0x2cc/4]=suppressionMode==3?256u:0u;}
static unsigned publicationSeed,publicationIndex,pressureWaits;static bool publicationFull,pressurePoolFull;
static unsigned initialCapacity,plannedOffset,plannedRemaining,plannedRefills;
static uint32_t* initialBlock;static uint32_t* plannedBlock;
static std::array<uint32_t,4096> replacementBlock,expectedOldBlock,expectedNewBlock;
static void planPackets(const std::array<float,6152>& source){
 expectedOldBlock.fill(0);expectedOldBlock[0]=1;expectedOldBlock[1]=0xcafef00d;expectedNewBlock.fill(0xdddddddd);expectedNewBlock[0]=pressurePoolFull?2u:0u;plannedOffset=2;plannedRemaining=initialCapacity;plannedRefills=0;plannedBlock=initialBlock;
 auto emit=[&](const std::vector<uint32_t>& packet){if(plannedRemaining<packet.size()){if(plannedRefills++)std::abort();expectedOldBlock[0]=2;expectedOldBlock[plannedOffset]=0;plannedBlock=replacementBlock.data();plannedOffset=1;plannedRemaining=4095;expectedNewBlock[0]=1;}auto& block=plannedRefills?expectedNewBlock:expectedOldBlock;std::copy(packet.begin(),packet.end(),block.begin()+plannedOffset);plannedOffset+=static_cast<unsigned>(packet.size());plannedRemaining-=static_cast<unsigned>(packet.size());};
 if(suppressionMode)return;
 std::vector<uint32_t> constants{10,1024,512};for(unsigned i=4096;i<6144;++i){uint32_t bits;std::memcpy(&bits,source.data()+i,4);constants.push_back(bits);}emit(constants);
 std::vector<uint32_t> viewport{20};viewport.insert(viewport.end(),viewportLive.begin(),viewportLive.end());emit(viewport);
 if(mutateGate)return;emit({2,257,static_cast<unsigned>(expectedTextureResource())});if(liveTexture){unsigned types[]{1,2,6,5,7};unsigned values[]{11,12,15,14,16};for(unsigned i=0;i<5;++i)emit({17,257,types[i],values[i]});}
}
static std::array<uint32_t,4096> ownerRecord;static std::vector<uint32_t> events;static bool mutate;static float* liveSource;
static void samplerSubmit(unsigned stage,unsigned type,unsigned value){unsigned types[]{1,2,6,5,7};unsigned values[]{11,12,15,14,16};if(stage!=257||textureSamplerCalls>=5||type!=types[textureSamplerCalls]||value!=values[textureSamplerCalls])std::abort();++textureSamplerCalls;events.insert(events.end(),{stage,type,value});}
static void textureSubmit(unsigned stage,uintptr_t resource){if(stage!=257+textureDirectCalls||resource!=expectedTextureResource())std::abort();++textureDirectCalls;events.insert(events.end(),{stage,0,static_cast<unsigned>(resource)});}
static void submitDirect(uint32_t start,const float* source,uint32_t count){events.insert(events.end(),{start,count});for(unsigned i=0;i<count*4;++i){uint32_t bits;std::memcpy(&bits,source+i,4);events.push_back(bits);}if(mutate)liveSource[start*4]=42;}
#ifdef _M_IX86
static std::array<uint32_t,4096> nativeOwner;static unsigned char* runCode;static void* observerArguments[3];static unsigned observerCalls,secondCalls;
static void __cdecl observer(void* command,void* current,void* cache,uint32_t force){if(command!=observerArguments[0]||current!=observerArguments[1]||cache!=observerArguments[2]||force!=1||observerCalls!=0)std::abort();++observerCalls;if(mutateGate){auto owner=reinterpret_cast<unsigned char*>(command)-0x648;reinterpret_cast<uint32_t*>(owner)[0xa8/4]=256;}}
static void __cdecl secondObserver(void* command,void* current,void* cache,uint32_t force){if(command!=observerArguments[0]||current!=observerArguments[1]||cache!=observerArguments[2]||force!=1||secondCalls!=0)std::abort();++secondCalls;}
static HRESULT __stdcall viewportDirect(void*,const unsigned* words){viewportSubmit(words);return E_FAIL;}
static void __fastcall viewportCookieCheck(unsigned value,void*){if(value!=viewportCookie)std::abort();}
static int32_t __fastcall textureCount(void*,void*){++textureCountCalls;return textureStageCount;}
static HRESULT __stdcall textureDirect(void*,unsigned stage,void* resource){textureSubmit(stage,reinterpret_cast<uintptr_t>(resource));return E_FAIL;}
static HRESULT __stdcall textureSampler(void*,unsigned stage,unsigned type,unsigned value){samplerSubmit(stage,type,value);return E_FAIL;}
static std::array<unsigned,640> sharedRing;
static std::array<unsigned*,500> sharedPool;
static std::array<std::array<unsigned,4>,500> sharedOccupied;
static unsigned sharedWaits;
static void __stdcall sharedWait(unsigned value){if(value||++sharedWaits!=1)std::abort();sharedRing[0x9d8/4]=sharedRing[0x8d0/4]-499u;if(pressurePoolFull){for(auto entry:sharedPool)if(entry[0]==0)std::abort();if(replacementBlock[0]!=2)std::abort();replacementBlock[0]=0;}}
static void installSharedRefill(std::ifstream& dll,unsigned char* code,unsigned* bindingValues){
 auto install=[&](unsigned offset,const unsigned char* expected,unsigned count,unsigned destination){std::array<unsigned char,105> raw{};dll.seekg(offset);dll.read(reinterpret_cast<char*>(raw.data()),count);if(!dll||std::memcmp(raw.data(),expected,count))std::abort();std::memcpy(code+destination,raw.data(),count);};
 install(refillOffset,refillBytes,76,sharedRefillOffset);install(publishOffset,publishBytes,105,sharedPublishOffset);install(acquireOffset,acquireBytes,77,sharedAcquireOffset);
 for(auto link:sharedRefillLinks){unsigned delta=link[1]-(link[0]+4);std::memcpy(code+link[0],&delta,4);}
 for(auto binding:sharedRefillBindings)std::memcpy(code+binding[0],bindingValues+binding[1],4);
}
static void checkSharedRefillImage(unsigned char* code){
 auto callback=reinterpret_cast<void(__fastcall*)(void*,void*)>(code+sharedRefillOffset);
 for(unsigned index:{0u,499u})for(bool full:{false,true}){
  std::array<unsigned,16> previous{},replacement{};previous.fill(0xcccccccc);replacement.fill(0xdddddddd);previous[0]=1;replacement[0]=0;auto expectedPrevious=previous,expectedReplacement=replacement;expectedPrevious[0]=2;expectedPrevious[2]=0;expectedReplacement[0]=1;
  for(unsigned i=0;i<500;++i){sharedOccupied[i].fill(0xaabb0000u+i);sharedOccupied[i][0]=2;sharedPool[i]=sharedOccupied[i].data();}sharedPool[index]=replacement.data();auto expectedPool=sharedPool;auto expectedOccupied=sharedOccupied;
  std::array<unsigned,8> state{};state.fill(0xabababab);state[5]=static_cast<unsigned>(reinterpret_cast<uintptr_t>(previous.data()));state[6]=static_cast<unsigned>(reinterpret_cast<uintptr_t>(previous.data()+2));state[7]=0;auto expectedState=state;expectedState[5]=static_cast<unsigned>(reinterpret_cast<uintptr_t>(replacement.data()));expectedState[6]=static_cast<unsigned>(reinterpret_cast<uintptr_t>(replacement.data()+1));expectedState[7]=4095;
  sharedRing.fill(0xeeeeeeee);sharedRing[0x8d0/4]=0xffffffffu;sharedRing[0x8d4/4]=index;sharedRing[0x9d8/4]=0xffffffffu-(full?500u:0u);auto expectedRing=sharedRing;expectedRing[index]=static_cast<unsigned>(reinterpret_cast<uintptr_t>(previous.data()));expectedRing[0x8d0/4]=0;expectedRing[0x8d4/4]=(index+1)%500;expectedRing[0x9d8/4]=0xffffffffu-(full?499u:0u);sharedWaits=0;
  callback(state.data(),nullptr);if(state!=expectedState||previous!=expectedPrevious||replacement!=expectedReplacement||sharedRing!=expectedRing||sharedWaits!=unsigned(full)||sharedPool!=expectedPool||sharedOccupied!=expectedOccupied)std::abort();
 }
 std::printf("shared_restore_image_refill_cases=4 passed\n");
}
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
 if(force!=1)std::abort();auto& owner=nativeOwner;owner.fill(0xabababab);
 std::memcpy(owner.data()+0x648/4,command,32);std::memcpy(owner.data()+0x2010/4,current,531*4);std::memcpy(owner.data()+0x1198/4,cache,531*4);
 observerArguments[0]=owner.data()+0x648/4;observerArguments[1]=owner.data()+0x2010/4;observerArguments[2]=owner.data()+0x1198/4;observerCalls=secondCalls=0;void* secondCallbacks[]{runCode+4096,reinterpret_cast<void*>(&secondObserver)};void* callbacks[]{runCode+1024,runCode+3072,reinterpret_cast<void*>(&observer)};owner[0x2e8c/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(callbacks));void* ownerTable[37]{};ownerTable[36]=runCode+1536;owner[0]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ownerTable));owner[0x2e98/4]=3;owner[0x2ea0/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(secondCallbacks));owner[0x2eac/4]=2;reinterpret_cast<unsigned char*>(owner.data())[0x2e88]=0x82;reinterpret_cast<unsigned char*>(owner.data())[0x2e89]=0x81;reinterpret_cast<unsigned char*>(owner.data())[0x30d8]=7;configureGate(owner);for(unsigned i=0;i<3;++i){owner[0x2ab0/4+i*6]=liveTexture&&i==0?static_cast<unsigned>(reinterpret_cast<uintptr_t>(textureRecord.data())):0;owner[0x1c38/4+i*6]=1;}auto expected=owner;
 expected[0x65c/4]=static_cast<unsigned>(reinterpret_cast<uintptr_t>(plannedBlock));expected[0x660/4]=static_cast<unsigned>(reinterpret_cast<uintptr_t>(plannedBlock+plannedOffset));expected[0x664/4]=plannedRemaining;
 if(!suppressionMode)std::copy(viewportLive.begin(),viewportLive.end(),expected.begin()+0x1198/4);if(!suppressionMode&&mutateGate)expected[0xa8/4]=256;if(!suppressionMode&&!mutateGate){for(unsigned i=0;i<positiveTextureCount();++i){auto offset=0x1c38/4+i*6;expected[offset]=liveTexture?static_cast<unsigned>(reinterpret_cast<uintptr_t>(textureRecord.data())):0;if(liveTexture){unsigned words[]{11,12,14,15,16};std::copy(words,words+5,expected.begin()+offset+1);}}expected[0x2eac/4]=0;reinterpret_cast<unsigned char*>(expected.data())[0x2e89]=0;}if(!suppressionMode){expected[0x2e98/4]=0;reinterpret_cast<unsigned char*>(expected.data())[0x2e88]=0;}std::array<uint32_t,6> staging{};unsigned fields[]{0x2858,0x285c,0x2860,0x2864,0x2868,0x286c};for(unsigned i=0;i<6;++i)staging[i]=expected[fields[i]/4];std::memcpy(expected.data()+0x2010/4,expected.data()+0x1198/4,3704);for(unsigned i=0;i<6;++i)expected[fields[i]/4]=staging[i];reinterpret_cast<unsigned char*>(expected.data())[0x30d8]=0;
 void* finishTable[184]{};finishTable[183]=reinterpret_cast<void*>(&complete);uint32_t receiver=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(finishTable));finishReceiver=&receiver;std::array<uint32_t,32> locals{};auto frame=locals.data()+24;frame[-0x40/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&receiver));finishes=0;finishOwner=&owner;finishExpected=&expected;
 invokeLoop(owner.data(),frame);if(owner!=expected||observerCalls!=(suppressionMode?0u:1u)||secondCalls!=(!suppressionMode&&!mutateGate?1u:0u)||finishes!=1)std::abort();std::memcpy(command,owner.data()+0x648/4,32);
 ownerRecord=owner;ownerRecord[0x65c/4]=plannedRefills?2u:1u;ownerRecord[0]=ownerRecord[0x648/4]=ownerRecord[0x2e8c/4]=ownerRecord[0x2ea0/4]=0;for(unsigned offset:{0x1c38u,0x2ab0u}){auto value=ownerRecord[offset/4];auto pointer=static_cast<unsigned>(reinterpret_cast<uintptr_t>(textureRecord.data()));if(value!=0&&value!=1&&value!=pointer)std::abort();if(value==pointer)ownerRecord[offset/4]=2;}ownerRecord[0x2858/4]=1;ownerRecord[0x19e0/4]=2;
}
#endif
struct Ops{
 float* source;float* target;bool queued;uint32_t*& cursor;uint32_t& remaining;uint32_t*& activeBlock;
#ifndef _M_IX86
 StudioDeviceQueuePublishCandidate publication;std::array<uint32_t*,500> pool{};std::array<std::array<uint32_t,4>,500> occupied{};
 unsigned waits=0;void waitZero(){if(++waits!=1)std::abort();publication.completed=publication.produced-499u;if(pressurePoolFull){for(auto entry:pool)if(entry[0]==0)std::abort();if(replacementBlock[0]!=2)std::abort();replacementBlock[0]=0;}}
 void publish(uint32_t* block){if(block!=initialBlock)std::abort();studioDeviceQueuePublishCandidate(publication,reinterpret_cast<uintptr_t>(block),*this);}
 void refill(){StudioDeviceQueueStateCandidate state{activeBlock,cursor,remaining};studioDeviceQueueRefillCandidate(state,*this);activeBlock=state.block;cursor=state.cursor;remaining=state.remaining;}
 uint32_t* acquire(unsigned header){return studioDeviceQueueAcquireCandidate(pool,header,*this);}
#endif
 void ensure(unsigned words){
#ifndef _M_IX86
 if(remaining<words)refill();
#endif
 if(remaining<words)std::abort();}
 float* current(){return source;}float* cache(){return target;}
 void submit(uint32_t start,uint32_t count,const float* data){if(!queued){submitDirect(start,data,count);return;}const unsigned words=count*4+3;ensure(words);remaining-=words;*cursor++=10;*cursor++=start;*cursor++=count;std::memcpy(cursor,data,count*16);cursor+=count*4;}
};
#ifndef _M_IX86
struct Snapshot{
 uint32_t*& cursor;uint32_t*& block;uint32_t& remaining;unsigned finishes=0;
 void clearRestoreForce(){reinterpret_cast<unsigned char*>(ownerRecord.data())[0x30d8]=0;}
 std::array<uint32_t,6> staging(){std::array<uint32_t,6> r{};unsigned f[]{0x2858,0x285c,0x2860,0x2864,0x2868,0x286c};for(unsigned i=0;i<6;++i)r[i]=ownerRecord[f[i]/4];return r;}
 void restoreStaging(const std::array<uint32_t,6>& r){unsigned f[]{0x2858,0x285c,0x2860,0x2864,0x2868,0x286c};for(unsigned i=0;i<6;++i)ownerRecord[f[i]/4]=r[i];}
 void copyState(){std::memcpy(ownerRecord.data()+0x2010/4,ownerRecord.data()+0x1198/4,3704);}
 void completeRestore(bool value){if(!value)std::abort();++finishes;ownerRecord[0x65c/4]=block==initialBlock?1u:2u;ownerRecord[0x660/4]=static_cast<uint32_t>((cursor-block)*4);ownerRecord[0x664/4]=remaining;ownerRecord[0x2e98/4]=suppressionMode?3u:0u;ownerRecord[0x2eac/4]=suppressionMode||mutateGate?2u:0u;}
};
static unsigned observerCalls,secondCalls;
static void ownedObserver(Ops* ops,float* current,float* cache,uint32_t force){if(!ops||current!=ops->source||cache!=ops->target||force!=1||observerCalls!=0)std::abort();++observerCalls;if(mutateGate)ownerRecord[0xa8/4]=256;}
static void ownedSecondObserver(Ops* ops,float* current,float* cache,uint32_t force){if(!ops||current!=ops->source||cache!=ops->target||force!=1||secondCalls!=0)std::abort();++secondCalls;}
struct ViewportOps{
 Ops& command;bool queueEnabled(){return command.queued;}
 void queueViewport(const StudioShaderViewportWordsCandidate& words){command.ensure(7);command.remaining-=7;*command.cursor++=20;std::copy(words.begin(),words.end(),command.cursor);command.cursor+=6;}
 void directViewport(const StudioShaderViewportWordsCandidate& words){viewportSubmit(words.data());}
};
static void ownedViewport(Ops* command,float*,float*,uint32_t force){
 StudioShaderViewportWordsCandidate live{},cache{};std::copy_n(ownerRecord.data()+0x2010/4,6,live.begin());std::copy_n(ownerRecord.data()+0x1198/4,6,cache.begin());ViewportOps ops{*command};studioShaderViewportSubmitCandidate(live,cache,force,ops);std::copy(live.begin(),live.end(),ownerRecord.begin()+0x2010/4);std::copy(cache.begin(),cache.end(),ownerRecord.begin()+0x1198/4);
}
struct TextureOps{
 Ops& ops;bool queueEnabled(){return ops.queued;}
 void queueTexture(unsigned stage,uintptr_t resource){if(stage<257||stage>=257+positiveTextureCount()||resource!=expectedTextureResource())std::abort();ops.ensure(3);ops.remaining-=3;*ops.cursor++=2;*ops.cursor++=stage;*ops.cursor++=static_cast<unsigned>(resource);}
 void directTexture(unsigned stage,uintptr_t resource){textureSubmit(stage,resource);}
 void queueSampler(unsigned stage,unsigned type,unsigned value){ops.ensure(4);ops.remaining-=4;*ops.cursor++=17;*ops.cursor++=stage;*ops.cursor++=type;*ops.cursor++=value;}void directSampler(unsigned stage,unsigned type,unsigned value){samplerSubmit(stage,type,value);}
};
static void ownedTexture(Ops* ops,float*,float*,uint32_t force){
 ++textureCountCalls;StudioVertexTextureRecordCandidate live;live.sampler={11,12,13,14,15,16};std::array<uintptr_t,2> resources{0x1234,0x5678};const auto expectedResources=resources;live.resourceCount=arrayTexture?2:1;live.activeResource=arrayTexture?1:0;live.resource=arrayTexture?reinterpret_cast<uintptr_t>(resources.data()):0x1234;if(arrayTexture&&live.resource<=UINT32_MAX)std::abort();const auto expectedLive=live;std::array<StudioVertexTextureRecordCandidate*,3> current{};if(liveTexture)current[0]=&live;std::array<StudioVertexTextureCacheCandidate,3> caches{};for(auto& cache:caches){cache.record=reinterpret_cast<StudioVertexTextureRecordCandidate*>(uintptr_t{1});cache.sampler.fill(0xabababab);}TextureOps services{*ops};studioVertexTextureSubmitCandidate(current.data(),caches.data(),textureStageCount,force,services);if(resources!=expectedResources||live.resource!=expectedLive.resource||live.resourceCount!=expectedLive.resourceCount||live.activeResource!=expectedLive.activeResource||live.sampler!=expectedLive.sampler)std::abort();for(unsigned i=0;i<3;++i){auto expected=i<positiveTextureCount()?current[i]:reinterpret_cast<StudioVertexTextureRecordCandidate*>(uintptr_t{1});if(caches[i].record!=expected)std::abort();ownerRecord[0x1c38/4+i*6]=caches[i].record==&live?2u:caches[i].record?1u:0u;if(liveTexture&&i==0){auto offset=0x1c38/4;unsigned fields[]{0,1,3,4,5};for(unsigned j=0;j<5;++j)ownerRecord[offset+j+1]=caches[0].sampler[fields[j]];}}
}
static void ownedCallback(Ops* ops,float* current,float* cache,uint32_t force){
 if(!ops||current!=ops->source||cache!=ops->target||force!=1)std::abort();
 studioExtendedConstantsCallbackCandidate(force,*ops);
}
#endif
int main(){const unsigned previousMxcsr=_mm_getcsr();_mm_setcsr(0x1f80);
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);std::array<unsigned char,sizeof(vertexConstantRunsBytes)> bytes{};dll.seekg(vertexConstantRunsFileOffset);dll.read(reinterpret_cast<char*>(bytes.data()),bytes.size());if(!dll||std::memcmp(bytes.data(),vertexConstantRunsBytes,bytes.size()))return 1;
 runCode=static_cast<unsigned char*>(VirtualAlloc(nullptr,8192,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!runCode)return 2;std::memcpy(runCode,bytes.data(),bytes.size());for(auto& call:vertexConstantRunsCalls){const uintptr_t target=call[1]==0x100df030?reinterpret_cast<uintptr_t>(&copy):reinterpret_cast<uintptr_t>(runCode+sharedRefillOffset);const uint32_t delta=static_cast<uint32_t>(target-reinterpret_cast<uintptr_t>(runCode+call[0]+5));std::memcpy(runCode+call[0]+1,&delta,4);}std::array<unsigned char,sizeof(vertexConstantWrapperBytes)> wrapper{};dll.seekg(vertexConstantWrapperFileOffset);dll.read(reinterpret_cast<char*>(wrapper.data()),wrapper.size());if(!dll||std::memcmp(wrapper.data(),vertexConstantWrapperBytes,wrapper.size()))return 8;std::memcpy(runCode+1024,wrapper.data(),wrapper.size());for(auto& call:vertexConstantWrapperCalls){if(call[1]!=0x1000f1b0)return 9;uint32_t delta=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(runCode)-reinterpret_cast<uintptr_t>(runCode+1024+call[0]+5));std::memcpy(runCode+1024+call[0]+1,&delta,4);}std::array<unsigned char,333> loop{},verified{};dll.seekg(restoreTailFileOffset);dll.read(reinterpret_cast<char*>(loop.data()),333);std::ifstream tail("outputs/restore-tail-verified-bytes.bin",std::ios::binary);tail.read(reinterpret_cast<char*>(verified.data()),333);if(!dll||!tail||loop!=verified||tail.peek()!=std::char_traits<char>::eof())return 12;std::memcpy(runCode+2048,loop.data(),333);runCode[2381]=0xc3;loopCode=runCode+2048;unsigned char gateBytes[36]{};dll.seekg(ownerGateOffset);dll.read(reinterpret_cast<char*>(gateBytes),36);if(!dll||std::memcmp(gateBytes,ownerGateBytes,36))return 13;std::memcpy(runCode+1536,gateBytes,36);unsigned char viewportRaw[249]{};dll.seekg(viewportOffset);dll.read(reinterpret_cast<char*>(viewportRaw),249);if(!dll||std::memcmp(viewportRaw,viewportBytes,249))return 15;std::memcpy(runCode+3072,viewportRaw,249);for(auto fix:viewportFixups){auto address=fix[1]==0x1017c370?&viewportCookie:nativeOwner.data()+0x648/4+(fix[1]-0x10180aa8)/4;unsigned value=static_cast<unsigned>(reinterpret_cast<uintptr_t>(address));std::memcpy(runCode+3072+fix[0],&value,4);}for(auto call:viewportCalls){auto target=call[1]==0x100df5ad?reinterpret_cast<uintptr_t>(&viewportCookieCheck):reinterpret_cast<uintptr_t>(runCode+sharedRefillOffset);unsigned delta=static_cast<unsigned>(target-reinterpret_cast<uintptr_t>(runCode+3072+call[0]+5));std::memcpy(runCode+3072+call[0]+1,&delta,4);}unsigned char textureRaw[751]{};dll.seekg(vertexTextureCallbackFileOffset);dll.read(reinterpret_cast<char*>(textureRaw),751);if(!dll||std::memcmp(textureRaw,vertexTextureCallbackBytes,751))return 17;std::memcpy(runCode+4096,textureRaw,751);void* serviceTable[32]{};serviceTable[31]=reinterpret_cast<void*>(&textureCount);void** serviceVtable=serviceTable;void*** servicePointer=&serviceVtable;for(auto fix:vertexTextureCallbackFixups){unsigned address=static_cast<unsigned>(reinterpret_cast<uintptr_t>(&servicePointer));std::memcpy(runCode+4096+fix[0],&address,4);}for(auto call:vertexTextureCallbackCalls){unsigned delta=static_cast<unsigned>(reinterpret_cast<uintptr_t>(runCode+sharedRefillOffset)-reinterpret_cast<uintptr_t>(runCode+4096+call[0]+5));std::memcpy(runCode+4096+call[0]+1,&delta,4);}auto sharedWaitFunction=&sharedWait;unsigned bindingValues[]{static_cast<unsigned>(reinterpret_cast<uintptr_t>(sharedRing.data())),static_cast<unsigned>(reinterpret_cast<uintptr_t>(sharedPool.data())),static_cast<unsigned>(reinterpret_cast<uintptr_t>(&sharedWaitFunction))};installSharedRefill(dll,runCode,bindingValues);DWORD protection;if(!VirtualProtect(runCode,8192,PAGE_EXECUTE_READ,&protection))return 3;FlushInstructionCache(GetCurrentProcess(),runCode,8192);checkSharedRefillImage(runCode);void* table[95]{};table[94]=reinterpret_cast<void*>(&direct);table[47]=reinterpret_cast<void*>(&viewportDirect);table[65]=reinterpret_cast<void*>(&textureDirect);table[69]=reinterpret_cast<void*>(&textureSampler);void** object=table;
 std::ofstream oracle("outputs/native-restore-resource-boundaries-tail.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-restore-resource-boundaries-tail.bin",std::ios::binary);
#endif
 unsigned cases=0;for(bool queued:{true})for(bool mutation:{false})for(unsigned special=0;special<1;++special)for(uint32_t force:{1u})for(unsigned mask:{255u})for(suppressionMode=0;suppressionMode<4;++suppressionMode)for(bool gateMutation:{false,true})for(int32_t count:{1})for(bool live:{false,true})for(bool array:{false,true})for(unsigned capacity:{2050u,2057u,2058u,2060u,2080u})for(unsigned seed:{0u,0xffffffffu})for(unsigned ringIndex:{0u,499u})for(unsigned pressure=0;pressure<3;++pressure){publicationSeed=seed;publicationIndex=ringIndex;publicationFull=pressure!=0;pressurePoolFull=pressure==2;pressureWaits=0;initialCapacity=capacity;arrayTexture=array;liveTexture=live;mutateGate=gateMutation;textureStageCount=count;
  mutate=mutation;events.clear();std::array<float,6152> source{},cached{};source.fill(17);cached.fill(17);for(unsigned i=0;i<512;++i)if(mask&(1u<<(i/64)))source[(1024+i)*4]=18;auto initialCache=cached;
  uint32_t bits=special==1?0x80000000u:special==2?0x7fc12345u:special==3?0x7f800000u:0;std::memcpy(&source[1024*4+1],&bits,4);if(special>=2)std::memcpy(&cached[1024*4+1],&bits,4);
  viewportCommand.fill(0);viewportQueue.fill(0);viewportDirectCalls=textureDirectCalls=textureCountCalls=textureSamplerCalls=0;textureRecord.fill(0xdddddddd);unsigned samplerWords[]{11,12,13,14,15,16};std::copy(samplerWords,samplerWords+6,textureRecord.begin());auto recordBytes=reinterpret_cast<unsigned char*>(textureRecord.data());recordBytes[0x1a]=arrayTexture?2:1;recordBytes[0x1b]=arrayTexture?1:0;textureRecord[0x48/4]=arrayTexture?static_cast<uint32_t>(reinterpret_cast<uintptr_t>(nativeTextureResources.data())):0x1234;const auto expectedNativeResources=nativeTextureResources;auto expectedTextureRecord=textureRecord;auto sourceBefore=source;auto cacheBefore=cached;liveSource=source.data();std::array<uint32_t,4096> block{};block[0]=1;block[1]=0xcafef00d;initialBlock=block.data();uint32_t* activeBlock=block.data();uint32_t* cursor=block.data()+2;uint32_t remaining=initialCapacity;replacementBlock.fill(0xdddddddd);replacementBlock[0]=pressurePoolFull?2u:0u;planPackets(source);std::array<uint32_t,640> normalizedRing{},expectedRing{};if(plannedRefills)expectedRing[publicationIndex]=1;expectedRing[0x8d0/4]=publicationSeed+plannedRefills;expectedRing[0x8d4/4]=(publicationIndex+plannedRefills)%500;expectedRing[0x9d8/4]=publicationSeed-(publicationFull?(plannedRefills?499u:500u):0u);
#ifdef _M_IX86
  std::array<uint32_t,531> current{},cache{};std::copy(viewportLive.begin(),viewportLive.end(),current.begin());current[0x848/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(source.data()));cache[0x848/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(cached.data()));std::array<uint32_t,8> command{};command[0]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&object));command[4]=queued?1:0;command[5]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(activeBlock));command[6]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(cursor));command[7]=remaining;sharedRing.fill(0);sharedRing[0x8d0/4]=publicationSeed;sharedRing[0x8d4/4]=publicationIndex;sharedRing[0x9d8/4]=publicationSeed-(publicationFull?500u:0u);sharedWaits=0;for(unsigned i=0;i<500;++i){sharedOccupied[i].fill(0xaabb0000u+i);sharedOccupied[i][0]=2;sharedPool[i]=sharedOccupied[i].data();}sharedPool[0]=replacementBlock.data();auto expectedPool=sharedPool;auto expectedOccupied=sharedOccupied;invoke(command.data(),current.data(),cache.data(),force);if(sharedPool!=expectedPool||sharedOccupied!=expectedOccupied||sharedWaits!=unsigned(publicationFull&&plannedRefills))std::abort();activeBlock=reinterpret_cast<uint32_t*>(command[5]);if(plannedRefills&&sharedRing[publicationIndex]!=reinterpret_cast<uintptr_t>(block.data()))std::abort();normalizedRing=sharedRing;if(plannedRefills)normalizedRing[publicationIndex]=1;pressureWaits=sharedWaits;cursor=reinterpret_cast<uint32_t*>(command[6]);remaining=command[7];ownerRecord[0x660/4]=static_cast<uint32_t>((cursor-activeBlock)*4);
#else
  Ops ops{source.data(),cached.data(),queued,cursor,remaining,activeBlock};ops.publication.produced=publicationSeed;ops.publication.completed=publicationSeed-(publicationFull?500u:0u);ops.publication.index=publicationIndex;for(unsigned i=0;i<500;++i){ops.occupied[i].fill(0xaabb0000u+i);ops.occupied[i][0]=2;ops.pool[i]=ops.occupied[i].data();}ops.pool[0]=replacementBlock.data();auto expectedPool=ops.pool;auto expectedOccupied=ops.occupied;NativeManagerCallbackAllocatorCandidate allocator;NativeManagerCallbackRegistryCandidateT<NativeManagerCallbackAllocatorCandidate,decltype(&ownedCallback)> registry{allocator};registry.append(ownedCallback);registry.append(ownedViewport);registry.append(ownedObserver);observerCalls=secondCalls=0;ownerRecord.fill(0xabababab);std::memset(ownerRecord.data()+0x648/4,0,32);std::memset(ownerRecord.data()+0x2010/4,0,531*4);std::memset(ownerRecord.data()+0x1198/4,0,531*4);ownerRecord[0]=ownerRecord[0x2e8c/4]=ownerRecord[0x2ea0/4]=ownerRecord[0x2eac/4]=0;std::copy(viewportLive.begin(),viewportLive.end(),ownerRecord.begin()+0x2010/4);ownerRecord[0x2858/4]=1;ownerRecord[0x19e0/4]=2;ownerRecord[0x658/4]=queued?1:0;ownerRecord[0x65c/4]=1;ownerRecord[0x660/4]=8;ownerRecord[0x664/4]=initialCapacity;ownerRecord[0x2e98/4]=3;ownerRecord[0x2eac/4]=2;auto bytes=reinterpret_cast<unsigned char*>(ownerRecord.data());bytes[0x2e88]=0x82;bytes[0x2e89]=0x81;bytes[0x30d8]=7;configureGate(ownerRecord);for(unsigned i=0;i<3;++i){ownerRecord[0x2ab0/4+i*6]=liveTexture&&i==0?2u:0u;ownerRecord[0x1c38/4+i*6]=1;}NativeManagerCallbackRegistryCandidateT<NativeManagerCallbackAllocatorCandidate,decltype(&ownedCallback)> second{allocator};second.append(ownedTexture);second.append(ownedSecondObserver);Snapshot snapshot{cursor,activeBlock,remaining};unsigned gates=0;auto gate=[&]{++gates;return studioRestoreOwnerGateCandidate(ownerRecord[0xa8/4],bytes[0xac],ownerRecord[0x2cc/4])!=0;};nativeRestoreDispatchSnapshotFinishCandidate(registry,bytes[0x2e88],second,bytes[0x2e89],gate,snapshot,&ops,source.data(),cached.data(),force);if(registry.size()!=(suppressionMode?3:0)||observerCalls!=(suppressionMode?0u:1u)||second.size()!=(suppressionMode||mutateGate?2:0)||secondCalls!=(!suppressionMode&&!mutateGate?1u:0u)||snapshot.finishes!=1||gates!=2)std::abort();
#endif
  if(nativeTextureResources!=expectedNativeResources||textureRecord!=expectedTextureRecord||textureSamplerCalls!=(!suppressionMode&&!mutateGate&&!queued&&liveTexture?5u:0u))return 20;
  if(textureCountCalls!=(!suppressionMode&&!mutateGate?1u:0u)||textureDirectCalls!=(!suppressionMode&&!mutateGate&&!queued?positiveTextureCount():0u))return 18;
  if(block!=expectedOldBlock||replacementBlock!=expectedNewBlock||activeBlock!=plannedBlock||cursor!=activeBlock+plannedOffset||remaining!=plannedRemaining)return 22;
#ifndef _M_IX86
  if(ops.pool!=expectedPool||ops.occupied!=expectedOccupied||ops.waits!=unsigned(publicationFull&&plannedRefills)||(plannedRefills&&ops.publication.ring[publicationIndex]!=reinterpret_cast<uintptr_t>(block.data())))std::abort();for(unsigned i=0;i<500;++i)normalizedRing[i]=plannedRefills&&i==publicationIndex?1u:static_cast<uint32_t>(ops.publication.ring[i]);normalizedRing[0x8d0/4]=ops.publication.produced;normalizedRing[0x8d4/4]=ops.publication.index;normalizedRing[0x9d8/4]=ops.publication.completed;pressureWaits=ops.waits;
#endif
  if(normalizedRing!=expectedRing)return 23;
  for(unsigned i=0;i<1024*4;++i)if(cached[i]!=initialCache[i])return 10;for(unsigned i=1536*4;i<cached.size();++i)if(cached[i]!=initialCache[i])return 11;
  std::array<uint32_t,29360> record{};record[0]=remaining;record[1]=static_cast<uint32_t>(cursor-activeBlock);record[2]=static_cast<uint32_t>(events.size());std::memcpy(record.data()+3,source.data(),sizeof(source));std::memcpy(record.data()+6155,cached.data(),sizeof(cached));std::copy(block.begin(),block.end(),record.begin()+12307);if(events.size()>4096)return 4;std::copy(events.begin(),events.end(),record.begin()+16403);std::copy(ownerRecord.begin(),ownerRecord.end(),record.begin()+20499);std::copy(viewportCommand.begin(),viewportCommand.end(),record.begin()+24595);std::copy(viewportQueue.begin(),viewportQueue.end(),record.begin()+24603);record[24619]=viewportDirectCalls;record[24620]=textureDirectCalls;record[24621]=textureCountCalls;record[24622]=textureSamplerCalls;std::copy(replacementBlock.begin(),replacementBlock.end(),record.begin()+24623);std::copy(normalizedRing.begin(),normalizedRing.end(),record.begin()+28719);record[29359]=pressureWaits;
#ifdef _M_IX86
  oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
  std::array<uint32_t,29360> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(record!=expected){std::printf("mismatch q=%u mutation=%u special=%u force=%u mask=%u\n",queued,mutation,special,force,mask);return 5;}
#endif
  if(!oracle)return 6;++cases;
 }
#ifdef _M_IX86
 VirtualFree(runCode,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 7;
#endif
 _mm_setcsr(previousMxcsr);std::printf("restore_resource_boundaries_tail_cases=%u passed\n",cases);
}

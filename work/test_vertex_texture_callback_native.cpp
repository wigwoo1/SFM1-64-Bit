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
#include "vertex_texture_callback_native_init.h"
#include "studio_vertex_texture_submit_candidate.h"
static std::vector<uint32_t> events;
static int32_t stageCount;
static bool mutateFields;
static bool switchQueue;
static uint32_t* activeQueue;
static uint32_t* liveSampler;
static HRESULT event(uint32_t stage,uint32_t type,uint32_t value){events.insert(events.end(),{stage,type,value});if(mutateFields){if(type==0)liveSampler[0]=91;if(type==1)liveSampler[1]=92;}if(switchQueue&&type==0)*activeQueue^=1;return E_FAIL;}
#ifdef _M_IX86
static int32_t __fastcall count(void*,void*){return stageCount;}
static void __fastcall refill(void*,void*){std::abort();}
static HRESULT __stdcall texture(void*,uint32_t stage,void* resource){return event(stage,0,static_cast<uint32_t>(reinterpret_cast<uintptr_t>(resource)));}
static HRESULT __stdcall sampler(void*,uint32_t stage,uint32_t type,uint32_t value){return event(stage,type,value);}
#endif
struct Ops{
 uint32_t& queued;uint32_t*& cursor;uint32_t& remaining;
 bool queueEnabled(){return queued;}
 void queueTexture(uint32_t stage,uintptr_t resource){if(remaining<3)std::abort();remaining-=3;*cursor++=2;*cursor++=stage;*cursor++=static_cast<uint32_t>(resource);}
 void queueSampler(uint32_t stage,uint32_t type,uint32_t value){if(remaining<4)std::abort();remaining-=4;*cursor++=17;*cursor++=stage;*cursor++=type;*cursor++=value;}
 void directTexture(uint32_t stage,uintptr_t resource){event(stage,0,static_cast<uint32_t>(resource));}
 void directSampler(uint32_t stage,uint32_t type,uint32_t value){event(stage,type,value);}
};
int main(){
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);
 std::array<unsigned char,sizeof(vertexTextureCallbackBytes)> original{};dll.seekg(vertexTextureCallbackFileOffset);dll.read(reinterpret_cast<char*>(original.data()),original.size());if(!dll||std::memcmp(original.data(),vertexTextureCallbackBytes,original.size()))return 1;
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,original.data(),original.size());
 void* serviceTable[32]{};serviceTable[31]=reinterpret_cast<void*>(&count);void** serviceVtable=serviceTable;void*** servicePointer=&serviceVtable;
 for(auto& fixup:vertexTextureCallbackFixups){const uint32_t address=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&servicePointer));std::memcpy(code+fixup[0],&address,4);}
 for(auto& call:vertexTextureCallbackCalls){const uint32_t delta=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&refill)-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&delta,4);}
 DWORD previous;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&previous))return 3;FlushInstructionCache(GetCurrentProcess(),code,4096);
 auto native=reinterpret_cast<void(__cdecl*)(void*,void*,void*,uint32_t)>(code);
 void* deviceTable[70]{};deviceTable[65]=reinterpret_cast<void*>(&texture);deviceTable[69]=reinterpret_cast<void*>(&sampler);void** deviceVtable=deviceTable;
 std::ofstream oracle("outputs/native-vertex-texture-callback.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-vertex-texture-callback.bin",std::ios::binary);
#endif
 constexpr unsigned fields[]={0,1,4,3,5};unsigned cases=0;
 for(bool queueSwitch:{false,true})for(bool mutation:{false,true})for(int32_t countValue:{-1,0,1,2,3})for(bool queued:{false,true})for(bool same:{false,true})for(bool array:{false,true})for(bool nullRecord:{false,true})for(uint32_t force:{0u,256u,1u,257u})for(unsigned mask=0;mask<32;++mask){
  switchQueue=queueSwitch;mutateFields=mutation;stageCount=countValue;events.clear();std::array<uint32_t,128> block{};uint32_t* cursor=block.data()+1;uint32_t remaining=127;uint32_t queueMode=queued?1:0;activeQueue=&queueMode;
  std::array<uint32_t,6> values{11,12,13,14,15,16},cacheValues=values;for(unsigned j=0;j<5;++j)if(mask&(1u<<j))cacheValues[fields[j]]=0;
  std::array<uint32_t,18> cacheResult{};
#ifdef _M_IX86
  std::array<uint32_t,20> record{};std::copy(values.begin(),values.end(),record.begin());auto* bytes=reinterpret_cast<unsigned char*>(record.data());bytes[0x1a]=array?2:1;bytes[0x1b]=1;uint32_t resources[]={0x1234,0x5678};record[0x48/4]=array?static_cast<uint32_t>(reinterpret_cast<uintptr_t>(resources)):resources[0];
  liveSampler=record.data();const uint32_t recordPointer=nullRecord?0:static_cast<uint32_t>(reinterpret_cast<uintptr_t>(record.data()));if(recordPointer>0x7fffffff)return 4;
  std::array<uint32_t,700> current{},cache{};
  constexpr unsigned cacheOffsets[]={1,2,4,3,5};
  for(unsigned stage=0;stage<3;++stage){const unsigned base=0xaa0/4+stage*6;current[base]=recordPointer;cache[base]=same?recordPointer:1;for(unsigned j=0;j<5;++j)cache[base+cacheOffsets[j]]=cacheValues[fields[j]];}
  std::array<uint32_t,8> command{};command[0]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&deviceVtable));command[4]=queueMode;command[6]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(cursor));command[7]=remaining;activeQueue=&command[4];
  native(command.data(),current.data(),cache.data(),force);cursor=reinterpret_cast<uint32_t*>(command[6]);remaining=command[7];
  for(unsigned stage=0;stage<3;++stage){const unsigned base=0xaa0/4+stage*6;const bool visited=static_cast<int32_t>(stage)<stageCount;const uint32_t expectedPointer=visited?recordPointer:same?recordPointer:1;if(cache[base]!=expectedPointer)return 8;cacheResult[stage*6]=cache[base]==0?1:0;for(unsigned j=0;j<5;++j)cacheResult[stage*6+j+1]=cache[base+cacheOffsets[j]];}
#else
  uintptr_t resources[]={0x1234,0x5678};StudioVertexTextureRecordCandidate record{values,uint8_t(array?2:1),1,array?reinterpret_cast<uintptr_t>(resources):resources[0]};auto* current=nullRecord?nullptr:&record;
  liveSampler=record.sampler.data();std::array<StudioVertexTextureRecordCandidate*,3> currents{current,current,current};std::array<StudioVertexTextureCacheCandidate,3> caches{};for(auto& cache:caches)cache={same?current:reinterpret_cast<StudioVertexTextureRecordCandidate*>(uintptr_t{1}),cacheValues};Ops ops{queueMode,cursor,remaining};studioVertexTextureSubmitCandidate(currents.data(),caches.data(),stageCount,force,ops);
  for(unsigned stage=0;stage<3;++stage){const bool visited=static_cast<int32_t>(stage)<stageCount;auto* expectedPointer=visited?current:same?current:reinterpret_cast<StudioVertexTextureRecordCandidate*>(uintptr_t{1});if(caches[stage].record!=expectedPointer)return 8;cacheResult[stage*6]=!caches[stage].record?1:0;for(unsigned j=0;j<5;++j)cacheResult[stage*6+j+1]=caches[stage].sampler[fields[j]];}
#endif
  std::array<uint32_t,208> result{};result[0]=remaining;result[1]=static_cast<uint32_t>(cursor-block.data());result[2]=static_cast<uint32_t>(events.size());std::copy(cacheResult.begin(),cacheResult.end(),result.begin()+3);std::copy(block.begin(),block.end(),result.begin()+21);std::copy(events.begin(),events.end(),result.begin()+149);result[203]=liveSampler[0];result[204]=liveSampler[1];result[205]=*activeQueue;
#ifdef _M_IX86
  oracle.write(reinterpret_cast<const char*>(result.data()),sizeof(result));
#else
  std::array<uint32_t,208> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(result!=expected){std::printf("mismatch count=%d q=%u same=%u array=%u null=%u force=%u mask=%u\n",stageCount,queued,same,array,nullRecord,force,mask);return 5;}
#endif
  if(!oracle)return 6;++cases;
 }
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 7;
#endif
 std::printf("vertex_texture_original_comparison_cases=%u passed\n",cases);
}

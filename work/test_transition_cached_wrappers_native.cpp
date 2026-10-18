#define NOMINMAX
#include <Windows.h>
#include <array>
#include <vector>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "transition_wrapper_native_init.h"
#include "studio_transition_alpha_helper_candidate.h"
#include "studio_transition_blend_helper_candidate.h"
#include "studio_transition_separate_blend_helper_candidate.h"
static std::array<unsigned,128> cache;
static std::array<unsigned,128> replacementCache;
static void* cacheObject;
static std::array<unsigned,32> queue;
static std::vector<unsigned> trace;
static StudioTransitionState72Candidate* current;
static unsigned handler,mutation,getters,used;
static bool queued;
static void submit(unsigned key,unsigned value){
 trace.insert(trace.end(),{key,value});
 if(mutation==3)cacheObject=replacementCache.data();
 const unsigned enableKey=handler==0?15:handler==1?27:206;
 auto& flags=handler==2?current->flags45:current->flags44;
 const unsigned bit=handler==0?64:handler==1?32:1;
 const unsigned base=handler==0?10:handler==1?4:7;
 const unsigned firstKey=handler==0?25:handler==1?19:207;
 const unsigned secondKey=handler==0?24:handler==1?20:208;
 if(key==enableKey&&mutation==1)flags=static_cast<std::uint8_t>(bit);
 if(key==enableKey&&mutation==2)flags=0;
 if(mutation==3){if(key==firstKey){current->values[base]=99;current->values[base+1]=100;}if(key==secondKey){current->values[base+1]=101;if(handler!=0)current->values[base+2]=102;}if(key==(handler==1?171u:209u))current->values[base+2]=103;}
}
#ifdef _M_IX86
struct Command{void* device;unsigned pad[3];unsigned mode,pad14;unsigned* cursor;unsigned remaining;};
static Command command;
static void* __cdecl getter(){++getters;return &command;}
static void __fastcall refill(void*,void*){std::abort();}
static HRESULT __stdcall onRender(void*,unsigned key,unsigned value){submit(key,value);return E_FAIL;}
#endif
struct Services{void render(unsigned key,unsigned value){++getters;if(queued){queue[used++]=1;queue[used++]=key;queue[used++]=value;}else submit(key,value);}};
int main(){
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);std::array<unsigned char*,8> bodies{};
 auto load=[&](unsigned index,unsigned size,unsigned offset,const auto& calls,bool wrapper){
  std::vector<unsigned char> saved(size),actual(size);std::ifstream f("outputs/transition-wrapper-body-"+std::to_string(index)+".bin",std::ios::binary);f.read(reinterpret_cast<char*>(saved.data()),size);if(!f||f.peek()!=std::char_traits<char>::eof())std::abort();dll.clear();dll.seekg(offset);dll.read(reinterpret_cast<char*>(actual.data()),size);if(!dll||actual!=saved)std::abort();auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)std::abort();std::memcpy(code,saved.data(),size);
  for(const auto& call:calls){auto* target=wrapper?bodies[index+1]:call[1]==0x1000df50?reinterpret_cast<unsigned char*>(&getter):reinterpret_cast<unsigned char*>(&refill);const auto displacement=static_cast<unsigned>(reinterpret_cast<uintptr_t>(target)-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&displacement,4);}
  if(wrapper){const auto address=reinterpret_cast<uintptr_t>(&cacheObject);std::memcpy(code+8,&address,4);}
  DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))std::abort();FlushInstructionCache(GetCurrentProcess(),code,4096);return code;
 };
 bodies[3]=load(3,transitionWrapper3Size,transitionWrapper3Offset,transitionWrapper3Calls,false);bodies[2]=load(2,transitionWrapper2Size,transitionWrapper2Offset,transitionWrapper2Calls,true);
 bodies[5]=load(5,transitionWrapper5Size,transitionWrapper5Offset,transitionWrapper5Calls,false);bodies[4]=load(4,transitionWrapper4Size,transitionWrapper4Offset,transitionWrapper4Calls,true);
 bodies[7]=load(7,transitionWrapper7Size,transitionWrapper7Offset,transitionWrapper7Calls,false);bodies[6]=load(6,transitionWrapper6Size,transitionWrapper6Offset,transitionWrapper6Calls,true);
 void* deviceTable[58]{};deviceTable[57]=reinterpret_cast<void*>(&onRender);void* device=deviceTable;
 std::ofstream oracle("outputs/native-transition-cached-wrappers.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-transition-cached-wrappers.bin",std::ios::binary);Services services;
#endif
 unsigned cases=0;
 for(handler=0;handler<3;++handler)for(unsigned flags=0;flags<256;++flags)for(unsigned enable:{0u,1u,255u})for(unsigned equal=0;equal<8;++equal)for(unsigned mode=0;mode<2;++mode)for(mutation=0;mutation<4;++mutation){
  StudioTransitionState72Candidate state{};current=&state;state.flags44=state.flags45=static_cast<std::uint8_t>(flags);
  const unsigned base=handler==0?10:handler==1?4:7;state.values[base]=7;state.values[base+1]=13;if(handler!=0)state.values[base+2]=17;
  cache.fill(0xcccccccc);const unsigned offset=handler==0?0x128:handler==1?0xfc:0x10c;
  reinterpret_cast<unsigned char*>(cache.data())[offset]=static_cast<unsigned char>(enable);
  cache[(offset+4)/4]=(equal&1)?7:0;cache[(offset+8)/4]=(equal&2)?13:0;if(handler!=0)cache[(offset+12)/4]=(equal&4)?17:0;
  replacementCache.fill(0xabababab);cacheObject=cache.data();queue.fill(0xdddddddd);trace.clear();getters=used=0;queued=mode!=0;
#ifdef _M_IX86
  cacheObject=cache.data();command={};command.device=&device;command.mode=mode;command.cursor=queue.data();command.remaining=32;
  reinterpret_cast<void(__cdecl*)(void*,unsigned)>(bodies[2+2*handler])(&state,0);used=static_cast<unsigned>(command.cursor-queue.data());if(command.remaining!=32-used)std::abort();
#else
  if(handler==0){StudioTransitionAlphaCacheCandidate c{static_cast<std::uint8_t>(enable),cache[(offset+4)/4],cache[(offset+8)/4]};studioTransitionAlphaHelperCandidate(state,c,services);reinterpret_cast<unsigned char*>(cache.data())[offset]=c.enabled128;cache[(offset+4)/4]=c.function12c;cache[(offset+8)/4]=c.reference130;}
  else if(handler==1){StudioTransitionBlendCacheCandidate c{static_cast<std::uint8_t>(enable),cache[(offset+4)/4],cache[(offset+8)/4],cache[(offset+12)/4]};studioTransitionBlendHelperCandidate(state,c,services);reinterpret_cast<unsigned char*>(cache.data())[offset]=c.enabledFc;cache[(offset+4)/4]=c.operation100;cache[(offset+8)/4]=c.source104;cache[(offset+12)/4]=c.destination108;}
  else{StudioTransitionSeparateBlendCacheCandidate c{static_cast<std::uint8_t>(enable),cache[(offset+4)/4],cache[(offset+8)/4],cache[(offset+12)/4]};studioTransitionSeparateBlendHelperCandidate(state,c,services);reinterpret_cast<unsigned char*>(cache.data())[offset]=c.enabled10c;cache[(offset+4)/4]=c.operation110;cache[(offset+8)/4]=c.source114;cache[(offset+12)/4]=c.destination118;}
#endif
  if(getters!=(queued?used/3:trace.size()/2)||used>12||trace.size()>8)std::abort();
  for(auto word:replacementCache)if(word!=0xabababab)std::abort();
  const bool switched=!queued&&mutation==3&&!trace.empty();if((cacheObject==replacementCache.data())!=switched)std::abort();
  std::array<unsigned,320> record{};std::memcpy(record.data(),&state,72);std::memcpy(record.data()+18,cache.data(),512);std::memcpy(record.data()+146,queue.data(),128);for(unsigned i=0;i<trace.size();++i)record[178+i]=trace[i];record[186]=static_cast<unsigned>(trace.size());record[187]=used;record[188]=getters;record[189]=switched?1u:0u;std::memcpy(record.data()+192,replacementCache.data(),512);
#ifdef _M_IX86
  oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
  std::array<unsigned,320> native{};oracle.read(reinterpret_cast<char*>(native.data()),sizeof(native));if(native!=record){std::printf("mismatch case=%u handler=%u\n",cases,handler);return 1;}
#endif
  if(!oracle)std::abort();++cases;
 }
#ifdef _M_IX86
 for(auto* body:bodies)if(body)VirtualFree(body,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 2;
#endif
 std::printf("transition_cached_wrappers_original_cases=%u passed\n",cases);
}


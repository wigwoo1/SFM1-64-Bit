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
#include "studio_transition_state194_helper_candidate.h"
static StudioTransitionState72Candidate* current;
static std::array<unsigned,128> cache;
static std::array<unsigned,16> queue;
static std::vector<unsigned> trace;
static unsigned capabilityValue,extra,getters,used;static int version;static bool mutation,queued;
static unsigned cap41(){trace.push_back(41);return capabilityValue;}
static int cap17(){trace.push_back(17);return version;}
static unsigned cap25(){trace.push_back(25);return extra;}
static void submit(unsigned key,unsigned value){trace.insert(trace.end(),{key,value});if(mutation){current->flags44=129;current->flags45=4;current->value40=99;}}
static void backend271(unsigned value){trace.insert(trace.end(),{271,value});}
static void combined(unsigned v,unsigned a,unsigned b,unsigned c){trace.insert(trace.end(),{264,v,a,b,c});}
#ifdef _M_IX86
struct Command{void* device;unsigned pad[3];unsigned mode,pad14;unsigned* cursor;unsigned remaining;};
static Command command;static void* cacheObject;static void* backendObject;static void* capabilityObject;
static void* __cdecl getter(){++getters;return &command;}
static void __fastcall refill(void*,void*){std::abort();}
static unsigned __fastcall on41(void*,void*){return cap41();}
static int __fastcall on17(void*,void*){return cap17();}
static unsigned __fastcall on25(void*,void*){return cap25();}
static void __fastcall on271(void*,void*,unsigned v){backend271(v);}
static void __fastcall onCombined(void*,void*,unsigned v,unsigned a,unsigned b,unsigned c){combined(v,a,b,c);}
static HRESULT __stdcall onRender(void*,unsigned key,unsigned value){submit(key,value);return E_FAIL;}
#endif
struct Services{
 unsigned capability41(){return cap41();}int capability17(){return cap17();}unsigned capability25(){return cap25();}
 void render(unsigned key,unsigned value){++getters;if(queued){queue[used++]=1;queue[used++]=key;queue[used++]=value;}else submit(key,value);}
 void applyBackend271(unsigned value){backend271(value);}void applyCombined(unsigned v,unsigned a,unsigned b,unsigned c){combined(v,a,b,c);}
};
int main(){
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);std::array<unsigned char*,2> bodies{};
 auto load=[&](unsigned index,unsigned size,unsigned offset,const auto& calls,const auto& fixups){
  std::vector<unsigned char> saved(size),actual(size);std::ifstream f("outputs/transition-wrapper-body-"+std::to_string(index)+".bin",std::ios::binary);f.read(reinterpret_cast<char*>(saved.data()),size);if(!f||f.peek()!=std::char_traits<char>::eof())std::abort();dll.clear();dll.seekg(offset);dll.read(reinterpret_cast<char*>(actual.data()),size);if(!dll||actual!=saved)std::abort();auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)std::abort();std::memcpy(code,saved.data(),size);
  for(const auto& call:calls){auto* target=index==0?bodies[1]:call[1]==0x1000df50?reinterpret_cast<unsigned char*>(&getter):reinterpret_cast<unsigned char*>(&refill);if(!target)std::abort();const auto displacement=static_cast<unsigned>(reinterpret_cast<uintptr_t>(target)-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&displacement,4);}
  for(const auto& fix:fixups){void* cell=fix[1]==0x10183fc4?static_cast<void*>(&cacheObject):fix[1]==0x1019092c?static_cast<void*>(&capabilityObject):static_cast<void*>(&backendObject);const auto address=reinterpret_cast<uintptr_t>(cell);std::memcpy(code+fix[0],&address,4);}
  DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))std::abort();FlushInstructionCache(GetCurrentProcess(),code,4096);return code;
 };
 bodies[1]=load(1,transitionWrapper1Size,transitionWrapper1Offset,transitionWrapper1Calls,transitionWrapper1Fixups);bodies[0]=load(0,transitionWrapper0Size,transitionWrapper0Offset,transitionWrapper0Calls,transitionWrapper0Fixups);
 void* deviceTable[58]{};deviceTable[57]=reinterpret_cast<void*>(&onRender);void* device=deviceTable;
 void* capTable[42]{};capTable[41]=reinterpret_cast<void*>(&on41);capTable[17]=reinterpret_cast<void*>(&on17);capTable[25]=reinterpret_cast<void*>(&on25);void* capReceiver=capTable;capabilityObject=&capReceiver;
 void* backendTable[272]{};backendTable[271]=reinterpret_cast<void*>(&on271);backendTable[264]=reinterpret_cast<void*>(&onCombined);void* backendReceiver=backendTable;backendObject=&backendReceiver;
 std::ofstream oracle("outputs/native-transition-state194.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-transition-state194.bin",std::ios::binary);Services services;
#endif
 unsigned cases=0;
 for(unsigned flags=0;flags<256;++flags)for(unsigned forced:{0u,1u,255u})for(unsigned cap:{0u,1u,256u})for(int ver:{-1,91,92})for(unsigned ext:{0u,1u,256u})for(unsigned mode=0;mode<2;++mode)for(unsigned change=0;change<2;++change){
  StudioTransitionState72Candidate state{};state.flags44=static_cast<std::uint8_t>(flags);state.flags45=4;state.value40=17;current=&state;capabilityValue=cap;version=ver;extra=ext;queued=mode!=0;mutation=change!=0;
  cache.fill(0xcccccccc);reinterpret_cast<unsigned char*>(cache.data())[0x144]=static_cast<unsigned char>(forced);queue.fill(0xdddddddd);trace.clear();getters=used=0;
#ifdef _M_IX86
  cacheObject=cache.data();command={};command.device=&device;command.mode=mode;command.cursor=queue.data();command.remaining=16;reinterpret_cast<void(__cdecl*)(void*,unsigned)>(bodies[0])(&state,0);used=static_cast<unsigned>(command.cursor-queue.data());if(command.remaining!=16-used)std::abort();
#else
  studioTransitionState194HelperCandidate(state,static_cast<std::uint8_t>(forced),services);
#endif
  const bool changed=!queued&&mutation;const unsigned value=forced?0u:cap&&ver>=92?0u:(flags>>7);std::vector<unsigned> expected;
  if(!forced){expected.push_back(41);if(cap)expected.push_back(17);}if(!queued)expected.insert(expected.end(),{194,value});expected.insert(expected.end(),{271,forced?0u:changed?1u:(flags>>7)});
  if(!forced){expected.push_back(25);if(ext==1)expected.insert(expected.end(),{264,changed?99u:17u,changed?1u:(flags&1u),changed?1u:(flags>>7),1});}
  if(trace!=expected||getters!=1||used!=(queued?3u:0u))std::abort();if(queued&&(queue[0]!=1||queue[1]!=194||queue[2]!=value))std::abort();
  std::array<unsigned,184> record{};std::memcpy(record.data(),&state,72);std::memcpy(record.data()+18,cache.data(),512);std::memcpy(record.data()+146,queue.data(),64);for(unsigned i=0;i<trace.size();++i)record[162+i]=trace[i];record[178]=static_cast<unsigned>(trace.size());record[179]=used;record[180]=getters;
#ifdef _M_IX86
  oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
  std::array<unsigned,184> native{};oracle.read(reinterpret_cast<char*>(native.data()),sizeof(native));if(native!=record){std::printf("mismatch case=%u\n",cases);return 1;}
#endif
  if(!oracle)std::abort();++cases;
 }
#ifdef _M_IX86
 for(auto* body:bodies)VirtualFree(body,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 2;
#endif
 std::printf("transition_state194_original_cases=%u passed\n",cases);
}


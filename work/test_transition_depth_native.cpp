#define NOMINMAX
#include <Windows.h>
#include <array>
#include <vector>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "transition_depth_native_init.h"
#include "studio_transition_depth_helper_candidate.h"
static std::array<unsigned,128> cache;
static std::array<unsigned,16> queue;
static std::vector<unsigned> trace;
static StudioTransitionState72Candidate* current;
static unsigned getters,used;static bool queued,mutation;
static void submit(unsigned key,unsigned value){trace.insert(trace.end(),{key,value});if(mutation&&key==7){current->values[1]=0;current->flags44=8;}}
static void backend(StudioTransitionState72Candidate& state){if(&state!=current)std::abort();trace.push_back(261);if(mutation)state.flags44=12;}
#ifdef _M_IX86
struct Command{void* device;unsigned pad[3];unsigned mode,pad14;unsigned* cursor;unsigned remaining;};
static Command command;static void* cacheObject;static void* backendObject;
static void* __cdecl getter(){++getters;return &command;}
static void __fastcall refill(void*,void*){std::abort();}
static HRESULT __stdcall onRender(void*,unsigned key,unsigned value){submit(key,value);return E_FAIL;}
static void __fastcall onBackend(void*,void*,StudioTransitionState72Candidate* state){backend(*state);}
#endif
struct Services{
 void submit(unsigned key,unsigned value){++getters;if(queued){queue[used++]=1;queue[used++]=key;queue[used++]=value;}else ::submit(key,value);}
 void applyDepthBackend(StudioTransitionState72Candidate& state){backend(state);}
};
int main(){
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);std::array<unsigned char*,4> bodies{};
 auto load=[&](unsigned index,unsigned size,unsigned offset,const auto& calls){
  std::vector<unsigned char> saved(size),actual(size);std::ifstream f("outputs/transition-depth-body-"+std::to_string(index)+".bin",std::ios::binary);f.read(reinterpret_cast<char*>(saved.data()),size);if(!f||f.peek()!=std::char_traits<char>::eof())std::abort();dll.clear();dll.seekg(offset);dll.read(reinterpret_cast<char*>(actual.data()),size);if(!dll||actual!=saved)std::abort();auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)std::abort();std::memcpy(code,saved.data(),size);
  for(const auto& call:calls){unsigned char* target=nullptr;switch(call[1]){case 0x1002e510:target=bodies[1];break;case 0x1002e410:target=bodies[2];break;case 0x1002e490:target=bodies[3];break;case 0x1000df50:target=reinterpret_cast<unsigned char*>(&getter);break;case 0x10002da0:target=reinterpret_cast<unsigned char*>(&refill);break;default:std::abort();}if(!target)std::abort();const auto displacement=static_cast<unsigned>(reinterpret_cast<uintptr_t>(target)-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&displacement,4);}
  if(index<2){const auto address=reinterpret_cast<uintptr_t>(index==0?&cacheObject:&backendObject);std::memcpy(code+(index==0?transitionDepth0Fixups[0][0]:transitionDepth1Fixups[0][0]),&address,4);}
  DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))std::abort();FlushInstructionCache(GetCurrentProcess(),code,4096);return code;
 };
 bodies[2]=load(2,transitionDepth2Size,transitionDepth2Offset,transitionDepth2Calls);bodies[3]=load(3,transitionDepth3Size,transitionDepth3Offset,transitionDepth3Calls);bodies[1]=load(1,transitionDepth1Size,transitionDepth1Offset,transitionDepth1Calls);bodies[0]=load(0,transitionDepth0Size,transitionDepth0Offset,transitionDepth0Calls);
 void* deviceTable[58]{};deviceTable[57]=reinterpret_cast<void*>(&onRender);void* device=deviceTable;void* backendTable[266]{};backendTable[261]=reinterpret_cast<void*>(&onBackend);void* backendReceiver=backendTable;backendObject=&backendReceiver;
 std::ofstream oracle("outputs/native-transition-depth.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-transition-depth.bin",std::ios::binary);Services services;
#endif
 unsigned cases=0;
 for(unsigned flags=0;flags<256;++flags)for(unsigned cachedMode=0;cachedMode<4;++cachedMode)for(unsigned dirty:{0u,1u,255u})for(unsigned equal=0;equal<4;++equal)for(unsigned mode=0;mode<2;++mode)for(unsigned change=0;change<2;++change){
  StudioTransitionState72Candidate state{};state.values[0]=3;state.values[1]=1;state.flags44=static_cast<std::uint8_t>(flags);current=&state;queued=mode!=0;mutation=change!=0;
  cache.fill(0xcccccccc);cache[0x11c/4]=(equal&1)?1:0;cache[0x120/4]=(equal&2)?3:0;cache[0x124/4]=cachedMode;reinterpret_cast<unsigned char*>(cache.data())[0x178]=static_cast<unsigned char>(dirty);queue.fill(0xdddddddd);trace.clear();getters=used=0;
#ifdef _M_IX86
  cacheObject=cache.data();command={};command.device=&device;command.mode=mode;command.cursor=queue.data();command.remaining=16;reinterpret_cast<void(__cdecl*)(void*,unsigned)>(bodies[0])(&state,0);used=static_cast<unsigned>(command.cursor-queue.data());if(command.remaining!=16-used)std::abort();
#else
  StudioTransitionDepthCacheCandidate c{cache[0x11c/4],cache[0x120/4],cachedMode,static_cast<std::uint8_t>(dirty)};studioTransitionDepthHelperCandidate(state,c,services);cache[0x11c/4]=c.state7;cache[0x120/4]=c.state23;cache[0x124/4]=c.mode124;reinterpret_cast<unsigned char*>(cache.data())[0x178]=c.dirty178;
#endif
  const bool first=!(equal&1),changed=first&&!queued&&mutation,second=!changed&&!(equal&2);const unsigned effective=changed?8u:flags;const unsigned desired=(effective>>2)&3u;const bool call=cachedMode!=desired||((effective&12)==8&&dirty);
  const unsigned commands=unsigned(first)+unsigned(second);std::vector<unsigned> expected;if(!queued){if(first)expected.insert(expected.end(),{7,1});if(second)expected.insert(expected.end(),{23,3});}if(call)expected.push_back(261);
  if(trace!=expected||getters!=commands||used!=(queued?commands*3:0)||cache[0x11c/4]!=1||cache[0x120/4]!=(second?3u:((equal&2)?3u:0u))||cache[0x124/4]!=(call?(mutation?3u:desired):cachedMode))std::abort();
  std::array<unsigned,176> record{};std::memcpy(record.data(),&state,72);std::memcpy(record.data()+18,cache.data(),512);std::memcpy(record.data()+146,queue.data(),64);for(unsigned i=0;i<trace.size();++i)record[162+i]=trace[i];record[170]=static_cast<unsigned>(trace.size());record[171]=used;record[172]=getters;
#ifdef _M_IX86
  oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
  std::array<unsigned,176> native{};oracle.read(reinterpret_cast<char*>(native.data()),sizeof(native));if(native!=record){std::printf("mismatch case=%u\n",cases);return 1;}
#endif
  if(!oracle)std::abort();++cases;
 }
#ifdef _M_IX86
 for(auto* body:bodies)VirtualFree(body,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 2;
#endif
 std::printf("transition_depth_original_cases=%u passed\n",cases);
}




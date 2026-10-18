#define NOMINMAX
#include <Windows.h>
#include <array>
#include <vector>
#include <string>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "transition_command_handlers_native_init.h"
#include "studio_transition_command_handlers_candidate.h"
static StudioTransitionState72Candidate* current;
static std::array<unsigned,16> queue;
static std::vector<unsigned> trace;
static unsigned getters,reads,used,capability,cacheA[81],cacheB[81];
static bool queued,mutation;
static void* cacheObject;
static void mutate(){if(mutation){current->values[2]=123;cacheObject=cacheB;}}
static void render(unsigned key,unsigned value){trace.insert(trace.end(),{1,key,value});mutate();}
static void sampler(unsigned stage,unsigned key,unsigned value){trace.insert(trace.end(),{17,stage,key,value});mutate();}
static void get(){++getters;if(mutation){current->values[2]=99;cacheObject=cacheB;}}
static void emit(std::initializer_list<unsigned> words){for(auto word:words)queue[used++]=word;}
#ifdef _M_IX86
struct Command{void* device;unsigned pad[3];unsigned mode,pad14;unsigned* cursor;unsigned remaining;};
static Command command;
static void* capabilityObject;
static void* __cdecl getter(){get();return &command;}
static void __fastcall refill(void*,void*){std::abort();}
static unsigned __fastcall special(void*,void*){++reads;return capability;}
static HRESULT __stdcall onRender(void*,unsigned key,unsigned value){render(key,value);return E_FAIL;}
static HRESULT __stdcall onSampler(void*,unsigned stage,unsigned key,unsigned value){sampler(stage,key,value);return E_FAIL;}
#endif
struct Services{
 void render(unsigned key,unsigned value){get();if(queued)emit({1,key,value});else ::render(key,value);}
 void sampler(unsigned stage,unsigned key,unsigned value){get();if(queued)emit({17,stage,key,value});else ::sampler(stage,key,value);}
 unsigned specialCapability(){++reads;return capability;}
 void publishCache140(unsigned value){static_cast<unsigned*>(cacheObject)[80]=value;}
};
int main(){
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);
 std::array<unsigned char*,5> bodies{};
 auto load=[&](unsigned index,unsigned size,unsigned offset,const auto& calls,const std::vector<std::array<unsigned,2>>& fixups){
  std::vector<unsigned char> saved(size),actual(size);std::ifstream f("outputs/transition-command-handler-"+std::to_string(index)+".bin",std::ios::binary);f.read(reinterpret_cast<char*>(saved.data()),size);if(!f||f.peek()!=std::char_traits<char>::eof())std::abort();dll.clear();dll.seekg(offset);dll.read(reinterpret_cast<char*>(actual.data()),size);if(!dll||actual!=saved)std::abort();
  auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)std::abort();std::memcpy(code,saved.data(),size);
  for(const auto& call:calls){auto* target=call[1]==0x1000df50?reinterpret_cast<unsigned char*>(&getter):reinterpret_cast<unsigned char*>(&refill);const auto displacement=static_cast<unsigned>(reinterpret_cast<uintptr_t>(target)-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&displacement,4);}
  for(const auto& fix:fixups){const auto address=reinterpret_cast<uintptr_t>(fix[1]==0x10183fc4?&cacheObject:&capabilityObject);std::memcpy(code+fix[0],&address,4);}
  DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))std::abort();FlushInstructionCache(GetCurrentProcess(),code,4096);return code;
 };
 bodies[0]=load(0,transitionCommand0Size,transitionCommand0Offset,transitionCommand0Calls,{});
 bodies[1]=load(1,transitionCommand1Size,transitionCommand1Offset,transitionCommand1Calls,{});
 bodies[2]=load(2,transitionCommand2Size,transitionCommand2Offset,transitionCommand2Calls,{{transitionCommand2Fixups[0][0],transitionCommand2Fixups[0][1]},{transitionCommand2Fixups[1][0],transitionCommand2Fixups[1][1]}});
 bodies[3]=load(3,transitionCommand3Size,transitionCommand3Offset,transitionCommand3Calls,{});
 bodies[4]=load(4,transitionCommand4Size,transitionCommand4Offset,transitionCommand4Calls,{{transitionCommand4Fixups[0][0],transitionCommand4Fixups[0][1]}});
 void* deviceTable[70]{};deviceTable[57]=reinterpret_cast<void*>(&onRender);deviceTable[69]=reinterpret_cast<void*>(&onSampler);void* device=deviceTable;
 void* capTable[50]{};capTable[49]=reinterpret_cast<void*>(&special);void* capabilityReceiver=capTable;capabilityObject=&capabilityReceiver;
 std::ofstream oracle("outputs/native-transition-command.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-transition-command.bin",std::ios::binary);Services services;
#endif
 unsigned cases=0;
 for(unsigned handler=0;handler<5;++handler)for(unsigned byte=0;byte<256;++byte)for(unsigned stage=0;stage<(handler>=3?16u:1u);++stage)for(unsigned mode=0;mode<2;++mode)for(unsigned cap:{0u,1u,256u})for(unsigned change=0;change<2;++change){
  StudioTransitionState72Candidate state{};state.values[2]=17;state.values[3]=0x12345678;state.flags44=static_cast<unsigned char>(byte);state.values[12+stage/4]=byte<<((stage%4)*8);current=&state;queued=mode!=0;mutation=change!=0;capability=cap;cacheA[80]=31;cacheB[80]=47;cacheObject=cacheA;queue.fill(0xcccccccc);trace.clear();getters=reads=used=0;
#ifdef _M_IX86
  command={};command.device=&device;command.mode=mode;command.cursor=queue.data();command.remaining=16;
  reinterpret_cast<void(__cdecl*)(void*,unsigned)>(bodies[handler])(&state,stage);used=static_cast<unsigned>(command.cursor-queue.data());if(command.remaining!=16-used)std::abort();
#else
  switch(handler){case 0:studioTransitionRender8Candidate(state,services);break;case 1:studioTransitionRender14Candidate(state,services);break;case 2:studioTransitionRender168Candidate(state,services);break;case 3:studioTransitionSampler11Candidate(state,stage,services);break;case 4:studioTransitionSampler8Candidate(state,stage,services);break;}
#endif
  const bool active=handler!=4||cap==1;const unsigned key=handler==0?8:handler==1?14:handler==2?168:handler==3?11:8;
  const unsigned value=handler==0?0x12345678:handler==2?17:handler==4?((byte&4)?0x34544547u:0x31544547u):((byte&2)?1u:0u);
  std::vector<unsigned> expected;if(active){if(handler>=3)expected={17,stage,key,value};else expected={1,key,value};}
  if(getters!=unsigned(active)||reads!=unsigned(handler==4)||used!=(queued?expected.size():0)||trace!=(queued?std::vector<unsigned>{}:expected))std::abort();
  if(queued)for(unsigned i=0;i<expected.size();++i)if(queue[i]!=expected[i])std::abort();
  if(handler==2){const auto published=mutation?(queued?99u:123u):17u;if((mutation?cacheB[80]:cacheA[80])!=published)std::abort();}
  std::array<unsigned,48> record{};std::memcpy(record.data(),&state,72);std::memcpy(record.data()+18,queue.data(),64);for(unsigned i=0;i<trace.size();++i)record[34+i]=trace[i];record[38]=static_cast<unsigned>(trace.size());record[39]=used;record[40]=getters;record[41]=reads;record[42]=cacheA[80];record[43]=cacheB[80];
#ifdef _M_IX86
  oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
  std::array<unsigned,48> native{};oracle.read(reinterpret_cast<char*>(native.data()),sizeof(native));if(native!=record){std::printf("mismatch case=%u handler=%u\n",cases,handler);return 1;}
#endif
  if(!oracle)std::abort();++cases;
 }
#ifdef _M_IX86
 for(auto* body:bodies)VirtualFree(body,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 2;
#endif
 std::printf("transition_command_original_cases=%u passed\n",cases);
}


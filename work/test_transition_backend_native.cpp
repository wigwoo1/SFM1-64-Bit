#define NOMINMAX
#include <Windows.h>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "transition_backend_handlers_native_init.h"
#include "studio_transition_backend_handlers_candidate.h"
static std::vector<uint32_t> trace;static int stageCount;static unsigned countReads;static void* countObject;static void* backendObject;static void* secondBackend;static bool switched;
static void flag44(unsigned v){trace.push_back(v);}static void flag45(unsigned v){trace.push_back(v);}static void combined(unsigned v,unsigned a,unsigned b,unsigned c){trace.insert(trace.end(),{v,a,b,c});}
static void stage(StudioTransitionState72Candidate* p,int index){trace.push_back(static_cast<unsigned>(index));trace.push_back(switched?1u:0u);++p->value40;if(index==0){stageCount=0;switched=true;backendObject=secondBackend;}}
#ifdef _M_IX86
static int __fastcall getCount(void*,void*){++countReads;return stageCount;}
static void __fastcall on44(void*,void*,unsigned v){flag44(v);}static void __fastcall on45(void*,void*,unsigned v){flag45(v);}
static void __fastcall onCombined(void*,void*,unsigned v,unsigned a,unsigned b,unsigned c){combined(v,a,b,c);}
static void __fastcall onStage(void* receiver,void*,StudioTransitionState72Candidate* p,int index){if((receiver==secondBackend)!=switched)std::abort();stage(p,index);}
#endif
struct Services{int stageCount(){++countReads;return ::stageCount;}void applyStage(StudioTransitionState72Candidate* p,int i){stage(p,i);}void applyFlag44(unsigned v){flag44(v);}void applyFlag45(unsigned v){flag45(v);}void applyCombined(unsigned v,unsigned a,unsigned b,unsigned c){combined(v,a,b,c);}};
int main(){
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);std::array<unsigned char*,4> bodies{};
 auto load=[&](unsigned index,unsigned size,unsigned offset,const auto& fixups){std::vector<unsigned char> saved(size),actual(size);std::ifstream file("outputs/transition-backend-handler-"+std::to_string(index)+".bin",std::ios::binary);file.read(reinterpret_cast<char*>(saved.data()),size);if(!file||file.peek()!=std::char_traits<char>::eof())std::abort();dll.clear();dll.seekg(offset);dll.read(reinterpret_cast<char*>(actual.data()),size);if(!dll||actual!=saved)std::abort();auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)std::abort();std::memcpy(code,saved.data(),size);for(auto& fix:fixups){const auto address=reinterpret_cast<uintptr_t>(fix[1]==0x1019092c?&countObject:&backendObject);std::memcpy(code+fix[0],&address,4);}DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))std::abort();FlushInstructionCache(GetCurrentProcess(),code,4096);return code;};
 bodies[0]=load(0,transitionBackend0Size,transitionBackend0Offset,transitionBackend0Fixups);bodies[1]=load(1,transitionBackend1Size,transitionBackend1Offset,transitionBackend1Fixups);bodies[2]=load(2,transitionBackend2Size,transitionBackend2Offset,transitionBackend2Fixups);bodies[3]=load(3,transitionBackend3Size,transitionBackend3Offset,transitionBackend3Fixups);
 void* countTable[2]{};countTable[1]=reinterpret_cast<void*>(&getCount);void* counter=countTable;countObject=&counter;void* backendTable[273]{};backendTable[262]=reinterpret_cast<void*>(&onStage);backendTable[263]=reinterpret_cast<void*>(&on44);backendTable[272]=reinterpret_cast<void*>(&on45);backendTable[264]=reinterpret_cast<void*>(&onCombined);void* backend=backendTable;void* other=backendTable;secondBackend=&other;
 std::ofstream oracle("outputs/native-transition-backend.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-transition-backend.bin",std::ios::binary);
#endif
 unsigned cases=0;
#ifndef _M_IX86
 Services services;
#endif
 auto compare=[&](const StudioTransitionState72Candidate& state){std::array<uint32_t,28> record{};std::memcpy(record.data(),&state,sizeof(state));record[18]=static_cast<unsigned>(trace.size());for(unsigned i=0;i<trace.size();++i)record[19+i]=trace[i];record[27]=countReads;
#ifdef _M_IX86
 oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
 std::array<uint32_t,28> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(expected!=record)std::abort();
#endif
 if(!oracle)std::abort();++cases;};
 for(unsigned a=0;a<256;++a)for(unsigned b=0;b<256;++b){StudioTransitionState72Candidate state;state.flags44=static_cast<uint8_t>(a);state.flags45=static_cast<uint8_t>(b);state.value40=0xabcdef01;trace.clear();countReads=0;switched=false;
#ifdef _M_IX86
 backendObject=&backend;for(unsigned i=1;i<4;++i)reinterpret_cast<void(__cdecl*)(void*,unsigned)>(bodies[i])(&state,0);
#else
 studioTransitionFlag44Candidate(state,services);studioTransitionFlag45Candidate(state,services);studioTransitionCombinedFlagsCandidate(state,services);
#endif
 if(trace!=std::vector<uint32_t>{(a&16)?1u:0u,(b&8)?1u:0u,0xabcdef01,(a&1)?1u:0u,(a&128)?1u:0u,(b&4)?1u:0u})return 1;compare(state);}
 for(int n:{-1,0,1,3}){StudioTransitionState72Candidate state;stageCount=n;trace.clear();countReads=0;switched=false;
#ifdef _M_IX86
 backendObject=&backend;reinterpret_cast<void(__cdecl*)(void*,unsigned)>(bodies[0])(&state,0);
#else
 studioTransitionStagesCandidate(&state,services);
#endif
 if(countReads!=1||state.value40!=unsigned(n>0?n:0))return 2;compare(state);}
#ifdef _M_IX86
 for(auto* body:bodies)VirtualFree(body,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 3;
#endif
 std::printf("transition_backend_original_cases=%u passed\n",cases);return 0;}



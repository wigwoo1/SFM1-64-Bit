#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "transition_byte_dispatch_native_init.h"
#include "studio_transition_byte_dispatch_candidate.h"
struct State{std::array<uint32_t,18> words;};static_assert(sizeof(State)==72);
static State* expectedState;static unsigned used;static bool mutate;
static std::array<uint32_t,8> events;static std::array<uint8_t,8> operations;
static void observe(State* state,unsigned handler,unsigned arg){if(state!=expectedState||used+2>events.size())std::abort();events[used++]=handler;events[used++]=arg;state->words[0]+=handler+arg;if(mutate&&used==2)operations[3]=0x9f;}
static void ordinary(State* p,unsigned a){observe(p,0,a);}static void packed0(State* p,unsigned a){observe(p,1,a);}static void packed1(State* p,unsigned a){observe(p,2,a);}
int main(){std::array<void(*)(State*,unsigned),13> basic;basic.fill(ordinary);std::array<void(*)(State*,unsigned),2> packed{packed0,packed1};
#ifdef _M_IX86
 std::array<unsigned char,transitionDispatchByteCount> verified{},actual{};std::ifstream artifact("outputs/transition-byte-dispatch-verified.bin",std::ios::binary);artifact.read(reinterpret_cast<char*>(verified.data()),verified.size());if(!artifact||artifact.peek()!=std::char_traits<char>::eof())return 1;
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);dll.seekg(transitionDispatchFileOffset);dll.read(reinterpret_cast<char*>(actual.data()),actual.size());if(!dll||actual!=verified)return 2;auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 3;std::memcpy(code,actual.data(),actual.size());for(auto& fix:transitionDispatchFixups){const auto target=reinterpret_cast<uintptr_t>(fix[1]==0x1016e13c?static_cast<void*>(basic.data()):static_cast<void*>(packed.data()));std::memcpy(code+fix[0],&target,4);}DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 4;FlushInstructionCache(GetCurrentProcess(),code,4096);auto native=reinterpret_cast<void(__thiscall*)(void*,int,unsigned,int)>(code);std::ofstream oracle("outputs/native-transition-byte-dispatch.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-transition-byte-dispatch.bin",std::ios::binary);
#endif
 unsigned cases=0;for(unsigned opcode=0;opcode<256;++opcode){if(!(opcode<13||(opcode>=128&&opcode<=159)))continue;for(int n:{-1,0,1,2})for(bool change:{false,true})for(unsigned kind:{0u,2u}){
 std::array<State,3> states{};for(auto& state:states)state.words.fill(99);expectedState=&states[kind];operations.fill(0xcc);operations[2]=static_cast<uint8_t>(opcode);operations[3]=0;used=0;events.fill(0);mutate=change;
#ifdef _M_IX86
 std::array<uint32_t,48> owner{};owner[0x58/4]=reinterpret_cast<uintptr_t>(states.data());owner[0xb8/4]=reinterpret_cast<uintptr_t>(operations.data());native(owner.data(),static_cast<int>(kind),2,n);
#else
 studioTransitionByteDispatchCandidate(expectedState,operations.data()+2,n,basic,packed);
#endif
 if(used!=unsigned(n>0?n*2:0))return 5;if(n>0){const unsigned expectedHandler=opcode<13?0u:((opcode>>4)&1)+1;const unsigned expectedArg=opcode<13?0u:(opcode&15);if(events[0]!=expectedHandler||events[1]!=expectedArg)return 6;}if(n==2&&(events[2]!=(change?2u:0u)||events[3]!=(change?15u:0u)))return 7;
 std::array<uint32_t,65> record{};std::memcpy(record.data(),states.data(),sizeof(states));std::memcpy(record.data()+54,events.data(),sizeof(events));std::memcpy(record.data()+62,operations.data(),operations.size());record[64]=used;
#ifdef _M_IX86
 oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
 std::array<uint32_t,65> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(record!=expected)return 8;
#endif
 if(!oracle)return 9;++cases;
 }}
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 10;
#endif
 std::printf("transition_dispatch_original_cases=%u passed\n",cases);return 0;}

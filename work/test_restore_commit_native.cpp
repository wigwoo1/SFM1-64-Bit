#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "restore_commit_native_init.h"
#include "studio_restore_commit_candidate.h"
static std::array<unsigned char,512> receiver;
static std::array<uint32_t,64> block;
static std::array<uint32_t,32> events;
static bool mutateCallbacks;
#ifndef _M_IX86
static StudioRestoreCommitStateCandidate* liveState;
#endif
static unsigned eventCount,cursor,remaining;static bool queued;
static uint32_t load(unsigned offset){uint32_t v;std::memcpy(&v,receiver.data()+offset,4);return v;}
static void store(unsigned offset,uint32_t v){std::memcpy(receiver.data()+offset,&v,4);}
static void event(unsigned a,unsigned b){if(eventCount+2>events.size())std::abort();events[eventCount++]=a;events[eventCount++]=b;}
static void mutation(unsigned kind){if(!mutateCallbacks)return;
#ifdef _M_IX86
 if(kind==1){receiver[0x135]=1;store(0x138,99);}
 if(kind==2){store(0x138,123);receiver[0x13c]=1;receiver[0x13d]=1;}
 if(kind==3){receiver[0x13e]=1;receiver[0x13f]=1;store(0x140,0xabc00055);}
#else
 if(kind==1){liveState->dirty135=1;liveState->value138=99;}
 if(kind==2){liveState->value138=123;liveState->dirty13c=1;liveState->enabled13d=1;}
 if(kind==3){liveState->dirty13e=1;liveState->enabled13f=1;liveState->cached140=0xabc00055;}
#endif
}
static HRESULT __stdcall direct(void*,unsigned key,unsigned value){event(key,value);if(key==14)mutation(3);return E_FAIL;}
#ifdef _M_IX86
static std::array<uint32_t,8> command;
static void __fastcall apply134(void* p,void*,unsigned v){if(p!=receiver.data()||v!=3)std::abort();event(1,v);mutation(1);}
static void __fastcall apply135(void* p,void*,unsigned v){if(p!=receiver.data()||v!=1)std::abort();event(2,v);mutation(2);}
static void* __cdecl getter(){event(900,load(0x140));return command.data();}
static void __fastcall refill(void*,void*){std::abort();}
#endif
struct Services{StudioRestoreCommitStateCandidate& state;
 void apply134(unsigned v){event(1,v);mutation(1);}void apply135(unsigned v){event(2,v);mutation(2);}
 void submit(unsigned key,unsigned value){event(900,state.cached140);if(queued){if(remaining<3)std::abort();block[cursor++]=1;block[cursor++]=key;block[cursor++]=value;remaining-=3;}else direct(nullptr,key,value);}
};
int main(){
#ifdef _M_IX86
 std::array<unsigned char,restoreCommitByteCount> verified{},actual{};std::ifstream artifact("outputs/restore-commit-verified-bytes.bin",std::ios::binary);artifact.read(reinterpret_cast<char*>(verified.data()),verified.size());if(!artifact||artifact.peek()!=std::char_traits<char>::eof())return 1;
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);dll.seekg(restoreCommitFileOffset);dll.read(reinterpret_cast<char*>(actual.data()),actual.size());if(!dll||actual!=verified)return 2;
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 3;std::memcpy(code,actual.data(),actual.size());
 for(auto& call:restoreCommitCalls){void* target=nullptr;switch(call[1]){case 0x1002e490:target=reinterpret_cast<void*>(&apply134);break;case 0x1002e410:target=reinterpret_cast<void*>(&apply135);break;case 0x1000df50:target=reinterpret_cast<void*>(&getter);break;case 0x10002da0:target=reinterpret_cast<void*>(&refill);break;default:return 4;}const uint32_t delta=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(target)-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&delta,4);}
 DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 5;FlushInstructionCache(GetCurrentProcess(),code,4096);auto native=reinterpret_cast<void(__thiscall*)(void*)>(code);void* table[58]{};table[57]=reinterpret_cast<void*>(&direct);void* device=table;std::ofstream oracle("outputs/native-restore-commit.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-restore-commit.bin",std::ios::binary);
#endif
 unsigned cases=0;for(bool change:{false,true})for(unsigned dirty=0;dirty<16;++dirty)for(unsigned enabled=0;enabled<4;++enabled)for(unsigned cached=0;cached<16;++cached)for(bool queue:{false,true}){
 mutateCallbacks=change;receiver.fill(0xa5);receiver[0x134]=static_cast<unsigned char>(dirty&1);receiver[0x135]=static_cast<unsigned char>(dirty&2);receiver[0x13c]=static_cast<unsigned char>(dirty&4);receiver[0x13e]=static_cast<unsigned char>(dirty&8);receiver[0x13d]=static_cast<unsigned char>(enabled&1);receiver[0x13f]=static_cast<unsigned char>(enabled&2);store(0x138,0x87654321);store(0x140,0xabcd0000|cached);block.fill(99);events.fill(0);eventCount=cursor=0;remaining=64;queued=queue;
#ifdef _M_IX86
 command.fill(0);command[0]=reinterpret_cast<uintptr_t>(&device);command[4]=queue?1:0;command[6]=reinterpret_cast<uintptr_t>(block.data());command[7]=64;native(receiver.data());cursor=(command[6]-reinterpret_cast<uintptr_t>(block.data()))/4;remaining=command[7];
#else
 StudioRestoreCommitStateCandidate state{receiver[0x134],receiver[0x135],load(0x138),receiver[0x13c],receiver[0x13d],receiver[0x13e],receiver[0x13f],load(0x140)};liveState=&state;Services services{state};studioRestoreCommitCandidate(state,services);store(0x140,state.cached140);store(0x138,state.value138);receiver[0x134]=state.dirty134;receiver[0x135]=state.dirty135;receiver[0x13c]=state.dirty13c;receiver[0x13d]=state.enabled13d;receiver[0x13e]=state.dirty13e;receiver[0x13f]=state.enabled13f;
#endif
 if(change&&(dirty&1)){if(receiver[0x135]!=1||load(0x138)!=123||receiver[0x13c]!=1||receiver[0x13d]!=1)return 9;if(queue){if(block[0]!=1||block[1]!=14||block[2]!=123)return 10;}else{bool found=false;for(unsigned i=0;i+1<eventCount;i+=2)if(events[i]==14&&events[i+1]==123)found=true;if(!found||receiver[0x13e]!=1||receiver[0x13f]!=1)return 11;}}
 std::array<uint32_t,227> record{};std::memcpy(record.data(),receiver.data(),receiver.size());std::memcpy(record.data()+128,block.data(),sizeof(block));std::memcpy(record.data()+192,events.data(),sizeof(events));record[224]=eventCount;record[225]=cursor;record[226]=remaining;
#ifdef _M_IX86
 oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
 std::array<uint32_t,227> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(record!=expected)return 6;
#endif
 if(!oracle)return 7;++cases;
 }
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 8;
#endif
 std::printf("restore_commit_original_cases=%u passed\n",cases);return 0;
}

#define NOMINMAX
#include <windows.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "engine_projection_pair_unlink_candidate.h"
static uint16_t sourceHeads[3],otherHeads[4];static unsigned sourceCalls,otherCalls;
#ifndef _WIN64
#include "engine_projection_pair_unlink_native_init.h"
uint16_t* __cdecl sourceHead(uint32_t key){++sourceCalls;return sourceHeads+uint16_t(key);}
uint16_t* __cdecl otherHead(uint32_t key){++otherCalls;return otherHeads+key;}
struct RawEntry{uint16_t a,b,previous,next;};
struct RawPool{uint32_t storage;int32_t capacity;uint32_t growth;uint16_t head,tail,free,count,fresh,maximum;uint32_t cache;};
struct RawContainer{RawPool opposite,current;uint32_t source,other;};
static_assert(sizeof(RawPool)==28&&sizeof(RawContainer)==64&&sizeof(RawEntry)==8);
#endif
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("oracle path");
#ifdef _WIN64
 std::ifstream oracle(argv[1],std::ios::binary);
#else
 std::ofstream oracle(argv[1],std::ios::binary);auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,projectionPairUnlinkAllocationSize,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)throw std::runtime_error("allocate");initializeProjectionPairUnlinkNative(code);DWORD old;if(!VirtualProtect(code,projectionPairUnlinkAllocationSize,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protect");FlushInstructionCache(GetCurrentProcess(),code,projectionPairUnlinkAllocationSize);using Remove=void(__thiscall*)(void*,uint32_t);auto remove=reinterpret_cast<Remove>(code);
#endif
 unsigned cases=0;for(unsigned length=0;length<=64;++length){std::vector<EngineShadowPoolLinkCandidate> current(length),opposite(length);std::vector<EngineProjectionPairPayloadCandidate> payload(length);for(auto& h:sourceHeads)h=0xffff;for(auto& h:otherHeads)h=0xffff;
 for(unsigned i=0;i<length;++i){uint16_t j=uint16_t(length-1-i);payload[i]={uint16_t(i%4),j};current[i]={0xffff,sourceHeads[i%3]};if(sourceHeads[i%3]!=0xffff)current[sourceHeads[i%3]].previous=uint16_t(i);sourceHeads[i%3]=uint16_t(i);opposite[j]={0xffff,otherHeads[i%4]};if(otherHeads[i%4]!=0xffff)opposite[otherHeads[i%4]].previous=j;otherHeads[i%4]=j;}
 EngineProjectionPairMapCandidate map;map.current={current.data(),int32_t(length),0xffff,0xffff,uint16_t(length),uint16_t(length?length-1:0)};map.opposite={opposite.data(),int32_t(length),0xffff,0xffff,uint16_t(length),uint16_t(length?length-1:0)};map.payload=payload.data();
#ifndef _WIN64
 std::vector<RawEntry> rawCurrent(length),rawOpposite(length);for(unsigned i=0;i<length;++i){rawCurrent[i]={payload[i].oppositeKey,payload[i].oppositeIndex,current[i].previous,current[i].next};rawOpposite[i]={0xa5a5,0xa5a5,opposite[i].previous,opposite[i].next};}
 auto pool=[&](RawEntry* entries){return RawPool{reinterpret_cast<uint32_t>(entries),int32_t(length),0,0xffff,0xffff,0xffff,uint16_t(length),0,uint16_t(length?length-1:0),0};};RawContainer container{pool(rawOpposite.data()),pool(rawCurrent.data()),reinterpret_cast<uint32_t>(&sourceHead),reinterpret_cast<uint32_t>(&otherHead)};
#endif
 unsigned order[3]={1,0,2};for(unsigned group:order){sourceCalls=otherCalls=0;
#ifdef _WIN64
 engineProjectionPairUnlinkCandidate(map,group|0xabcd0000u,[&](uint32_t key){++sourceCalls;return sourceHeads+uint16_t(key);},[&](uint32_t key){++otherCalls;return otherHeads+key;});
#else
 remove(&container,group|0xabcd0000u);
#endif
 uint32_t state[273]{};unsigned at=0;for(auto h:sourceHeads)state[at++]=h;for(auto h:otherHeads)state[at++]=h;
#ifdef _WIN64
 for(auto* p:{&map.current,&map.opposite}){state[at++]=p->head;state[at++]=p->tail;state[at++]=p->count;state[at++]=p==&map.current?map.currentFree:map.oppositeFree;}
 for(unsigned i=0;i<64;++i){state[at++]=i<length?current[i].previous:0;state[at++]=i<length?current[i].next:0;state[at++]=i<length?opposite[i].previous:0;state[at++]=i<length?opposite[i].next:0;}
#else
 for(auto* p:{&container.current,&container.opposite}){state[at++]=p->head;state[at++]=p->tail;state[at++]=p->count;state[at++]=p->free;}
 for(unsigned i=0;i<64;++i){state[at++]=i<length?rawCurrent[i].previous:0;state[at++]=i<length?rawCurrent[i].next:0;state[at++]=i<length?rawOpposite[i].previous:0;state[at++]=i<length?rawOpposite[i].next:0;if(i<length&&(rawCurrent[i].a!=payload[i].oppositeKey||rawCurrent[i].b!=payload[i].oppositeIndex||rawOpposite[i].a!=0xa5a5||rawOpposite[i].b!=0xa5a5))throw std::runtime_error("payload changed");}
#endif
 state[at++]=sourceCalls;state[at++]=otherCalls;if(at!=273)throw std::runtime_error("record size");
#ifdef _WIN64
 uint32_t expected[273];if(!oracle.read(reinterpret_cast<char*>(expected),sizeof(expected))||std::memcmp(state,expected,sizeof(state)))throw std::runtime_error("native pair mismatch case "+std::to_string(cases));
#else
 oracle.write(reinterpret_cast<char*>(state),sizeof(state));
#endif
 ++cases;}}
#ifdef _WIN64
 if(oracle.peek()!=std::char_traits<char>::eof())throw std::runtime_error("extra oracle");
#else
 oracle.flush();if(!oracle)throw std::runtime_error("write");VirtualFree(code,0,MEM_RELEASE);
#endif
 std::cout<<"pair_cases="<<cases<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}

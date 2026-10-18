#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>
#include "studio_decal_chain_release_candidate.h"
#include "studio_decal_index_unlink_candidate.h"
#include "studio_decal_resource_release_candidate.h"
static std::vector<uint32_t> events;
static void check(bool v){if(!v)throw std::runtime_error("fixture contract");}
#ifndef _WIN64
#include "studio_decal_release_native_init.h"
#include "studio_decal_index_unlink_native_init.h"
using IndexUnlink=void(__thiscall*)(void*,uint32_t);static IndexUnlink nativeUnlink;
static uint32_t owner[22],outer[5];
static void __fastcall associations(void* receiver,void*,void* node){check(receiver==owner&&node==outer);events.push_back(1);}
static void __fastcall unlinkIndex(void* receiver,void*,uint32_t index){check(receiver==owner+2);events.push_back(0x10000+index);nativeUnlink(receiver,index);}
static void __fastcall remove24(void*,void*){events.push_back(24);}
static void __fastcall remove48(void*,void*){events.push_back(48);}
static void __fastcall freeBuffer(void*,void*,void* buffer){events.push_back(uint32_t(buffer));}
static void __fastcall releaseArray(void* buffer,void*,uint32_t kind){check(uint32_t(buffer)==outer[1]&&kind==3);events.push_back(3);}
static bool __fastcall classify(void* pool,void*,void* node,void* frontier){check(pool==owner+9&&node==outer&&frontier==owner+17&&owner[21]==1);events.push_back(4);return false;}
static uint32_t __cdecl identity(){events.push_back(42);return 42;}
static void __fastcall contention(void*,void*,uint32_t,int){throw std::runtime_error("contention");}
static uint32_t read32(const unsigned char* p){uint32_t v;std::memcpy(&v,p,4);return v;}
static uint16_t read16(const unsigned char* p){uint16_t v;std::memcpy(&v,p,2);return v;}
static void put32(unsigned char* p,uint32_t v){std::memcpy(p,&v,4);}
static void put16(unsigned char* p,uint16_t v){std::memcpy(p,&v,2);}
#endif
int main(int argc,char**argv){try{check(argc==2);unsigned cases=0;
#ifdef _WIN64
std::ifstream file(argv[1],std::ios::binary);
#else
std::ofstream file(argv[1],std::ios::binary);auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,8192,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code);uint32_t allocatorTable[6]={0,0,0,0,0,uint32_t(&freeBuffer)};uint32_t allocatorObject=uint32_t(allocatorTable);uint32_t allocatorPointer=uint32_t(&allocatorObject);
initializeStudioDecalReleaseNative(code,uint32_t(&associations),uint32_t(&unlinkIndex),uint32_t(&remove24),uint32_t(&remove48),uint32_t(&releaseArray),uint32_t(&classify),3,uint32_t(&allocatorPointer),uint32_t(&identity),uint32_t(&contention));std::memcpy(code+1024,studioDecalIndexUnlinkNativeBytes,sizeof(studioDecalIndexUnlinkNativeBytes));nativeUnlink=reinterpret_cast<IndexUnlink>(code+1024);DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old));FlushInstructionCache(GetCurrentProcess(),code,4096);using Release=void(__thiscall*)(void*,void*);auto release=reinterpret_cast<Release>(code);
#endif
for(unsigned length=1;length<=64;++length)for(unsigned mask=0;mask<8;++mask){events.clear();std::vector<uint32_t> result;
#ifdef _WIN64
std::vector<StudioDecalChainNodeCandidate> nodes(length);for(unsigned i=0;i<length;++i){auto& n=nodes[i];n.next=i+1<length?uint16_t(i+1):uint16_t(0xffff);n.contribution=uint16_t(i+1);n.array={0x1000+i*3+1,7,(mask&1)?-1:0};n.list48={0x1000+i*3+2,8,(mask&2)?-1:0};n.list24={0x1000+i*3+3,9,(mask&4)?-1:0};n.arrayCount=10;n.arrayCache=123;}
std::vector<EngineShadowPoolLinkCandidate> links(length);for(unsigned i=0;i<length;++i)links[i]={i?uint16_t(i-1):uint16_t(0xffff),nodes[i].next};EngineShadowPoolStateCandidate inner{links.data(),int32_t(length),0,uint16_t(length-1),uint16_t(length),uint16_t(length-1)};uint16_t freeHead=0xffff;uint32_t counter=3;unsigned char entries[32]={};StudioDecalPointerPoolCandidate pool;StudioDecalPointerNodeCandidate node;node.descriptor=123;node.entries=reinterpret_cast<uintptr_t>(entries);node.entryCount=1;node.previous=node.next=nullptr;pool.head=pool.tail=&node;pool.linkedCount=1;uint32_t thread=0,depth=0;
studioDecalResourceReleaseCandidate(pool,&node,[&](auto*){events.push_back(1);},[&](uint16_t head){studioDecalChainReleaseCandidate(head,freeHead,counter,[&](uint16_t i){return &nodes[i];},[&](uint16_t i){events.push_back(0x10000+i);studioDecalChainIndexUnlinkCandidate(inner,i,[&](uint16_t j){return &nodes[j];});},[&](auto&){events.push_back(24);},[&](auto&){events.push_back(48);},[&](uintptr_t p){events.push_back(uint32_t(p));});},[&](uintptr_t,int kind){check(kind==3);events.push_back(3);},[&]{events.push_back(42);thread=42;depth=1;},[&]{thread=depth=0;},[&](auto&,auto*){check(depth==1);events.push_back(4);return false;});
result={counter,freeHead,uint32_t(!pool.head&&!pool.tail),uint32_t(pool.linkedCount),uint32_t(pool.freeHead==&node),uint32_t(node.entries==0&&node.descriptor==123&&node.entryCount==1),thread,depth,inner.head,inner.tail,inner.count};
for(auto& n:nodes){result.insert(result.end(),{uint32_t(n.array.address),uint32_t(n.array.capacity),uint32_t(n.arrayCount),uint32_t(n.arrayCache),uint32_t(n.list48.address),uint32_t(n.list48.capacity),uint32_t(n.list24.address),uint32_t(n.list24.capacity),n.next,n.contribution,n.previous});}
#else
std::memset(owner,0,sizeof(owner));std::vector<unsigned char> nodes(length*84,0xa5);unsigned char entries[32]={};for(unsigned i=0;i<length;++i){auto* n=nodes.data()+i*84;put32(n+4,0x1000+i*3+1);put32(n+8,7);put32(n+12,(mask&1)?0xffffffff:0);put32(n+16,10);put32(n+20,123);put32(n+24,0x1000+i*3+2);put32(n+28,8);put32(n+32,(mask&2)?0xffffffff:0);put16(n+42,uint16_t(i+1));put32(n+52,0x1000+i*3+3);put32(n+56,9);put32(n+60,(mask&4)?0xffffffff:0);put16(n+80,i?uint16_t(i-1):uint16_t(0xffff));put16(n+82,i+1<length?uint16_t(i+1):uint16_t(0xffff));}
owner[2]=uint32_t(nodes.data());owner[3]=length;put16(reinterpret_cast<unsigned char*>(owner)+20,0);put16(reinterpret_cast<unsigned char*>(owner)+22,uint16_t(length-1));put16(reinterpret_cast<unsigned char*>(owner)+26,uint16_t(length));put16(reinterpret_cast<unsigned char*>(owner)+30,uint16_t(length-1));put16(reinterpret_cast<unsigned char*>(owner)+24,0xffff);outer[0]=123;outer[1]=uint32_t(entries);outer[2]=1;outer[3]=outer[4]=0;owner[12]=owner[13]=uint32_t(outer);owner[15]=1;*reinterpret_cast<uint32_t*>(code+4096)=3;release(owner,outer);
result={*reinterpret_cast<uint32_t*>(code+4096),read16(reinterpret_cast<unsigned char*>(owner)+24),uint32_t(!owner[12]&&!owner[13]),owner[15],uint32_t(owner[14]==uint32_t(outer)),uint32_t(outer[1]==0&&outer[0]==123&&outer[2]==1),owner[20],owner[21],read16(reinterpret_cast<unsigned char*>(owner)+20),read16(reinterpret_cast<unsigned char*>(owner)+22),read16(reinterpret_cast<unsigned char*>(owner)+26)};for(unsigned i=0;i<length;++i){auto* n=nodes.data()+i*84;result.insert(result.end(),{read32(n+4),read32(n+8),read32(n+16),read32(n+20),read32(n+24),read32(n+28),read32(n+52),read32(n+56),read16(n+82),read16(n+42),read16(n+80)});}
#endif
result.push_back(uint32_t(events.size()));result.insert(result.end(),events.begin(),events.end());
#ifdef _WIN64
std::vector<uint32_t> expected(result.size());check(bool(file.read(reinterpret_cast<char*>(expected.data()),expected.size()*4)));if(expected!=result)throw std::runtime_error("native populated-chain mismatch at "+std::to_string(cases));
#else
file.write(reinterpret_cast<char*>(result.data()),result.size()*4);
#endif
++cases;}
#ifdef _WIN64
check(file.peek()==std::char_traits<char>::eof());
#else
file.flush();check(bool(file));VirtualFree(code,0,MEM_RELEASE);
#endif
std::cout<<"chain_native_cases="<<cases<<"\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}


#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <array>
#include "model_render_record_release_candidate.h"
#include "model_render_record_resource_cleanup_candidate.h"
static uint32_t calls[6];
static void studioObserve(uint32_t v){calls[0]=calls[0]*10+1;calls[1]=v;}
static void shadowObserve(uint32_t v){calls[0]=calls[0]*10+2;calls[2]=v;}
static void resourceObserve(uint32_t v,int a,int b,int c){calls[0]=calls[0]*10+3;calls[3]=v;calls[4]=uint32_t(a|b);calls[5]=uint32_t(c);}
#ifndef _WIN64
#include "engine_model_render_release_native_init.h"
static void __fastcall studio(void*,void*,uint32_t v){studioObserve(v);}
static void __fastcall shadow(void*,void*,uint32_t v){shadowObserve(v);}
static void __fastcall resource(void*,void*,uint32_t v,int a,int b,int c){resourceObserve(v,a,b,c);}
static uint16_t read16(const unsigned char*p){uint16_t v;std::memcpy(&v,p,2);return v;}
static void write16(unsigned char*p,uint16_t v){std::memcpy(p,&v,2);}
struct RawPool{uint32_t storage;int32_t capacity;uint32_t growth;uint16_t head,tail,free,count,fresh,maximum;uint32_t cache;};
struct RawOwner{uint32_t vtable,secondary,resource;RawPool pool;};
static_assert(sizeof(RawPool)==28&&offsetof(RawOwner,pool)==12);
#endif
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("oracle path");unsigned cases=0;
#ifdef _WIN64
 std::ifstream f(argv[1],std::ios::binary);
#else
 std::ofstream f(argv[1],std::ios::binary);uint32_t studioTable[35]={},shadowTable[13]={},resourceTable[9]={};studioTable[34]=uint32_t(reinterpret_cast<uintptr_t>(&studio));shadowTable[12]=uint32_t(reinterpret_cast<uintptr_t>(&shadow));resourceTable[8]=uint32_t(reinterpret_cast<uintptr_t>(&resource));uint32_t studioObject=uint32_t(reinterpret_cast<uintptr_t>(studioTable)),shadowObject=uint32_t(reinterpret_cast<uintptr_t>(shadowTable)),resourceObject=uint32_t(reinterpret_cast<uintptr_t>(resourceTable));
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,modelRenderReleaseAllocationSize,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)throw std::runtime_error("allocate");initializeModelRenderReleaseNative(code,uint32_t(reinterpret_cast<uintptr_t>(&studioObject)),uint32_t(reinterpret_cast<uintptr_t>(&shadowObject)));DWORD old;if(!VirtualProtect(code,modelRenderReleaseAllocationSize,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protect");FlushInstructionCache(GetCurrentProcess(),code,modelRenderReleaseAllocationSize);using Release=void(__thiscall*)(void*,uint32_t);auto release=reinterpret_cast<Release>(code);
#endif
 for(unsigned length=1;length<=128;++length){
#ifdef _WIN64
  std::vector<EngineShadowPoolLinkCandidate> links(length);std::vector<ModelRenderRecordResourceCandidate> resources(length);for(unsigned i=0;i<length;++i){links[i]={i?uint16_t(i-1):uint16_t(0xffff),i+1<length?uint16_t(i+1):uint16_t(0xffff)};resources[i].resource=i%2?0x12340000u+i:0;}
  ModelRenderRecordAllocationCandidate state;state.pool={links.data(),int32_t(length),0,uint16_t(length-1),uint16_t(length),uint16_t(length-1)};state.freshCount=uint16_t(length);
#else
  std::vector<std::array<unsigned char,264>> records(length);for(unsigned i=0;i<length;++i){records[i].fill(0xa5);uint32_t value=0x56780000u+i,r=i%2?0x12340000u+i:0;std::memcpy(records[i].data()+8,&value,4);std::memcpy(records[i].data()+256,&r,4);write16(records[i].data()+260,i?uint16_t(i-1):uint16_t(0xffff));write16(records[i].data()+262,i+1<length?uint16_t(i+1):uint16_t(0xffff));}
  RawOwner owner{0,0,uint32_t(reinterpret_cast<uintptr_t>(&resourceObject)),{uint32_t(reinterpret_cast<uintptr_t>(records.data())),int32_t(length),0,0,uint16_t(length-1),0xffff,uint16_t(length),uint16_t(length),uint16_t(length-1),0}};
#endif
  for(unsigned phase=0;phase<2;++phase)for(unsigned index=phase;index<length;index+=2){std::memset(calls,0,sizeof(calls));uint32_t argument=0xabcd0000u|index;
#ifdef _WIN64
   modelRenderRecordReleaseCandidate(state,argument,[&](uint16_t i){return 0x56780000u+i;},studioObserve,shadowObserve,[&](uint32_t key){modelRenderRecordResourceCleanupCandidate(key,[&](uint16_t i)->ModelRenderRecordResourceCandidate&{return resources[i];},resourceObserve);});
#else
   release(&owner,argument);
#endif
   uint32_t snapshot[396]={};unsigned at=0;for(auto v:calls)snapshot[at++]=v;
#ifdef _WIN64
   for(auto v:{state.pool.head,state.pool.tail,state.pool.count,state.freeHead,state.freshCount,state.pool.maximumIndex})snapshot[at++]=v;
   for(unsigned i=0;i<128;++i){snapshot[at++]=i<length?links[i].previous:0;snapshot[at++]=i<length?links[i].next:0;snapshot[at++]=i<length?uint32_t(resources[i].resource):0;}
#else
   for(auto v:{owner.pool.head,owner.pool.tail,owner.pool.count,owner.pool.free,owner.pool.fresh,owner.pool.maximum})snapshot[at++]=v;
   for(unsigned i=0;i<128;++i){snapshot[at++]=i<length?read16(records[i].data()+260):0;snapshot[at++]=i<length?read16(records[i].data()+262):0;uint32_t r=0;if(i<length)std::memcpy(&r,records[i].data()+256,4);snapshot[at++]=r;}
#endif
   if(at!=396)throw std::runtime_error("snapshot size");
#ifdef _WIN64
   uint32_t expected[396];if(!f.read(reinterpret_cast<char*>(expected),sizeof(expected))||std::memcmp(snapshot,expected,sizeof(snapshot)))throw std::runtime_error("release mismatch "+std::to_string(cases));
#else
   f.write(reinterpret_cast<char*>(snapshot),sizeof(snapshot));
#endif
   ++cases;
  }
 }
#ifdef _WIN64
 if(f.peek()!=std::char_traits<char>::eof())throw std::runtime_error("trailing oracle");
#else
 f.flush();if(!f)throw std::runtime_error("write");VirtualFree(code,0,MEM_RELEASE);
#endif
 if(cases!=8256)throw std::runtime_error("case count");std::cout<<"release_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

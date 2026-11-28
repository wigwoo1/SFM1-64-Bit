#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>
#include "source_server_trace_annotation_candidate.h"
#include "physics_search_test_fixture.h"
static void require(bool v,const char*s){if(!v)throw std::runtime_error(s);}
struct Input{uint32_t handle=0,worldFlags=0,worldPresent=0;SourcePhysicsSearchTraceCandidate trace;};
static Input* active=nullptr;static std::vector<uint32_t> events;
static uintptr_t collideableIdentity=0,entityIdentity=0,worldIdentity=0;
struct Services{
 uintptr_t collideableEntity(uintptr_t e){require(e==collideableIdentity,"collideable argument");events.push_back(1);return entityIdentity;}
 uint32_t referenceHandle(uintptr_t e){require(e==entityIdentity,"entity argument");events.push_back(2);return active->handle;}
};
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
template<class T>static T read(const uint8_t*p,size_t n){T v;std::memcpy(&v,p+n,sizeof(v));return v;}
template<class T>static void write(uint8_t*p,size_t n,T v){std::memcpy(p+n,&v,sizeof(v));}
static uint32_t defaultSurface=0;
static SourcePhysicsSearchTraceCandidate readTrace(const uint8_t*p){
 SourcePhysicsSearchTraceCandidate t;std::memcpy(t.start.data(),p,12);std::memcpy(t.end.data(),p+12,12);std::memcpy(t.plane.normal.data(),p+24,12);t.plane.distance=read<float>(p,36);std::memcpy(t.planeTags.data(),p+40,4);t.fraction=read<float>(p,44);t.contents=read<uint32_t>(p,48);t.displacementFlags=read<uint16_t>(p,52);t.allSolid=p[54];t.startSolid=p[55];t.fractionLeftSolid=read<float>(p,56);auto name=read<uint32_t>(p,60);t.surfaceId=name==defaultSurface?0:uint64_t(name)+1;t.surfaceMetadata=read<uint32_t>(p,64);t.hitGroup=read<uint32_t>(p,68);t.physicsBone=read<uint16_t>(p,72);t.worldSurface=read<uint16_t>(p,74);t.entityId=read<uint32_t>(p,76);t.hitBox=read<uint32_t>(p,80);return t;
}
static void writeTrace(uint8_t*p,const SourcePhysicsSearchTraceCandidate&t){
 require(t.surfaceId<=0x100000000ull&&t.entityId<=UINT32_MAX,"legacy input identity");std::memcpy(p,t.start.data(),12);std::memcpy(p+12,t.end.data(),12);std::memcpy(p+24,t.plane.normal.data(),12);write(p,36,t.plane.distance);std::memcpy(p+40,t.planeTags.data(),4);write(p,44,t.fraction);write(p,48,t.contents);write(p,52,t.displacementFlags);p[54]=t.allSolid;p[55]=t.startSolid;write(p,56,t.fractionLeftSolid);write(p,60,t.surfaceId?uint32_t(t.surfaceId-1):defaultSurface);write(p,64,t.surfaceMetadata);write(p,68,t.hitGroup);write(p,72,t.physicsBone);write(p,74,t.worldSurface);write(p,76,uint32_t(t.entityId));write(p,80,t.hitBox);
}
static void* __fastcall entity(void* self,void*){require(uintptr_t(self)==collideableIdentity,"native collideable receiver");events.push_back(1);return reinterpret_cast<void*>(entityIdentity);}
static const uint32_t* __fastcall handle(void* self,void*){require(uintptr_t(self)==entityIdentity,"native entity receiver");events.push_back(2);return &active->handle;}
#endif
int main(int argc,char**argv){try{
 require(argc==4,"expected engine or -, input, output");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);require(bool(in)&&bool(out),"streams");
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1]);defaultSurface=*reinterpret_cast<uint32_t*>(image.base+0x402fe8);require(defaultSurface==0x1035067c,"default surface");image.seal();
 std::array<uintptr_t,1> collideableTable{uintptr_t(&entity)};std::array<uintptr_t,3> entityTable{};entityTable[2]=uintptr_t(&handle);uintptr_t c=uintptr_t(collideableTable.data()),e=uintptr_t(entityTable.data());uint32_t worldToken=0;
 collideableIdentity=uintptr_t(&c);entityIdentity=uintptr_t(&e);worldIdentity=uintptr_t(&worldToken);
 alignas(4)std::array<uint8_t,16> worldRecord{};*reinterpret_cast<uintptr_t*>(image.base+0x60d704)=uintptr_t(worldRecord.data());
 const auto managerBefore=std::array<uint32_t,3>{*reinterpret_cast<uint32_t*>(image.base+0x40b598),*reinterpret_cast<uint32_t*>(image.base+0x40b59c),*reinterpret_cast<uint32_t*>(image.base+0x40b5a0)};
#else
 collideableIdentity=0x100000001ull;entityIdentity=0x200000001ull;worldIdentity=0x300000001ull;
#endif
 size_t count=0;while(in.peek()!=std::char_traits<char>::eof()){
  Input q;supportGet(in,q.handle);supportGet(in,q.worldFlags);supportGet(in,q.worldPresent);searchReadTrace(in,q.trace);require(q.worldFlags<=255&&q.worldPresent<=1,"world binding fixture");active=&q;events.clear();auto result=q.trace;
#ifndef _WIN64
  worldRecord.fill(0);worldRecord[0]=uint8_t(q.worldFlags);write(worldRecord.data(),12,q.worldPresent?uint32_t(worldIdentity):0u);const auto beforeWorld=worldRecord;
  std::array<uint8_t,116> bytes;bytes.fill(0xa7);writeTrace(bytes.data()+16,q.trace);const auto before=bytes;
  reinterpret_cast<void(__thiscall*)(void*,void*,void*)>(0x1016d840)(nullptr,reinterpret_cast<void*>(collideableIdentity),bytes.data()+16);
  require(worldRecord==beforeWorld,"world record changed");for(size_t i=0;i<16;++i)require(bytes[i]==0xa7&&bytes[100+i]==0xa7,"trace guards");
  for(size_t i=0;i<84;++i)if(i<76||i>=84)require(bytes[16+i]==before[16+i],"annotation changed unrelated trace bytes");
  result=readTrace(bytes.data()+16);
  if(result.entityId==entityIdentity)result.entityId=0x200000001ull;else if(result.entityId==worldIdentity)result.entityId=0x300000001ull;
#else
  Services services;sourceServerTraceAnnotationCandidate(collideableIdentity,result,{uint8_t(q.worldFlags),q.worldPresent?worldIdentity:0},services);
#endif
  searchWriteTrace(out,result);supportPut(out,uint32_t(events.size()));for(uint32_t id:events)supportPut(out,id);++count;
 }
#ifndef _WIN64
 require(std::memcmp(image.base+0x40b598,managerBefore.data(),12)==0,"original manager changed");image.verify();
#endif
 require(count>0,"no queries");out.flush();require(bool(out),"output write");std::cout<<"PASS annotation cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

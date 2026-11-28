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
#include "source_server_owner_handle_candidate.h"
#include "source_server_pass_entity_filter_candidate.h"
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct Slot{uintptr_t entity=0;uint32_t serial=0;};
struct Query{uint32_t flags,ignoredMode,firstState,secondState,generation,indexMode;};
Query query{};std::vector<uint32_t> events;std::array<uint32_t,2> handles{};
std::vector<Slot> slots(65536);
uintptr_t candidate=0,ignored=0,baseA=0,baseB=0,unrelated=0;
void event(uint32_t value){events.push_back(value);}
uintptr_t resolved(unsigned index){return (query.flags&(index?8:2))?(index&&(query.flags&16)?baseA:(index?baseB:baseA)):0;}
struct Services{
 bool isStaticProp(uintptr_t entity){const unsigned id=entity==candidate?0:1;require(entity==candidate||entity==ignored,"classify entity");event(id?3:1);return (query.flags&(id?4:1))!=0;}
 uintptr_t baseEntity(uintptr_t entity){unsigned id=entity==candidate?0:1;event(id?4:2);return resolved(id);}
 uintptr_t ownerEntity(uintptr_t base){unsigned id=base==baseA?0:1;require(base==baseA||base==baseB,"owner base");event(id?6:5);return sourceServerOwnerHandleCandidate(handles[id],[&](uint16_t i){return slots.at(i);});}
};
void setup(){
 const std::array<uint16_t,2> indices=query.indexMode?std::array<uint16_t,2>{65535,65534}:std::array<uint16_t,2>{0,1};
 for(unsigned id=0;id<2;++id){const auto state=id?query.secondState:query.firstState;auto& slot=slots[indices[id]];slot.serial=query.generation;slot.entity=state==1?(id?baseA:baseB):state==2?(id?baseB:baseA):state==3?unrelated:0;
  if(state==4)slot.serial=query.generation^1u;
  if(state==5)slot.serial=query.generation|0x10000u;
  if(state==6)slot.entity=id?baseA:baseB;
  handles[id]=state==0?UINT32_MAX:(query.generation<<16)|indices[id];
 }
}
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
template<class T>T& at(uintptr_t p){return *reinterpret_cast<T*>(p);}
std::array<uintptr_t,6> handleTable{};std::array<uintptr_t,4> managerTable{};
std::array<uint8_t,0x1d0> entityA{},entityB{};
std::vector<std::array<uint32_t,4>> nativeSlots(65536);
uintptr_t manager=0;
bool __fastcall classify(void* self,void*,uintptr_t entity){require(reinterpret_cast<uintptr_t>(self)==manager,"manager receiver");Services service;return service.isStaticProp(entity);}
void* __fastcall resolve(void* self,void*){Services service;return reinterpret_cast<void*>(service.baseEntity(reinterpret_cast<uintptr_t>(self)));}
void observer(unsigned index,CONTEXT& c){require(index==0,"unexpected owner boundary");require(c.Ecx==baseA||c.Ecx==baseB,"owner receiver");event(c.Ecx==baseA?5:6);c.EFlags|=0x10000u;}
void bind(){
 for(unsigned id=0;id<2;++id){auto p=id?baseB:baseA;at<uint32_t>(p+0x1c0)=handles[id];}
 for(unsigned i:query.indexMode?std::array<unsigned,2>{65535,65534}:std::array<unsigned,2>{0,1}){nativeSlots[i][1]=uint32_t(slots[i].entity);nativeSlots[i][2]=slots[i].serial;}
}
#endif
int main(int argc,char** argv){try{
 require(argc==3,"expected server or - and output");std::ofstream output(argv[2],std::ios::binary);require(bool(output),"output");
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1],0x6de6c4);image.seal();
 uintptr_t table=reinterpret_cast<uintptr_t>(handleTable.data());std::array<uintptr_t,2> handleStorage{table,table};uintptr_t mt=reinterpret_cast<uintptr_t>(managerTable.data());
 candidate=reinterpret_cast<uintptr_t>(&handleStorage[0]);ignored=reinterpret_cast<uintptr_t>(&handleStorage[1]);baseA=reinterpret_cast<uintptr_t>(entityA.data());baseB=reinterpret_cast<uintptr_t>(entityB.data());uint32_t token=0;unrelated=reinterpret_cast<uintptr_t>(&token);manager=reinterpret_cast<uintptr_t>(&mt);
 handleTable[5]=reinterpret_cast<uintptr_t>(&resolve);managerTable[3]=reinterpret_cast<uintptr_t>(&classify);at<uintptr_t>(0x10635cb8)=manager;at<uintptr_t>(0x105a01e0)=reinterpret_cast<uintptr_t>(nativeSlots.data());
 PrivateX86BoundaryObserverFixture observe({0x10010090,0x100100bc,0x100100bd,0x100100be},observer);
#else
 candidate=uintptr_t(0x100000001ull);ignored=uintptr_t(0x200000001ull);baseA=uintptr_t(0x300000001ull);baseB=uintptr_t(0x400000001ull);unrelated=uintptr_t(0x500000001ull);
#endif
 uint32_t cases=0;
 for(uint32_t flags=0;flags<32;++flags)for(uint32_t mode=0;mode<3;++mode)for(uint32_t a=0;a<7;++a)for(uint32_t b=0;b<7;++b)for(uint32_t gen:{0u,1u,65534u,65535u})for(uint32_t ix=0;ix<2;++ix){
  query={flags,mode,a,b,gen,ix};setup();events.clear();bool answer;
#ifndef _WIN64
  bind();const auto beforeA=entityA,beforeB=entityB;observe.configure(true);
  answer=reinterpret_cast<bool(__cdecl*)(uintptr_t,uintptr_t)>(0x10267610)(candidate,mode==0?0:mode==1?candidate:ignored);
  observe.configure(false);require(entityA==beforeA&&entityB==beforeB,"native entity mutation");
#else
  Services services;answer=sourceServerPassEntityFilterCandidate(candidate,mode==0?0:mode==1?candidate:ignored,services);
#endif
  std::array<uint32_t,16> row{};std::memcpy(row.data(),&query,24);row[6]=answer;row[7]=uint32_t(events.size());require(events.size()<=6,"events size");for(size_t i=0;i<events.size();++i)row[8+i]=events[i];
  // Direct owner probes cover stale serial, sentinel, null slot and high index
  // even when PassServerEntityFilter returns before owner access.
  for(unsigned id=0;id<2;++id){uintptr_t owner;
#ifndef _WIN64
   owner=reinterpret_cast<uintptr_t(__thiscall*)(void*)>(0x10010090)(reinterpret_cast<void*>(id?baseB:baseA));
#else
   owner=sourceServerOwnerHandleCandidate(handles[id],[](uint16_t i){return slots.at(i);});
#endif
   row[14+id]=owner==0?0:owner==baseA?1:owner==baseB?2:owner==unrelated?3:99;require(row[14+id]!=99,"owner identity");
  }
  output.write(reinterpret_cast<const char*>(row.data()),sizeof(row));require(bool(output),"record write");++cases;
 }
#ifndef _WIN64
 image.verify();
#endif
 std::cout<<"{\"cases\":"<<cases<<",\"passed\":true}\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

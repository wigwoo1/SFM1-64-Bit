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
#include "source_server_simple_trace_filter_candidate.h"
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct Query{uint32_t flags,mask;int32_t group,target;};
Query query{};std::vector<uint32_t> events;
bool flag(unsigned bit){return (query.flags&(1u<<bit))!=0;}
void event(uint32_t id){events.push_back(id);}
struct Services{
 bool standardRules(uintptr_t,uint32_t){event(1);return flag(0);}
 bool passEntity(uintptr_t,uintptr_t){event(2);return flag(2);}
 bool isStaticProp(uintptr_t){event(3);return flag(3);}
 uintptr_t baseEntity(uintptr_t){event(4);return flag(4)?2:0;}
 bool entityAllows(uintptr_t,int32_t,uint32_t){event(5);return flag(5);}
 int32_t collisionGroup(uintptr_t){return query.target;}
 bool groupsAllow(int32_t,int32_t){event(6);return flag(6);}
 bool extraAllows(uintptr_t,uint32_t){event(7);return flag(8);}
};
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
template<class T>T& at(uintptr_t address){return *reinterpret_cast<T*>(address);}
std::array<uintptr_t,6> handleTable{};std::array<uintptr_t,18> entityTable{};
std::array<uintptr_t,4> managerTable{};std::array<uintptr_t,29> rulesTable{};
std::array<uint8_t,0x1e0> entity{};
uintptr_t handle=0,manager=0,rules=0,ignored=0;
bool __fastcall classify(void* self,void*,uintptr_t e){require(reinterpret_cast<uintptr_t>(self)==manager&&e==handle,"classification arguments");event(3);return flag(3);}
void* __fastcall resolve(void* self,void*){require(reinterpret_cast<uintptr_t>(self)==handle,"entity resolution arguments");event(4);return flag(4)?entity.data():nullptr;}
bool __fastcall allows(void* self,void*,int32_t group,uint32_t mask){require(self==entity.data()&&group==query.group&&mask==query.mask,"entity policy arguments");event(5);return flag(5);}
bool __fastcall groups(void* self,void*,int32_t a,int32_t b){require(reinterpret_cast<uintptr_t>(self)==rules&&a==query.group&&b==query.target,"group policy arguments");event(6);return flag(6);}
bool __cdecl extra(uintptr_t e,uint32_t mask){require(e==handle&&mask==query.mask,"extra predicate arguments");event(7);return flag(8);}
void boundary(unsigned slot,CONTEXT& c){using O=PrivateX86BoundaryObserverFixture;
 require(slot<2,"unexpected native boundary");
 require(at<uintptr_t>(c.Esp+4)==handle,"standard/pass candidate argument");
 if(slot==0){require(at<uint32_t>(c.Esp+8)==query.mask&&at<uint32_t>(c.Esp)==0x102677b4,"standard rule call site/arguments");event(1);O::returnCdecl(c,flag(0));}
 else{require(at<uintptr_t>(c.Esp+8)==ignored&&at<uint32_t>(c.Esp)==0x102677d1,"pass entity call site/arguments");event(2);O::returnCdecl(c,flag(2));}
}
bool native(){
 alignas(4)std::array<uintptr_t,4> filter={0x104e7390,flag(1)?ignored:0,0,flag(7)?reinterpret_cast<uintptr_t>(&extra):0};
 std::memcpy(&filter[2],&query.group,4);std::memcpy(entity.data()+0x1d4,&query.target,4);
 const auto before=filter;const auto beforeEntity=entity;
 const bool result=reinterpret_cast<bool(__thiscall*)(void*,uintptr_t,uint32_t)>(0x102677a0)(filter.data(),handle,query.mask);
 require(filter==before&&entity==beforeEntity,"filter/entity mutated");return result;
}
#endif
int main(int argc,char** argv){try{
 require(argc==3,"expected server or - and output");std::ofstream output(argv[2],std::ios::binary);require(bool(output),"output open");
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1],0x6de6c4);image.seal();
 uintptr_t handleVtable=reinterpret_cast<uintptr_t>(handleTable.data()),managerVtable=reinterpret_cast<uintptr_t>(managerTable.data()),rulesVtable=reinterpret_cast<uintptr_t>(rulesTable.data());
 handle=reinterpret_cast<uintptr_t>(&handleVtable);manager=reinterpret_cast<uintptr_t>(&managerVtable);rules=reinterpret_cast<uintptr_t>(&rulesVtable);ignored=handle+4;
 handleTable[5]=reinterpret_cast<uintptr_t>(&resolve);managerTable[3]=reinterpret_cast<uintptr_t>(&classify);rulesTable[28]=reinterpret_cast<uintptr_t>(&groups);entityTable[17]=reinterpret_cast<uintptr_t>(&allows);
 at<uintptr_t>(reinterpret_cast<uintptr_t>(entity.data()))=reinterpret_cast<uintptr_t>(entityTable.data());at<uintptr_t>(0x10635cb8)=manager;at<uintptr_t>(0x10636664)=rules;
 PrivateX86BoundaryObserverFixture observer({0x102676b0,0x10267610,0x102677a0,0x10267852},boundary);
 // Unused slots point at unreachable padding; only declared static services
 // are intercepted. The complete ShouldHitEntity body executes unchanged.
 observer.entries[2]=0x10267852;observer.entries[3]=0x10267853;observer.configure(true);
#endif
 uint32_t cases=0;
 for(uint32_t bits=0;bits<512;++bits)for(uint32_t mask:{0u,1u,0x200400bu,0xffffffffu})for(int32_t group:{-1,0,5}){
  query={bits,mask,group,group==5?-7:31};events.clear();bool result;
#ifndef _WIN64
  result=native();
#else
  Services services;result=sourceServerSimpleTraceFilterCandidate(uintptr_t(0x100000001ull),mask,flag(1)?uintptr_t(0x200000001ull):0,group,flag(7),services);
#endif
  std::array<uint32_t,14> row{};row[0]=bits;row[1]=mask;std::memcpy(&row[2],&query.group,4);std::memcpy(&row[3],&query.target,4);row[4]=result;row[5]=uint32_t(events.size());
  require(events.size()<=7,"event count");for(size_t i=0;i<events.size();++i)row[6+i]=events[i];
  output.write(reinterpret_cast<const char*>(row.data()),sizeof(row));require(bool(output),"output write");++cases;
 }
#ifndef _WIN64
 observer.configure(false);image.verify();
#endif
 std::cout<<"{\"cases\":"<<cases<<",\"passed\":true}\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

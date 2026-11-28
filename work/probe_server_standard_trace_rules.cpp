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
#include "source_server_standard_trace_rules_candidate.h"
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
struct Query{uint32_t flags,mask;int32_t model,solid;uint32_t movement,first,second;};
Query query{};std::vector<uint32_t> events;
void event(uint32_t id){events.push_back(id);}
constexpr int32_t indexValue=-19;
#ifdef _WIN64
struct Services{
 bool isStaticProp(uintptr_t){event(1);return (query.flags&1)!=0;}
 uintptr_t baseEntity(uintptr_t){event(2);return (query.flags&2)?uintptr_t(0x200000001ull):0;}
 int32_t solidType(uintptr_t){event(3);return query.solid;}
 int32_t modelIndex(uintptr_t){event(4);return indexValue;}
 uintptr_t modelForIndex(int32_t index){require(index==indexValue,"owned index");event(5);return uintptr_t(0x300000001ull);}
 int32_t modelType(uintptr_t){event(6);return query.model;}
 uint32_t modelFlag30(uintptr_t){event(7);return query.first;}
 uint32_t modelFlag34(uintptr_t){event(8);return query.second;}
 uint8_t moveType(uintptr_t){return uint8_t(query.movement);}
};
#endif
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
template<class T>T& at(uintptr_t p){return *reinterpret_cast<T*>(p);}
std::array<uintptr_t,6> handleTable{};std::array<uintptr_t,7> entityTable{};
std::array<uintptr_t,12> solidTable{};std::array<uintptr_t,4> managerTable{};std::array<uintptr_t,14> modelTable{};
std::array<uint8_t,0x180> entity{};uintptr_t handle=0,manager=0,modelInfo=0,model=0;
bool __fastcall classify(void* self,void*,uintptr_t candidate){require(reinterpret_cast<uintptr_t>(self)==manager&&candidate==handle,"manager arguments");event(1);return (query.flags&1)!=0;}
void* __fastcall resolve(void* self,void*){require(reinterpret_cast<uintptr_t>(self)==handle,"handle arguments");event(2);return (query.flags&2)?entity.data():nullptr;}
int32_t __fastcall solidType(void* self,void*){require(self==entity.data()+0x164,"solid arguments");event(3);return query.solid;}
int32_t __fastcall modelIndex(void* self,void*){require(self==entity.data(),"model index arguments");event(4);return indexValue;}
void* __fastcall modelForIndex(void* self,void*,int32_t index){require(reinterpret_cast<uintptr_t>(self)==modelInfo&&index==indexValue,"model lookup arguments");event(5);return reinterpret_cast<void*>(model);}
int32_t __fastcall modelType(void* self,void*,uintptr_t input){require(reinterpret_cast<uintptr_t>(self)==modelInfo&&input==model,"model type arguments");event(6);return query.model;}
uint32_t __fastcall first(void* self,void*,uintptr_t input){require(reinterpret_cast<uintptr_t>(self)==modelInfo&&input==model,"first model flag arguments");event(7);return query.first;}
uint32_t __fastcall second(void* self,void*,uintptr_t input){require(reinterpret_cast<uintptr_t>(self)==modelInfo&&input==model,"second model flag arguments");event(8);return query.second;}
bool native(){
 entity[0x156]=uint8_t(query.movement);const auto before=entity;
 bool result=reinterpret_cast<bool(__cdecl*)(uintptr_t,uint32_t)>(0x102676b0)(handle,query.mask);
 require(entity==before,"original changed entity");return result;
}
#endif
int main(int argc,char** argv){try{
 require(argc==3,"expected server or - and output");std::ofstream output(argv[2],std::ios::binary);require(bool(output),"output");
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1],0x6de6c4);image.seal();
 uintptr_t handleVtable=reinterpret_cast<uintptr_t>(handleTable.data()),managerVtable=reinterpret_cast<uintptr_t>(managerTable.data()),modelVtable=reinterpret_cast<uintptr_t>(modelTable.data());uint32_t modelToken=0;
 handle=reinterpret_cast<uintptr_t>(&handleVtable);manager=reinterpret_cast<uintptr_t>(&managerVtable);modelInfo=reinterpret_cast<uintptr_t>(&modelVtable);model=reinterpret_cast<uintptr_t>(&modelToken);
 handleTable[5]=reinterpret_cast<uintptr_t>(&resolve);entityTable[6]=reinterpret_cast<uintptr_t>(&modelIndex);solidTable[11]=reinterpret_cast<uintptr_t>(&solidType);managerTable[3]=reinterpret_cast<uintptr_t>(&classify);
 modelTable[1]=reinterpret_cast<uintptr_t>(&modelForIndex);modelTable[9]=reinterpret_cast<uintptr_t>(&modelType);modelTable[12]=reinterpret_cast<uintptr_t>(&first);modelTable[13]=reinterpret_cast<uintptr_t>(&second);
 at<uintptr_t>(reinterpret_cast<uintptr_t>(entity.data()))=reinterpret_cast<uintptr_t>(entityTable.data());at<uintptr_t>(reinterpret_cast<uintptr_t>(entity.data()+0x164))=reinterpret_cast<uintptr_t>(solidTable.data());
 at<uintptr_t>(0x10635cb8)=manager;at<uintptr_t>(0x10635cc8)=modelInfo;
#endif
 uint32_t cases=0;
 for(uint32_t flags=0;flags<4;++flags)for(int32_t type:{-1,0,1,2})for(int32_t solid:{-1,0,1,6,7})for(uint32_t move:{0u,6u,7u,8u,255u})
 for(uint32_t firstValue:{0u,1u,0x100u,0x101u})for(uint32_t secondValue:{0u,1u,0x100u,0x101u})for(uint32_t selector=0;selector<10;++selector){
  uint32_t mask=selector<8?((selector&1?2u:0)|(selector&2?0x4000u:0)|(selector&4?0x2000000u:0)):selector==8?0x200400bu:0xffffffffu;
  query={flags,mask,type,solid,move,firstValue,secondValue};events.clear();bool result;
#ifndef _WIN64
  result=native();
#else
  Services services;result=sourceServerStandardTraceRulesCandidate(uintptr_t(0x100000001ull),mask,services);
#endif
  std::array<uint32_t,18> row{};static_assert(sizeof(Query)==28,"query layout");std::memcpy(row.data(),&query,28);row[7]=result;row[8]=uint32_t(events.size());require(events.size()<=8,"event limit");for(size_t i=0;i<events.size();++i)row[9+i]=events[i];
  output.write(reinterpret_cast<const char*>(row.data()),sizeof(row));require(bool(output),"record write");++cases;
 }
#ifndef _WIN64
 image.verify();
#endif
 std::cout<<"{\"cases\":"<<cases<<",\"passed\":true}\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

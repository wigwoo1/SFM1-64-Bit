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
#include "source_server_composed_trace_filter_candidate.h"
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
// flags, ignore mode, type, solid, move, flag30, flag34, mask, group,
// policy bits (entity/group/extra), owner mode, serial mode.
std::array<uint32_t,12> q{};
std::vector<uint32_t> events;
uintptr_t candidate,skip,baseA,baseB;
struct Slot{uintptr_t entity;uint32_t serial;};
Slot slots[2]{};
uint32_t owners[2]{};
int32_t group(){int32_t v;std::memcpy(&v,&q[8],4);return v;}
struct Host {
 bool isStaticProp(uintptr_t e){require(e==candidate||e==skip,"identity");events.push_back(e==candidate?1:9);return q[0]&(e==candidate?1:4);}
 uintptr_t baseEntity(uintptr_t e){events.push_back(e==candidate?2:10);return q[0]&(e==candidate?2:8)?(e==candidate?baseA:baseB):0;}
 int32_t solidType(uintptr_t e){require(e==baseA,"solid identity");events.push_back(3);return int32_t(q[3]);}
 int32_t modelIndex(uintptr_t e){require(e==baseA,"index identity");events.push_back(4);return -19;}
 uintptr_t modelForIndex(int32_t i){require(i==-19,"index");events.push_back(5);return 1;}
 int32_t modelType(uintptr_t){events.push_back(6);return int32_t(q[2]);}
 uint32_t modelFlag30(uintptr_t){events.push_back(7);return q[5];}
 uint32_t modelFlag34(uintptr_t){events.push_back(8);return q[6];}
 uint8_t moveType(uintptr_t){return uint8_t(q[4]);}
 uintptr_t ownerEntity(uintptr_t e){return sourceServerOwnerHandleCandidate(owners[e==baseA?0:1],[](uint16_t i){require(i<2,"slot index");return slots[i];});}
 bool entityAllows(uintptr_t e,int32_t g,uint32_t mask){require(e==baseA&&g==group()&&mask==q[7],"entity args");events.push_back(11);return q[9]&1;}
 int32_t collisionGroup(uintptr_t){return -7;}
 bool groupsAllow(int32_t g,int32_t target){require(g==group()&&target==-7,"group args");events.push_back(12);return q[9]&2;}
 bool extraAllows(uintptr_t e,uint32_t m){require(e==candidate&&m==q[7],"extra args");events.push_back(13);return q[9]&4;}
};
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
template<class T>T& at(uintptr_t p){return *reinterpret_cast<T*>(p);}
std::array<uintptr_t,6> htable{};std::array<uintptr_t,18> etable{};
std::array<uintptr_t,12> stable{};std::array<uintptr_t,4> mtable{};
std::array<uintptr_t,14> modeltable{};std::array<uintptr_t,29> gtable{};
uintptr_t manager,models;uint32_t token;
bool __fastcall classify(void* self,void*,uintptr_t e){require(uintptr_t(self)==manager,"manager");return Host{}.isStaticProp(e);}
void* __fastcall resolve(void* self,void*){return reinterpret_cast<void*>(Host{}.baseEntity(uintptr_t(self)));}
int32_t __fastcall solid(void* self,void*){require(uintptr_t(self)==baseA+0x164,"solid");return Host{}.solidType(baseA);}
int32_t __fastcall index(void* self,void*){return Host{}.modelIndex(uintptr_t(self));}
void* __fastcall lookup(void* self,void*,int32_t i){require(uintptr_t(self)==models,"models");Host{}.modelForIndex(i);return &token;}
int32_t __fastcall type(void* self,void*,void* m){require(uintptr_t(self)==models&&m==&token,"type");return Host{}.modelType(1);}
uint32_t __fastcall first(void* self,void*,void* m){require(uintptr_t(self)==models&&m==&token,"first");return Host{}.modelFlag30(1);}
uint32_t __fastcall second(void* self,void*,void* m){require(uintptr_t(self)==models&&m==&token,"second");return Host{}.modelFlag34(1);}
bool __fastcall entityRule(void* self,void*,int32_t g,uint32_t mask){return Host{}.entityAllows(uintptr_t(self),g,mask);}
bool __fastcall groupRule(void*,void*,int32_t a,int32_t b){return Host{}.groupsAllow(a,b);}
bool __cdecl extra(uintptr_t e,uint32_t m){return Host{}.extraAllows(e,m);}
#endif
int main(int argc,char** argv){try{
 require(argc==4,"expected server or -, input, output");std::ifstream input(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);
 require(bool(input)&&bool(out),"file access");
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1],0x6de6c4);image.seal();
 std::array<uint8_t,0x1e0> a{},b{};std::array<std::array<uint32_t,4>,2> nativeSlots{};
 uintptr_t hv=uintptr_t(htable.data());std::array<uintptr_t,2> hs{hv,hv};
 uintptr_t mv=uintptr_t(mtable.data()),modv=uintptr_t(modeltable.data()),gv=uintptr_t(gtable.data());
 candidate=uintptr_t(&hs[0]);skip=uintptr_t(&hs[1]);baseA=uintptr_t(a.data());baseB=uintptr_t(b.data());
 manager=uintptr_t(&mv);models=uintptr_t(&modv);
 htable[5]=uintptr_t(&resolve);etable[6]=uintptr_t(&index);etable[17]=uintptr_t(&entityRule);stable[11]=uintptr_t(&solid);
 mtable[3]=uintptr_t(&classify);modeltable[1]=uintptr_t(&lookup);modeltable[9]=uintptr_t(&type);modeltable[12]=uintptr_t(&first);modeltable[13]=uintptr_t(&second);gtable[28]=uintptr_t(&groupRule);
 at<uintptr_t>(baseA)=uintptr_t(etable.data());at<uintptr_t>(baseA+0x164)=uintptr_t(stable.data());
 at<uintptr_t>(0x10635cb8)=manager;at<uintptr_t>(0x10635cc8)=models;at<uintptr_t>(0x10636664)=uintptr_t(&gv);
 at<uintptr_t>(0x105a01e0)=uintptr_t(nativeSlots.data());
#else
 candidate=0x100000001ull;skip=0x200000001ull;baseA=0x300000001ull;baseB=0x400000001ull;
#endif
 unsigned count=0;
 while(input.read(reinterpret_cast<char*>(q.data()),sizeof(q))){
  slots[0]={q[10]==1?baseB:0,1};slots[1]={q[10]==2?baseA:0,1};
  owners[0]=0x10000;owners[1]=0x10001;
  if(q[11]==1){slots[0].serial=0;slots[1].serial=0;}
  if(q[11]==2){owners[0]=UINT32_MAX;owners[1]=UINT32_MAX;}
  events.clear();bool answer;
#ifndef _WIN64
  a[0x156]=uint8_t(q[4]);at<uint32_t>(baseA+0x1c0)=owners[0];at<uint32_t>(baseB+0x1c0)=owners[1];
  for(unsigned i=0;i<2;++i){nativeSlots[i][1]=uint32_t(slots[i].entity);nativeSlots[i][2]=slots[i].serial;}
  at<int32_t>(baseA+0x1d4)=-7;
  std::array<uintptr_t,4> filter{0x104e7390,q[1]==0?0:q[1]==1?candidate:skip,0,q[0]&16?uintptr_t(&extra):0};
  std::memcpy(&filter[2],&q[8],4);const auto aa=a,bb=b;
  answer=reinterpret_cast<bool(__thiscall*)(void*,uintptr_t,uint32_t)>(0x102677a0)(filter.data(),candidate,q[7]);
  require(a==aa&&b==bb,"storage changed");
#else
  Host host;answer=sourceServerComposedTraceFilterCandidate(candidate,q[7],q[1]==0?0:q[1]==1?candidate:skip,group(),q[0]&16,host);
#endif
  require(events.size()<=24,"trace too large");std::array<uint32_t,26> result{};result[0]=answer;result[1]=uint32_t(events.size());
  for(size_t i=0;i<events.size();++i)result[2+i]=events[i];
  out.write(reinterpret_cast<char*>(result.data()),sizeof(result));++count;
 }
 require(input.eof(),"truncated query");
#ifndef _WIN64
 image.verify();
#endif
 std::cout<<count;return 0;
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}

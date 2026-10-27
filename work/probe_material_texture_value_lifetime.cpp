// Reference investigation only. Executes a private original CMaterialVar setter
// with explicit texture/material reference spies, never a live SFM process.
#include <array>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <cstdlib>
#define NOMINMAX
#include <windows.h>
#include "material_construction_native_fixture.h"
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct TextureFixture:MaterialConstructionNativeFixture {
 struct Resource {uintptr_t methods=0;unsigned id=0,refs=1;};
 std::array<uintptr_t,25>methods{};std::array<Resource,3>resources{};
 std::string events;
 static TextureFixture&self(){return *static_cast<TextureFixture*>(active);}
 static Resource&checked(void*p){auto&f=self();for(auto&r:f.resources)if(p==&r)return r;throw std::runtime_error("unknown fixture resource");}
 static void event(char op,unsigned id=0){auto&f=self();f.events+=op;if(id)f.events+=std::to_string(id);f.events+=' ';}
 static void* __fastcall queue(void*,void*){++self().queues;event('Q');return nullptr;}
 static void __fastcall retain(void*p,void*){auto&r=checked(p);++r.refs;event('A',r.id);}
 static void __fastcall releaseTexture(void*p,void*){auto&r=checked(p);need(r.refs>1,"owned texture reference");--r.refs;event('R',r.id);}
 static void __fastcall releaseMaterial(void*p,void*){auto&r=checked(p);need(r.refs>1,"owned material reference");--r.refs;event('M',r.id);}
 static void __fastcall removeUnreferenced(void*p,void*){auto&r=checked(p);need(r.refs>=1,"external fixture reference remains");event('D',r.id);}
 static void __fastcall freeTracked(void*a,void*b,void*p){event('F');MaterialConstructionNativeFixture::free(a,b,p);}
 explicit TextureFixture(const char*path):MaterialConstructionNativeFixture(path,true){
  services[144]=reinterpret_cast<uintptr_t>(&queue);allocators[5]=reinterpret_cast<uintptr_t>(&freeTracked);
  methods.fill(reinterpret_cast<uintptr_t>(&unexpected));methods[10]=reinterpret_cast<uintptr_t>(&retain);methods[11]=reinterpret_cast<uintptr_t>(&releaseTexture);methods[13]=reinterpret_cast<uintptr_t>(&releaseMaterial);methods[24]=reinterpret_cast<uintptr_t>(&removeUnreferenced);
  for(unsigned i=0;i<resources.size();++i)resources[i]={reinterpret_cast<uintptr_t>(methods.data()),i+1,1};
 }
 uintptr_t pointer(unsigned token){if(token==UINT32_MAX)return UINT32_MAX;if(!token)return 0;need(token<=resources.size(),"bounded resource token");return reinterpret_cast<uintptr_t>(&resources[token-1]);}
 unsigned token(uint32_t address){if(address==UINT32_MAX||!address)return address;for(auto&r:resources)if(address==reinterpret_cast<uintptr_t>(&r))return r.id;throw std::runtime_error("unexpected published resource");}
};
static uint32_t word(const void*p,size_t at){uint32_t n;std::memcpy(&n,static_cast<const uint8_t*>(p)+at,4);return n;}
static void word(void*p,size_t at,uint32_t n){std::memcpy(static_cast<uint8_t*>(p)+at,&n,4);}
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==4,"private original x86 reference, not the editor");
 need(argc==4&&!std::ifstream(argv[3]).good(),"image queries fresh-output");SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
 std::ifstream query(argv[2]);std::ofstream out(argv[3],std::ios::binary);need(bool(query)&&bool(out),"reference IO");TextureFixture fixture(argv[1]);
 unsigned count=0,steps=0,totalAllocated=0,totalFreed=0;std::string line;
 while(std::getline(query,line)){
  std::istringstream row(line);unsigned id,type,old,metadata,remove,components;need(bool(row>>id>>type>>old>>metadata>>remove>>components),"case header");need(id==count&&type<=8&&type!=5&&metadata<=240&&!(metadata&15)&&remove<2&&components>=1&&components<=4,"case scope");
  fixture.begin(true);fixture.events.clear();for(auto&r:fixture.resources){need(r.refs==1,"previous resource balance");}
  fixture.image.base[0x109a60]=uint8_t(remove);std::array<uint8_t,40>object{};
  using Empty=void*(__thiscall*)(void*,void*,const char*);using Scalar=void*(__thiscall*)(void*,void*,const char*,uint32_t);
  using Text=void*(__thiscall*)(void*,void*,const char*,const char*);using Vector=void*(__thiscall*)(void*,void*,const char*,const void*,int);
  using Matrix=void(__thiscall*)(void*,const void*);using Set=void(__thiscall*)(void*,void*);using Clear=void(__thiscall*)(void*);
  const std::array<uint32_t,4>values{{0x80000000,0x3f800000,0x7fc12345,0x40000000}};
  if(type==0||type==4)reinterpret_cast<Scalar>(type==0?0x10017910:0x100179b0)(object.data(),nullptr,nullptr,values[1]);
  else if(type==1)reinterpret_cast<Text>(0x10017780)(object.data(),nullptr,nullptr,"retained old string");
  else if(type==2)reinterpret_cast<Vector>(0x10017850)(object.data(),nullptr,nullptr,values.data(),int(components));
  else{
   reinterpret_cast<Empty>(0x10017a50)(object.data(),nullptr,nullptr);
   if(type==7){std::array<uint32_t,16>matrix{};for(unsigned i=0;i<16;++i)matrix[i]=(i%5==0)?0x3f800000:0;reinterpret_cast<Matrix>(0x10019f70)(object.data(),matrix.data());}
   else if(type==3||type==8){need((type==3&&(old==1||old==2||old==UINT32_MAX))||(type==8&&(old==0||old==3)),"retained seed resource domain");object[28]=uint8_t(type);word(object.data(),36,uint32_t(fixture.pointer(old)));if(old&&old!=UINT32_MAX)++fixture.resources[old-1].refs;}
  }
  object[28]=uint8_t((object[28]&15)|metadata);const uint32_t namePointer=word(object.data(),4),symbols=word(object.data(),28)&0xffffff00u;
  unsigned step=0,next=0;
  auto emit=[&](unsigned input){
   need((word(object.data(),28)&0xffffff00u)==symbols&&word(object.data(),4)==namePointer&&word(object.data(),32)==0,"name symbol/queue metadata and owner preserved");
   out<<"STEP\t"<<id<<'\t'<<step++<<'\t'<<input<<'\t'<<unsigned(object[28]);for(unsigned at:{8u,12u,16u,20u,24u})out<<'\t'<<word(object.data(),at);
   out<<'\t'<<fixture.token(word(object.data(),36));for(const auto&r:fixture.resources)out<<'\t'<<r.refs;out<<'\t'<<fixture.allocations<<'\t'<<fixture.releases<<'\t'<<fixture.events<<'\n';++steps;
  };
  while(row>>next){need(next==1||next==2||next==UINT32_MAX,"nonnull texture or environment sentinel only");fixture.events.clear();reinterpret_cast<Set>(0x10019710)(object.data(),reinterpret_cast<void*>(fixture.pointer(next)));emit(next);fixture.image.verify();}
  need(row.eof()&&step>0,"complete case sequence");fixture.events.clear();reinterpret_cast<Clear>(0x1001a0a0)(object.data());emit(0);fixture.image.verify();
  for(const auto&r:fixture.resources)need(r.refs==1,"all texture/material references returned to external baseline");need(fixture.allocations==fixture.releases,"all old strings/matrices freed");totalAllocated+=fixture.allocations;totalFreed+=fixture.releases;++count;
 }
 need(query.eof()&&count>0,"complete query file");out.flush();need(bool(out),"complete reference output");fixture.image.verify();std::cout<<"PASS texture_value_reference cases="<<count<<" transitions="<<steps<<" allocations="<<totalAllocated<<" releases="<<totalFreed<<" code=UNCHANGED_RX queue=NULL owner=ABSENT resources=SPIES SFM=NOT_OPENED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

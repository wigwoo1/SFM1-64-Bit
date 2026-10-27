#include <array>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <optional>
#include <stdexcept>
#include <cstring>
#include <cstdlib>
#ifdef SFM64_NATIVE_TEXTURE_LOOKUP
#define NOMINMAX
#include <windows.h>
#include "material_construction_native_fixture.h"
#else
#include "material_texture_lookup_candidate.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static uint32_t read(std::istream&i){uint32_t n;need(bool(i.read(reinterpret_cast<char*>(&n),4)),"word input");return n;}
static void put(std::ostream&o,uint32_t n){o.write(reinterpret_cast<const char*>(&n),4);}
static std::string text(std::istream&i){auto n=read(i);need(n<=4095,"bounded input text");std::string s(n,'\0');need(bool(i.read(s.data(),n)),"input text");return s;}
static void text(std::ostream&o,const std::string&s){put(o,uint32_t(s.size()));o.write(s.data(),s.size());}
static std::string prefix(std::string s){s.resize(s.find('\0')==std::string::npos?s.size():s.find('\0'));return s;}
struct Route {uint32_t firstError=0,secondError=0,complain=0,groupPresent=0;int32_t flags=0;std::string name,group;};
static std::string findEvent(unsigned call,const std::string&name,const std::optional<std::string>&group,int32_t flags){return "F"+std::to_string(call)+" "+name+" "+(group?"G:"+*group:"NULL")+" "+std::to_string(flags)+"\n";}
#ifdef SFM64_NATIVE_TEXTURE_LOOKUP
struct LookupFixture:MaterialConstructionNativeFixture {
 struct Resource {uintptr_t methods=0;unsigned id=0;};
 std::array<uintptr_t,84>systemMethods{};std::array<uintptr_t,16>resourceMethods{};std::array<uintptr_t,8>managerMethods{};
 uintptr_t system=0,manager=0;std::array<Resource,2>resources{};Route current;unsigned finds=0,warns=0;std::string trace;
 std::array<uint8_t,0xa4>actualManager{};std::array<uint8_t,16>primary{},alias{};
 static LookupFixture&self(){return *static_cast<LookupFixture*>(active);}
 static void* __fastcall find(void*p,void*,const char*name,const char*group,int flags){auto&f=self();need(p==&f.manager&&name&&f.finds<2,"controlled manager request");++f.finds;f.trace+=findEvent(f.finds,name,group?std::optional<std::string>(group):std::nullopt,flags);return &f.resources[f.finds-1];}
 static bool __fastcall error(void*p,void*){auto&f=self();for(auto&r:f.resources)if(p==&r){bool error=r.id==1?f.current.firstError!=0:f.current.secondError!=0;f.trace+="E"+std::to_string(r.id)+" "+std::to_string(error)+"\n";return error;}throw std::runtime_error("unknown texture result");}
 static void __cdecl warning(const char*format,const char*name){auto&f=self();need(format==reinterpret_cast<const char*>(0x10096664)&&name,"original warning format");++f.warns;f.trace+="W "+std::string(name)+"\n";}
 static void set(void*p,size_t at,uintptr_t n){uint32_t w=uint32_t(n);std::memcpy(static_cast<uint8_t*>(p)+at,&w,4);}
 static void shortWord(void*p,size_t at,uint16_t n){std::memcpy(static_cast<uint8_t*>(p)+at,&n,2);}
 explicit LookupFixture(const char*path):MaterialConstructionNativeFixture(path,true){
  systemMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));systemMethods[83]=0x1000b7e0;system=reinterpret_cast<uintptr_t>(systemMethods.data());
  managerMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));managerMethods[7]=reinterpret_cast<uintptr_t>(&find);manager=reinterpret_cast<uintptr_t>(managerMethods.data());
  resourceMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));resourceMethods[15]=reinterpret_cast<uintptr_t>(&error);for(unsigned i=0;i<2;++i)resources[i]={reinterpret_cast<uintptr_t>(resourceMethods.data()),i+1};word(0x951b8,reinterpret_cast<uintptr_t>(&warning));word(0x106184,0);
 }
 unsigned token(void*p){for(auto&r:resources)if(p==&r)return r.id;need(!p,"unrecognized original result");return 0;}
 void seed(unsigned at,std::array<uint8_t,16>&record,const std::string&key,uintptr_t value){
  // Controlled one-node cache DATA. Native constructor/comparator/tree lookup
  // execute; insertion, allocation and loader behavior are NOT replaced/proven.
  record.fill(0);for(unsigned offset:{0u,2u,4u})shortWord(record.data(),offset,0xffff);set(record.data(),8,reinterpret_cast<uintptr_t>(key.c_str()));set(record.data(),12,value);
  set(actualManager.data(),at+4,reinterpret_cast<uintptr_t>(record.data()));set(actualManager.data(),at+8,1);shortWord(actualManager.data(),at+16,0);shortWord(actualManager.data(),at+18,1);shortWord(actualManager.data(),at+22,0);set(actualManager.data(),at+24,reinterpret_cast<uintptr_t>(record.data()));
 }
 void route(std::ostream&out,unsigned ci,const Route&q){current=q;finds=warns=0;trace.clear();word(0x101a84,reinterpret_cast<uintptr_t>(&manager));using Find=void*(__thiscall*)(void*,const char*,const char*,bool,int);auto p=reinterpret_cast<Find>(0x1000b7e0)(&system,q.name.c_str(),q.groupPresent?q.group.c_str():nullptr,q.complain!=0,q.flags);put(out,ci);put(out,token(p));put(out,warns);put(out,finds==2);text(out,trace);image.verify();}
 void cache(std::ostream&out,unsigned ci,unsigned mode,const std::string&name,const std::string&seedKey){
  need(mode<=4,"cache mode");current={};trace.clear();using Construct=void*(__thiscall*)(void*);reinterpret_cast<Construct>(0x10041e00)(actualManager.data());word(0x101a84,reinterpret_cast<uintptr_t>(actualManager.data()));const std::string target="fixture/actual-target";seed(4,primary,mode==1?target:seedKey,reinterpret_cast<uintptr_t>(&resources[0]));if(mode==1)seed(0x20,alias,seedKey,reinterpret_cast<uintptr_t>(target.c_str()));
  const char*input=mode==3?nullptr:name.c_str();std::array<char,260>key{};
  if(input){using Key=char*(__cdecl*)(const char*,char*,int);need(reinterpret_cast<Key>(0x1002c390)(input,key.data(),260)==key.data(),"original cache-key destination identity");}
  using Cached=void*(__thiscall*)(void*,const char*);auto direct=reinterpret_cast<Cached>(0x10042080)(actualManager.data(),input);unsigned managerToken=UINT32_MAX,publicToken=UINT32_MAX;
  if(mode<=1||mode==3){using Find=void*(__thiscall*)(void*,const char*,const char*,int);managerToken=token(reinterpret_cast<Find>(0x10042350)(actualManager.data(),input,"ignored group",int(ci)));}
  if(mode<=1){using Find=void*(__thiscall*)(void*,const char*,const char*,bool,int);publicToken=token(reinterpret_cast<Find>(0x1000b7e0)(&system,input,nullptr,true,int(ci)));}
  put(out,ci);text(out,key.data());put(out,token(direct));put(out,managerToken);put(out,publicToken);image.verify();
 }
};
#else
static int safety(){unsigned checks=0;auto check=[&](bool b){need(b,"owned lookup safety");++checks;};std::string key="keep";check(!materialTextureCacheKeyCandidate(std::string(260,'x'),key)&&key=="keep");check(!materialTextureCacheKeyCandidate("\xff",key)&&key=="keep");check(materialTextureCacheKeyCandidate("A.HDR",key)&&key=="a.hdr");check(materialTextureCacheKeyCandidate("A.PNG",key)&&key=="a");
 MaterialTextureLookupRequestCandidate q;q.name="fixture/a";MaterialTextureLookupServicesCandidate s;check(!materialTextureLookupCandidate(q,s));s.find=[](const auto&,const auto&,int32_t)->std::optional<uintptr_t>{return 1;};check(!materialTextureLookupCandidate(q,s));s.isError=[](uintptr_t)->std::optional<bool>{return false;};check(materialTextureLookupCandidate(q,s)->texture==1);
 auto good=s;for(auto token:{uintptr_t(0),UINTPTR_MAX}){s.find=[=](const auto&,const auto&,int32_t)->std::optional<uintptr_t>{return token;};check(!materialTextureLookupCandidate(q,s));}s=good;s.isError=[](uintptr_t)->std::optional<bool>{return {};};check(!materialTextureLookupCandidate(q,s));s=good;s.find=[](const auto&,const auto&,int32_t)->std::optional<uintptr_t>{throw std::bad_alloc();};check(!materialTextureLookupCandidate(q,s));s=good;s.isError=[](uintptr_t)->std::optional<bool>{throw std::runtime_error("service");};check(!materialTextureLookupCandidate(q,s));s=good;
 q.name.reset();check(!materialTextureLookupCandidate(q,s));for(auto name:{std::string(260,'x'),std::string("\xff",1),std::string(4096,'a')}){q.name=name;check(!materialTextureLookupCandidate(q,s));}q.name="fixture/a";q.group=std::string(256,'x');check(!materialTextureLookupCandidate(q,s));q.group.reset();check(materialTextureLookupCandidate(q,s)->texture==1);
 q.name="_rt_FullFrameFB1";unsigned calls=0;s.find=[&](const auto&,const auto&,int32_t)->std::optional<uintptr_t>{if(++calls==2)return {};return 1;};s.isError=[](uintptr_t)->std::optional<bool>{return true;};check(!materialTextureLookupCandidate(q,s)&&calls==2);
 constexpr uintptr_t high=0x6244000000000001ull;s.find=[=](const auto&,const auto&,int32_t)->std::optional<uintptr_t>{return high;};s.isError=[](uintptr_t)->std::optional<bool>{return false;};check(materialTextureLookupCandidate(q,s)->texture==high);
 {MaterialTextureLookupRequestCandidate request;request.name="_rt_FullFrameFB1";request.group="original";request.flags=7;request.complain=255;MaterialTextureLookupServicesCandidate service;unsigned finds=0,errors=0;service.find=[&](const auto&name,const auto&group,int32_t flags)->std::optional<uintptr_t>{++finds;need(name==(finds==1?"_rt_FullFrameFB1":"_rt_FullScreen")&&group==std::optional<std::string>("original")&&flags==7,"frozen call arguments");request.name="mutated";request.group.reset();request.flags=-3;request.complain=0;service.find={};service.isError={};return finds;};service.isError=[&](uintptr_t)->std::optional<bool>{++errors;return true;};auto result=materialTextureLookupCandidate(request,service);check(result&&result->texture==2&&result->redirected&&result->warning&&finds==2&&errors==2);}
 {auto service=good;unsigned finds=0;service.find=[&](const auto&,const auto&,int32_t)->std::optional<uintptr_t>{++finds;return 1;};service.isError={};MaterialTextureLookupRequestCandidate request;request.name="a";check(!materialTextureLookupCandidate(request,service)&&finds==0);}
 {auto service=good;MaterialTextureLookupRequestCandidate request;request.name="a";bool nested=false;service.find=[&](const auto&,const auto&,int32_t)->std::optional<uintptr_t>{auto inner=materialTextureLookupCandidate(request,good);nested=inner&&inner->texture==1;return 2;};auto result=materialTextureLookupCandidate(request,service);check(result&&result->texture==2&&nested);}
 {auto service=good;service.find=[](const auto&,const auto&,int32_t)->std::optional<uintptr_t>{return {};};MaterialTextureLookupRequestCandidate request;request.name="a";check(!materialTextureLookupCandidate(request,service));}
 std::cout<<"PASS texture_lookup_safety="<<checks<<" token_bits=64 ownership=BORROWED editor=NOT_CONNECTED\n";return 0;}
#endif
int main(int argc,char**argv){try{
#ifndef SFM64_NATIVE_TEXTURE_LOOKUP
 static_assert(sizeof(void*)==8,"owned native x64");if(argc==1)return safety();
#else
 static_assert(sizeof(void*)==4,"separate private original x86 reference");SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
#endif
 need(argc==5&&!std::ifstream(argv[4]).good(),"mode image query fresh-output");const std::string mode=argv[1];need(mode=="--keys"||mode=="--routes","probe mode");std::ifstream in(argv[3],std::ios::binary);std::ofstream out(argv[4],std::ios::binary);need(bool(in)&&bool(out),"probe IO");
#ifdef SFM64_NATIVE_TEXTURE_LOOKUP
 LookupFixture f(argv[2]);
#else
 need(!std::ifstream(argv[2]).good(),"owned original runtime absent");
#endif
 auto count=read(in);need(count&&count<100000,"bounded case count");put(out,count);
 for(unsigned ci=0;ci<count;++ci){
  if(mode=="--keys"){auto kind=read(in);auto name=text(in),seed=text(in);need(kind<=4&&prefix(name).size()<=259&&seed.size()<=259,"cache case extent");
#ifdef SFM64_NATIVE_TEXTURE_LOOKUP
   f.cache(out,ci,kind,name,seed);
#else
   std::string key;if(kind!=3)need(materialTextureCacheKeyCandidate(name,key),"owned key admission");unsigned found=kind<=1&&key==seed?1:0;put(out,ci);text(out,key);put(out,found);put(out,kind<=1?found:kind==3?0:UINT32_MAX);put(out,kind<=1?found:UINT32_MAX);
#endif
  }else{Route q;q.firstError=read(in);q.secondError=read(in);q.complain=read(in);q.groupPresent=read(in);uint32_t flags=read(in);std::memcpy(&q.flags,&flags,4);q.name=text(in);q.group=text(in);need(q.firstError<2&&q.secondError<2&&q.complain<=255&&q.groupPresent<2&&prefix(q.name).size()<=259&&prefix(q.group).size()<=255,"route case extent");
#ifdef SFM64_NATIVE_TEXTURE_LOOKUP
   f.route(out,ci,q);
#else
   MaterialTextureLookupRequestCandidate request;request.name=q.name;request.group=q.groupPresent?std::optional<std::string>(q.group):std::nullopt;request.complain=uint8_t(q.complain);request.flags=q.flags;unsigned calls=0;std::string trace;constexpr uintptr_t base=0x6244000000000000ull;MaterialTextureLookupServicesCandidate service;service.find=[&](const auto&name,const auto&group,int32_t input)->std::optional<uintptr_t>{trace+=findEvent(++calls,name,group,input);return base+calls;};service.isError=[&](uintptr_t token)->std::optional<bool>{auto id=unsigned(token-base);need(id==1||id==2,"owned borrowed identity");bool e=id==1?q.firstError!=0:q.secondError!=0;trace+="E"+std::to_string(id)+" "+std::to_string(e)+"\n";return e;};auto result=materialTextureLookupCandidate(request,service);need(bool(result),"owned route admission");if(result->warning)trace+="W "+(result->redirected?std::string("_rt_FullScreen"):prefix(q.name))+"\n";put(out,ci);put(out,uint32_t(result->texture-base));put(out,result->warning);put(out,result->redirected);text(out,trace);
#endif
  }
 }
 need(in.peek()==std::char_traits<char>::eof(),"complete query");out.flush();need(bool(out),"complete output");
#ifdef SFM64_NATIVE_TEXTURE_LOOKUP
 f.image.verify();need(f.allocations==f.releases&&f.allocations==0,"cache/wrapper calls allocate and release no resources");
#endif
 std::cout<<"PASS texture_lookup_cases="<<count<<" mode="<<mode<<" resource_ownership=BORROWED loader=NOT_EXECUTED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

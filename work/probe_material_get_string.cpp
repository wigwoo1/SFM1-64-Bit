#include <array>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <optional>
#ifdef SFM64_NATIVE_GET_STRING
#define NOMINMAX
#include <windows.h>
#include <float.h>
#include "material_construction_native_fixture.h"
#else
#include "material_get_string_candidate.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static uint32_t read(std::istream&i){uint32_t v;need(bool(i.read(reinterpret_cast<char*>(&v),4)),"word");return v;}
static void put(std::ostream&o,uint32_t v){o.write(reinterpret_cast<const char*>(&v),4);}
static std::string text(std::istream&i){auto n=read(i);need(n<=4095,"text length");std::string s(n,'\0');need(bool(i.read(s.data(),n)),"text bytes");return s;}
static void text(std::ostream&o,const std::string&s){put(o,uint32_t(s.size()));o.write(s.data(),s.size());}
#ifdef SFM64_NATIVE_GET_STRING
struct GetFixture:MaterialConstructionNativeFixture{
 std::array<uintptr_t,14>resourceMethods{};uintptr_t resource=0;std::string resourceText;unsigned names=0,refs=0;
 static GetFixture&self(){return *static_cast<GetFixture*>(active);}
 static const char*__fastcall name(void*,void*){++self().names;return self().resourceText.c_str();}
 static void __fastcall release(void*,void*){need(self().refs==1,"single retained fixture resource");--self().refs;}
 explicit GetFixture(const char*p):MaterialConstructionNativeFixture(p,true){reinterpret_cast<void(__cdecl*)()>(0x1007d4e7)();resourceMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));resourceMethods[0]=reinterpret_cast<uintptr_t>(&name);resourceMethods[11]=resourceMethods[13]=reinterpret_cast<uintptr_t>(&release);resource=reinterpret_cast<uintptr_t>(resourceMethods.data());}
};
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){std::cerr<<"getter fault "<<std::hex<<p->ExceptionRecord->ExceptionCode<<" at "<<p->ExceptionRecord->ExceptionAddress<<'\n';return EXCEPTION_CONTINUE_SEARCH;}
#endif
int main(int argc,char**argv){try{
#ifndef SFM64_NATIVE_GET_STRING
 if(argc==2&&std::string(argv[1])=="--self-test"){
  MaterialGetStringInputCandidate in;std::string out="keep";unsigned checks=0;auto check=[&](bool b){need(b,"getter safety");++checks;};
  in.type=16;check(!materialGetStringCandidate(in,out)&&out=="keep");in.type=2;in.components=5;check(!materialGetStringCandidate(in,out)&&out=="keep");
  in.components=4;in.type=1;in.text=std::string(4096,'x');check(!materialGetStringCandidate(in,out)&&out=="keep");in.text.clear();in.type=3;check(!materialGetStringCandidate(in,out)&&out=="keep");
  in.resourceName=std::string(512,'x');check(!materialGetStringCandidate(in,out)&&out=="keep");in.resourceName="resource";check(materialGetStringCandidate(in,out)&&out=="resource");in.environmentTexture=true;check(materialGetStringCandidate(in,out)&&out=="env_cubemap");
  in={};in.type=1;in.text=std::string("a\0ignored",9);check(materialGetStringCandidate(in,out)&&out=="a");in.type=4;in.integer=uint32_t(-7);check(materialGetStringCandidate(in,out)&&out=="-7");in.type=6;check(materialGetStringCandidate(in,out)&&out=="<UNDEFINED>");
  std::cout<<"PASS get_string_safety_checks="<<checks<<'\n';return 0;
 }
#endif
 need(argc==4&&!std::ifstream(argv[3]).good(),"image query fresh-output");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"IO");
#ifdef SFM64_NATIVE_GET_STRING
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);AddVectoredExceptionHandler(1,&fault);GetFixture fixture(argv[1]);unsigned control=0,oldControl=0;_controlfp_s(&oldControl,0,0);_controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);_mm_setcsr(0x1f80);unsigned totalAlloc=0,totalFree=0;
#else
 need(!std::ifstream(argv[1]).good(),"owned original runtime absent");
#endif
 auto count=read(in);need(count&&count<20000,"case count");put(out,count);
 for(unsigned ci=0;ci<count;++ci){auto type=read(in),components=read(in),integer=read(in),environment=read(in),metadata=read(in);std::array<uint32_t,16>numbers;for(auto&n:numbers)n=read(in);auto inputText=text(in),resourceText=text(in);need(type<=15&&components<=4&&environment<2&&metadata<2&&resourceText.size()<=511,"case scope");
  const unsigned storage=type==1?1u:(type==0||type==2||type==4||type==7||type==8||(type==3&&!environment))?2u:0u;std::string snapshot,after;unsigned warnings=type==5||type>=9?1u:0u;unsigned names=(type==8||(type==3&&!environment))?1u:0u;
#ifdef SFM64_NATIVE_GET_STRING
  fixture.begin(true);_mm_setcsr(0x1f80);fixture.warnings=fixture.names=fixture.refs=0;fixture.resourceText=resourceText;
  std::array<uint8_t,40>object{},scratch{};auto word=[](void*p,size_t at,uint32_t v){std::memcpy(static_cast<uint8_t*>(p)+at,&v,4);};
  if(type==0||type==4){using Ctor=void*(__thiscall*)(void*,void*,const char*,uint32_t);reinterpret_cast<Ctor>(type==0?0x10017910:0x100179b0)(object.data(),nullptr,nullptr,type==0?numbers[0]:integer);}
  else if(type==1){using Ctor=void*(__thiscall*)(void*,void*,const char*,const char*);reinterpret_cast<Ctor>(0x10017780)(object.data(),nullptr,nullptr,inputText.c_str());}
  else if(type==2){using Ctor=void*(__thiscall*)(void*,void*,const char*,const void*,int);reinterpret_cast<Ctor>(0x10017850)(object.data(),nullptr,nullptr,numbers.data(),int(std::max(components,1u)));if(!components)object[28]=2;}
  else{using Ctor=void*(__thiscall*)(void*,void*,const char*);reinterpret_cast<Ctor>(0x10017a50)(object.data(),nullptr,nullptr);
   if(type==7){using Set=void(__thiscall*)(void*,const void*);reinterpret_cast<Set>(0x10019f70)(object.data(),numbers.data());}
   else{object[28]=uint8_t(type);if(type==3||type==8){word(object.data(),36,type==3&&environment?UINT32_MAX:uint32_t(reinterpret_cast<uintptr_t>(&fixture.resource)));if(!(type==3&&environment))fixture.refs=1;}}
  }
  object[28]|=metadata?0x80:0;using IntCtor=void*(__thiscall*)(void*,void*,const char*,uint32_t);reinterpret_cast<IntCtor>(0x100179b0)(scratch.data(),nullptr,nullptr,42);
  const auto before=object;fixture.queues=0;using Get=const char*(__thiscall*)(void*);auto pointer=reinterpret_cast<Get>(0x10017de0)(object.data());need(pointer!=nullptr,"native nonnull getter");snapshot=pointer;
  need((storage==2)==(pointer==reinterpret_cast<char*>(fixture.image.base+0x109860)),"borrowed scratch identity");
  if(storage==1){uint32_t member;std::memcpy(&member,object.data()+4,4);need(pointer==reinterpret_cast<char*>(uintptr_t(member)),"borrowed active string identity");}
  reinterpret_cast<Get>(0x10017de0)(scratch.data());after=pointer;need(before==object,"getter object state unchanged");need(fixture.queues==2&&fixture.warnings==warnings&&fixture.names==names,"native service counts");
  // Types5/9..15 are controlled unknown-layout getter inputs; restore undefined
  // before cleanup rather than asserting a lifetime for unsupported storage.
  if(type==5||type>=9)object[28]=6;
  using Clear=void(__thiscall*)(void*);reinterpret_cast<Clear>(0x1001a0a0)(object.data());reinterpret_cast<Clear>(0x1001a0a0)(scratch.data());
  need(fixture.refs==0&&fixture.allocations==fixture.releases,"actual resource/string/matrix cleanup");fixture.image.verify();totalAlloc+=fixture.allocations;totalFree+=fixture.releases;
#else
  MaterialGetStringInputCandidate input;input.type=type;input.components=components;input.integer=integer;input.numbers=numbers;input.text=inputText;input.resourceName=resourceText;input.environmentTexture=environment!=0;need(materialGetStringCandidate(input,snapshot),"owned getter");after=storage==2?"42":snapshot;
#endif
  put(out,storage);text(out,snapshot);text(out,after);put(out,2);put(out,warnings);put(out,names);put(out,names?1:0);
 }
 need(in.peek()==std::char_traits<char>::eof(),"query tail");out.flush();need(bool(out),"output");
#ifdef SFM64_NATIVE_GET_STRING
 _controlfp_s(&control,oldControl,_MCW_RC|_MCW_PC);std::cout<<"allocations="<<totalAlloc<<" releases="<<totalFree<<'\n';
#endif
 std::cout<<"PASS get_string_cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

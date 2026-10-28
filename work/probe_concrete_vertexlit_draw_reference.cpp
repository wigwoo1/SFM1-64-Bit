// Whole original VertexLitGeneric slot14, real info builders/cloak gate/skip
// dispatcher. Main/cloak/emissive/flesh render helpers are boundary observers.
#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
#include <immintrin.h>
#include "studio_vertex_lit_passes_candidate.h"
#include "studio_shader_cloak_gate_candidate.h"
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct Query{uint32_t serial,enabled,changed,shadow,factor,strength,mutate,compression;};
struct Var{uintptr_t table;uint32_t id,integer;std::array<uint32_t,4>vec;uint32_t type;};
struct Object{uintptr_t table;};
static Query q;static std::array<Var,125>vars;static std::array<Var*,125>params;
static std::array<std::vector<uint32_t>,4>info;
static Object shader,api,shadowObject,service;static uint32_t contextWord=0,ownedPass=0;
static std::vector<std::array<uint32_t,8>>events;static uint32_t leaves=0;
#ifndef _WIN64
static uint8_t*imageBase;
static void word(size_t at,uintptr_t v){uint32_t w=uint32_t(v);std::memcpy(imageBase+at,&w,4);}
static uint32_t word(size_t at){uint32_t v;std::memcpy(&v,imageBase+at,4);return v;}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"REFERENCE_FAULT eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);}
static_assert(offsetof(Var,integer)==8&&offsetof(Var,vec)==12&&offsetof(Var,type)==28,"native variable fields");
#endif
static uint32_t pass(){
#ifdef _WIN64
 return ownedPass;
#else
 return word(0xdb958);
#endif
}
static void pass(uint32_t v){
#ifdef _WIN64
 ownedPass=v;
#else
 word(0xdb958,v);
#endif
}
static uint32_t enabled(){return uint32_t(vars[73].integer!=0)|uint32_t(vars[65].integer!=0)*2u|uint32_t(vars[77].integer!=0)*4u;}
static void enabled(uint32_t mask){vars[73].integer=mask&1;vars[65].integer=mask&2;vars[77].integer=mask&4;}
#ifdef _WIN64
static float value(uint32_t id){float f;std::memcpy(&f,&vars[id].vec[0],4);return f;}
#endif
static uint32_t digest(const std::vector<uint32_t>&words){uint32_t h=2166136261u;for(auto v:words)h=(h^v)*16777619u;return h;}
static void emit(uint32_t kind){const auto index=kind?kind-1:0;events.push_back({kind,kind?digest(info[index]):0,kind?uint32_t(info[index].size()):0,pass(),enabled(),q.shadow,kind?q.compression:0,kind==1?1u:0u});if(events.size()==1&&q.mutate)enabled(q.changed);}
static void leaf(uint32_t kind){emit(kind);pass(pass()+1);++leaves;}
#ifdef _WIN64
struct Ops{
 bool cloakEnabled(){return vars[73].integer!=0;}bool emissiveEnabled(){return vars[65].integer!=0;}bool fleshEnabled(){return vars[77].integer!=0;}
 bool cloakOpaque(){return studioShaderCloakOpaqueCandidate(value(74));}float cloakFactor(){return value(74);}float emissiveStrength(){return value(68);}
 void skip(uint32_t v){need(v==0,"owned skip argument");emit(0);pass(pass()+1);}void mainLighting(){leaf(1);}void cloak(){leaf(2);}void emissive(){leaf(3);}void flesh(){leaf(4);}
};
#else
static uint32_t __fastcall defined(void*,void*){return 1;}
static void __fastcall skipShadow(void*,void*){need(q.shadow==1,"shadow skip service");emit(0);}
static void __fastcall skipRuntime(void*,void*,uint32_t data,uint32_t draw){need(!q.shadow&&data==0xaabbccdd&&draw==0,"runtime skip supplied instance data and draw0");emit(0);}
static void boundary(unsigned index,CONTEXT&c){
 auto*a=reinterpret_cast<const uint32_t*>(c.Esp);need(a[1]==reinterpret_cast<uintptr_t>(&shader)&&a[2]==reinterpret_cast<uintptr_t>(params.data())&&a[3]==reinterpret_cast<uintptr_t>(&api)&&a[4]==(q.shadow?reinterpret_cast<uintptr_t>(&shadowObject):0),"real concrete helper supplied arguments");
 const uint32_t*record=nullptr;
 if(index==0){need(a[5]==1&&a[7]==q.compression&&a[8]==reinterpret_cast<uintptr_t>(&contextWord),"main vertexlit/compression/context arguments");record=reinterpret_cast<const uint32_t*>(a[6]);}
 else{need(a[6]==q.compression,"effect compression");record=reinterpret_cast<const uint32_t*>(a[5]);}
 if(std::memcmp(record,info[index].data(),info[index].size()*4)!=0){for(size_t i=0;i<info[index].size();++i)if(record[i]!=info[index][i])std::cerr<<"INFO_MISMATCH case="<<q.serial<<" helper="<<index<<" word="<<i<<" actual="<<record[i]<<" expected="<<info[index][i]<<'\n';throw std::runtime_error("complete original-built parameter-index record");}
 leaf(index+1);PrivateX86BoundaryObserverFixture::returnCdecl(c);
}
#endif
int main(int argc,char**argv){try{
#ifdef _WIN64
 need(argc==4&&!std::ifstream(argv[3]).good(),"mapping query fresh trace");const char*mapping=argv[1],*inputPath=argv[2],*outputPath=argv[3];
#else
 need(argc==5&&!std::ifstream(argv[4]).good(),"private shader mapping query fresh trace");const char*mapping=argv[2],*inputPath=argv[3],*outputPath=argv[4];SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);auto handler=AddVectoredExceptionHandler(0,&fault);NativePrivateWorldImageFixture image(argv[1],0xe0000);imageBase=image.base;
#endif
 std::ifstream map(mapping,std::ios::binary);uint32_t n=0;map.read(reinterpret_cast<char*>(&n),4);need(n==112,"registered extra parameter index bindings");for(uint32_t i=0;i<n;++i){uint32_t pair[2];map.read(reinterpret_cast<char*>(pair),8);need(pair[0]>=0xc0000&&pair[0]<0xdb000&&pair[1]>=13&&pair[1]<125,"bounded data-only descriptor index binding");
#ifndef _WIN64
 word(pair[0],pair[1]);
#endif
 }
 const uint32_t lengths[]{110,7,7,20};for(size_t i=0;i<4;++i){uint32_t size=0;map.read(reinterpret_cast<char*>(&size),4);need(size==lengths[i],"complete info record extent");info[i].resize(size);map.read(reinterpret_cast<char*>(info[i].data()),size*4);}
 need(bool(map)&&map.peek()==std::char_traits<char>::eof(),"complete mapping framing");
#ifndef _WIN64
 std::array<uintptr_t,18>variableMethods{};variableMethods[17]=reinterpret_cast<uintptr_t>(&defined);for(auto&v:vars)v.table=reinterpret_cast<uintptr_t>(variableMethods.data());
 std::array<uintptr_t,5>serviceMethods{};serviceMethods[3]=reinterpret_cast<uintptr_t>(&skipShadow);serviceMethods[4]=reinterpret_cast<uintptr_t>(&skipRuntime);service.table=reinterpret_cast<uintptr_t>(serviceMethods.data());shader.table=0x1009c39c;
 std::array<uint8_t,128>configuration{};std::array<uint32_t,2>record{0,0xaabbccdd};std::array<uintptr_t,8>instanceRecords{};instanceRecords.fill(reinterpret_cast<uintptr_t>(record.data()));
 word(0xdb9dc,reinterpret_cast<uintptr_t>(configuration.data()));word(0xdb9e0,reinterpret_cast<uintptr_t>(&service));word(0xdb95c,reinterpret_cast<uintptr_t>(instanceRecords.data()));image.seal();
 PrivateX86BoundaryObserverFixture traps({0x1005a820,0x10009140,0x10015c60,0x1001f590},&boundary);traps.configure(true);
#endif
 for(uint32_t i=0;i<125;++i){vars[i].id=i;vars[i].type=4;params[i]=&vars[i];}
 std::ifstream input(inputPath,std::ios::binary);std::ofstream output(outputPath,std::ios::binary);need(bool(input)&&bool(output),"query trace IO");uint32_t cases=0,eventCount=0,leafCount=0;
 while(input.read(reinterpret_cast<char*>(&q),sizeof(q))){need(q.serial==cases&&q.enabled<8&&q.changed<8&&q.shadow<2&&q.mutate<2,"query domain");events.clear();leaves=0;enabled(q.enabled);vars[74].vec[0]=q.factor;vars[68].vec[0]=q.strength;pass(0);_mm_setcsr(0x1f80);
#ifdef _WIN64
  Ops ops;studioVertexLitPassesCandidate(q.shadow?reinterpret_cast<uintptr_t>(&shadowObject):0,ops);
#else
  word(0xdb940,q.shadow?reinterpret_cast<uintptr_t>(&shadowObject):0);word(0xdb944,reinterpret_cast<uintptr_t>(params.data()));word(0xdb948,reinterpret_cast<uintptr_t>(&api));
  auto*table=*reinterpret_cast<uintptr_t**>(&shader);using Draw=void(__thiscall*)(void*,void*,void*,void*,uint32_t,void*);reinterpret_cast<Draw>(table[14])(&shader,params.data(),q.shadow?&shadowObject:nullptr,&api,q.compression,&contextWord);
#endif
  need(pass()==events.size()&&!events.empty()&&events.size()<=4,"declared single-pass boundary observers and skip dispatcher");const uint32_t header[]{cases,uint32_t(events.size()),enabled(),pass(),leaves};output.write(reinterpret_cast<const char*>(header),sizeof(header));for(const auto&e:events)output.write(reinterpret_cast<const char*>(e.data()),sizeof(e));eventCount+=uint32_t(events.size());leafCount+=leaves;++cases;
 }
 need(input.eof()&&input.gcount()==0&&cases,"complete query framing");output.flush();need(bool(output),"complete trace");
#ifndef _WIN64
 traps.configure(false);need(traps.hits==leafCount,"exact intercepted helper count");image.verify();need(VirtualFree(image.base,0,MEM_RELEASE)!=0,"private image release");RemoveVectoredExceptionHandler(handler);
#endif
 std::cout<<"PASS concrete_vertexlit_cases="<<cases<<" events="<<eventCount<<" helper_boundaries="<<leafCount<<" real_render_helpers=NOT_RUN";
#ifdef _WIN64
 std::cout<<" target=OWNED_X64\n";
#else
 std::cout<<" target=ORIGINAL_X86_REFERENCE original_code=UNCHANGED_RX\n";
#endif
 return 0;}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}

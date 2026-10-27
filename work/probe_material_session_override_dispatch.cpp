#define NOMINMAX
#include <windows.h>
#include <array>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>
#ifdef SFM64_NATIVE_SESSION_OVERRIDE
#include "native_private_world_image_fixture.h"
#else
#include "material_session_override_dispatch_candidate.h"
#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_material_override_dispatch_candidate.h"
#include "live_session_scene_candidate.h"
#include "session_material_overrides_candidate.h"
#include "session_render_root_candidate.h"
#endif
static void requireOverride(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static uint32_t readWord(std::istream&in){uint32_t v=0;requireOverride(bool(in.read(reinterpret_cast<char*>(&v),4)),"Query word");return v;}
static void putWord(std::ostream&out,uint32_t v){out.write(reinterpret_cast<const char*>(&v),4);}
static std::string readText(std::istream&in){const auto n=readWord(in);requireOverride(n<=4095,"Query text extent");std::string s(n,'\0');requireOverride(bool(in.read(s.data(),n)),"Query text");return s;}
struct OverrideRow {uint32_t nameState=0,stringState=0,type=0,presence=3,texture=0;std::array<uint32_t,16>words{};std::string name,text;};
struct OverrideEvent {uint32_t kind=0,index=0,count=0;std::array<uint32_t,16>words{};};
static std::vector<OverrideRow>rows;static std::vector<OverrideEvent>events;static size_t selected=0;
static void event(uint32_t kind,size_t index,std::initializer_list<uint32_t>data){OverrideEvent e;e.kind=kind;e.index=uint32_t(index);e.count=uint32_t(data.size());std::copy(data.begin(),data.end(),e.words.begin());events.push_back(e);}
#ifdef SFM64_NATIVE_SESSION_OVERRIDE
template<size_t N>static void field(std::array<uint8_t,N>&v,size_t at,uintptr_t x){requireOverride(at+4<=N,"Native field extent");const auto word=uint32_t(x);std::memcpy(v.data()+at,&word,4);}
struct Variable {uintptr_t methods;uint32_t index;};static std::vector<Variable>variables;
static void*__fastcall findVariable(void*,void*,const char*name,bool*found,bool complain){
 size_t index=0;for(;index<rows.size();++index)if(rows[index].nameState==0&&rows[index].name.c_str()==name)break;requireOverride(index<rows.size(),"Original FindVar name identity");selected=index;const auto&r=rows[index];*found=bool(r.presence&2);event(0,index,{uint32_t(complain),uint32_t(*found),uint32_t(bool(r.presence&1))});return r.presence&1?&variables[index]:nullptr;
}
static size_t variableIndex(void*p){return static_cast<Variable*>(p)->index;}
static void __fastcall setInteger(void*p,void*,uint32_t v){event(1,variableIndex(p),{v});}
static void __fastcall setScalar(void*p,void*,uint32_t v){event(2,variableIndex(p),{v});}
static void __fastcall setVector2(void*p,void*,uint32_t a,uint32_t b){event(3,variableIndex(p),{a,b});}
static void __fastcall setVector3(void*p,void*,uint32_t a,uint32_t b,uint32_t c){event(4,variableIndex(p),{a,b,c});}
static void __fastcall setVector4(void*p,void*,uint32_t a,uint32_t b,uint32_t c,uint32_t d){event(5,variableIndex(p),{a,b,c,d});}
static void __fastcall setMatrix(void*p,void*,const uint32_t*data){OverrideEvent e;e.kind=6;e.index=uint32_t(variableIndex(p));e.count=16;std::copy_n(data,16,e.words.begin());events.push_back(e);}
static void*__fastcall findTexture(void*,void*,const char*name,const char*group,bool complain,int flags){const auto&r=rows[selected];const char*expected=r.stringState==1?nullptr:r.stringState==2?"":r.text.c_str();requireOverride((!expected&&!name)||(expected&&name&&std::strcmp(expected,name)==0),"Original texture lookup string");requireOverride(group&&!*group&&!complain&&!flags,"Original FindTexture default arguments");event(7,selected,{uint32_t(name==nullptr),uint32_t(name&&!*name),uint32_t(complain),uint32_t(flags)});return reinterpret_cast<void*>(uintptr_t(r.texture));}
static void __fastcall setTexture(void*p,void*,void*texture){event(8,variableIndex(p),{uint32_t(reinterpret_cast<uintptr_t>(texture))});}
#else
struct OwnedOverrideServices:MaterialSessionOverrideServicesCandidate {
 const std::vector<MaterialSessionOverrideInputCandidate>*active=nullptr;
 MaterialSessionOverrideLookupCandidate find(std::string_view name)override{size_t i=0;requireOverride(active!=nullptr,"Owned input lifetime");for(;i<active->size();++i)if((*active)[i].name.data()==name.data())break;selected=i;requireOverride(i<rows.size()&&std::string_view(rows[i].name).substr(0,rows[i].name.find('\0'))==name,"Owned stable input name");const auto&r=rows[i];event(0,i,{0,uint32_t(bool(r.presence&2)),uint32_t(bool(r.presence&1))});return {r.presence&1?i+1:0,bool(r.presence&2)};}
 void integer(uintptr_t p,uint32_t v)override{event(1,p-1,{v});}
 void scalar(uintptr_t p,uint32_t v)override{event(2,p-1,{v});}
 void vector(uintptr_t p,const std::array<uint32_t,4>&v,uint32_t n)override{OverrideEvent e;e.kind=n==2?3:n==3?4:5;e.index=uint32_t(p-1);e.count=n;std::copy_n(v.begin(),n,e.words.begin());events.push_back(e);}
 void matrix(uintptr_t p,const std::array<uint32_t,16>&v)override{OverrideEvent e;e.kind=6;e.index=uint32_t(p-1);e.count=16;e.words=v;events.push_back(e);}
 std::optional<uintptr_t>texture(const std::optional<std::string>&name)override{event(7,selected,{uint32_t(!name),uint32_t(name&&name->empty()),0,0});return rows[selected].texture;}
 void setTexture(uintptr_t p,uintptr_t value)override{event(8,p-1,{uint32_t(value)});}
};
static void writeOverrideText(std::ostream&out,const std::string&s){putWord(out,uint32_t(s.size()));out.write(s.data(),s.size());}
static int benchmarkQuery(const char*session,const char*destination){
 requireOverride(!std::ifstream(destination).good(),"Fresh benchmark query");std::ofstream out(destination,std::ios::binary);requireOverride(bool(out),"Benchmark query open");putWord(out,0);
 auto graph=loadGraph(session);RetainedShotTimelineCandidate timeline(graph,sessionRenderRootCandidate(graph));uint32_t count=0,attributes=0;
 for(int32_t tick:{50000,50167,50333}){auto step=evaluateLiveSessionSceneCandidate(timeline,tick,512,512);for(const auto&model:step.scene.models)for(const auto&record:readSessionMaterialOverridesCandidate(timeline.graph(),model.id)){
  const auto input=readSessionMaterialOverrideDispatchCandidate(timeline.graph(),record.element);putWord(out,3);putWord(out,uint32_t(input.size()));
  for(const auto&r:input){putWord(out,0);putWord(out,r.textureName?0:1);putWord(out,r.typeFlags);putWord(out,3);putWord(out,0x12340000u+count);for(auto v:r.words)putWord(out,v);writeOverrideText(out,r.name);writeOverrideText(out,r.textureName?*r.textureName:std::string());}
  std::cout<<"benchmark_case="<<count<<" tick="<<tick<<" model="<<model.id<<" material="<<record.element<<" attributes="<<input.size()<<'\n';++count;attributes+=uint32_t(input.size());
 }}out.seekp(0);putWord(out,count);out.flush();requireOverride(bool(out)&&count>0,"Complete benchmark query");std::cout<<"PASS actual_override_material_snapshots="<<count<<" ordered_attributes="<<attributes<<" variable_and_texture_services=CONTROLLED\n";return 0;
}
#endif
int main(int argc,char**argv){try{
#ifndef SFM64_NATIVE_SESSION_OVERRIDE
 if(argc==4&&std::string(argv[1])=="--benchmark")return benchmarkQuery(argv[2],argv[3]);
#endif
 requireOverride(argc==4,"private-or-absent-image query fresh-output");requireOverride(!std::ifstream(argv[3]).good(),"Fresh output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);requireOverride(bool(in)&&bool(out),"Probe file IO");
#ifdef SFM64_NATIVE_SESSION_OVERRIDE
 static_assert(sizeof(void*)==4,"Separate private original reference x86");SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);NativePrivateWorldImageFixture image(argv[1]);
 std::array<uintptr_t,84>systemMethods{};systemMethods[83]=reinterpret_cast<uintptr_t>(&findTexture);uintptr_t system=reinterpret_cast<uintptr_t>(systemMethods.data());const auto systemPointer=reinterpret_cast<uintptr_t>(&system);std::memcpy(image.base+0xf4c778,&systemPointer,4);image.seal();
 std::array<uintptr_t,20>variableMethods{};variableMethods[3]=reinterpret_cast<uintptr_t>(&setScalar);variableMethods[4]=reinterpret_cast<uintptr_t>(&setInteger);variableMethods[9]=reinterpret_cast<uintptr_t>(&setVector4);variableMethods[10]=reinterpret_cast<uintptr_t>(&setVector3);variableMethods[11]=reinterpret_cast<uintptr_t>(&setVector2);variableMethods[14]=reinterpret_cast<uintptr_t>(&setTexture);variableMethods[19]=reinterpret_cast<uintptr_t>(&setMatrix);
 std::array<uintptr_t,12>materialMethods{};materialMethods[11]=reinterpret_cast<uintptr_t>(&findVariable);uintptr_t material=reinterpret_cast<uintptr_t>(materialMethods.data());
#else
 static_assert(sizeof(void*)==8,"Independent native64 dispatch");requireOverride(!std::ifstream(argv[1]).good(),"Owned path must not consume an engine image");
#endif
 const auto cases=readWord(in);requireOverride(cases&&cases<100000,"Case count");putWord(out,cases);unsigned total=0;
 for(uint32_t caseIndex=0;caseIndex<cases;++caseIndex){const auto flags=readWord(in),count=readWord(in);requireOverride(count<=4096,"Row count");rows.clear();rows.resize(count);events.clear();for(auto&r:rows){r.nameState=readWord(in);r.stringState=readWord(in);r.type=readWord(in);r.presence=readWord(in);r.texture=readWord(in);for(auto&v:r.words)v=readWord(in);r.name=readText(in);r.text=readText(in);requireOverride(r.nameState<3&&r.stringState<3&&r.presence<4,"Fixture states");}
#ifdef SFM64_NATIVE_SESSION_OVERRIDE
 std::vector<std::array<uint8_t,24>>attributes(count);std::vector<std::array<uint32_t,16>>payloads(count);variables.resize(count);
 for(size_t i=0;i<count;++i){const auto&r=rows[i];payloads[i]=r.words;variables[i]={reinterpret_cast<uintptr_t>(variableMethods.data()),uint32_t(i)};field(attributes[i],0,i+1<count?reinterpret_cast<uintptr_t>(attributes[i+1].data()):0);field(attributes[i],4,reinterpret_cast<uintptr_t>(payloads[i].data()));field(attributes[i],0x10,r.type);field(attributes[i],0x14,r.nameState==1?0:r.nameState==2?UINT32_MAX:reinterpret_cast<uintptr_t>(r.name.c_str()));if((r.type&31)==5)payloads[i][0]=r.stringState==1?0:r.stringState==2?UINT32_MAX:uint32_t(reinterpret_cast<uintptr_t>(r.text.c_str()));}
 std::array<uint8_t,0x34>element{};field(element,0x30,count?reinterpret_cast<uintptr_t>(attributes[0].data()):0);using Apply=void(__cdecl*)(void*,void*);reinterpret_cast<Apply>(0x1023e140)(flags&1?element.data():nullptr,flags&2?&material:nullptr);image.verify();
#else
 if((flags&3)==3){OwnedOverrideServices services;std::vector<MaterialSessionOverrideInputCandidate>input;input.reserve(rows.size());for(const auto&r:rows){MaterialSessionOverrideInputCandidate v;v.name=r.name;v.nameAvailable=r.nameState==0;v.typeFlags=r.type;v.words=r.words;v.textureName=r.stringState==1?std::optional<std::string>{}:r.stringState==2?std::optional<std::string>(std::string()):std::optional<std::string>(r.text);input.push_back(std::move(v));}services.active=&input;requireOverride(dispatchMaterialSessionOverridesCandidate(input,services),"Owned dispatch");}
#endif
 putWord(out,caseIndex);putWord(out,uint32_t(events.size()));for(const auto&e:events)out.write(reinterpret_cast<const char*>(&e),sizeof(e));total+=unsigned(events.size());
 }
 requireOverride(in.peek()==std::char_traits<char>::eof(),"Query trailing bytes");out.flush();requireOverride(bool(out),"Output complete");
#ifdef SFM64_NATIVE_SESSION_OVERRIDE
 image.verify();requireOverride(VirtualFree(image.base,0,MEM_RELEASE)!=0,"Private image release");
#endif
 std::cout<<"PASS typed_override_cases="<<cases<<" events="<<total<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

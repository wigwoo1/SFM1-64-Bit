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
#ifdef SFM64_NATIVE_MODEL_CONTEXT
#include "native_private_world_image_fixture.h"
#else
#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_model_proxy_context_candidate.h"
#include "live_session_scene_candidate.h"
#include "session_render_root_candidate.h"
#endif
static void need(bool b,const char*m){if(!b)throw std::runtime_error(m);}
static uint32_t get(std::istream&in){uint32_t v=0;in.read(reinterpret_cast<char*>(&v),4);need(bool(in),"Query framing");return v;}
static void put(std::ostream&out,uint32_t v){out.write(reinterpret_cast<const char*>(&v),4);}
#ifndef SFM64_NATIVE_MODEL_CONTEXT
static Graph modelGraph(const std::string&name){
 Graph g;g.strings={"DmeGameModel","modelName","model","DmeDag"};Element e{};e.type=0;e.name=2;e.id[0]=42;Value v;v.text=name;e.attributes.push_back({1,5,{v}});g.elements.push_back(e);return g;
}
static void safety(){
 unsigned n=0;auto check=[&](bool b){need(b,"Model-context safety");++n;};
 SessionModelProxyContextCandidate unknown;check(!unknown.knownOrdinaryIfmPuppet()&&!unknown.gameEntityPresent().has_value());
 auto g=modelGraph("models/a.mdl");auto c=readSessionModelProxyContextCandidate(g,0);check(c.knownOrdinaryIfmPuppet()&&c.gameEntityPresent()==std::optional<bool>(false)&&c.matches(0,g.elements[0].id,"models/a.mdl"));
 auto copied=c;std::vector<SessionModelProxyContextCandidate>moved;moved.push_back(std::move(copied));for(int i=0;i<100;++i)moved.emplace_back();check(moved.front().matches(0,g.elements[0].id,"models/a.mdl"));
 auto guid=g.elements[0].id;g.elements[0].attributes[0].values[0].text="models/b.mdl";g.elements[0].id[0]=7;check(!c.matches(0,g.elements[0].id,"models/b.mdl")&&c.matches(0,guid,"models/a.mdl"));g=Graph{};check(c.asset()=="models/a.mdl"&&c.gameEntityPresent()==std::optional<bool>(false));
 auto invalid=[&](Graph v){return !readSessionModelProxyContextCandidate(v,0).gameEntityPresent().has_value();};
 auto fresh=modelGraph("models/a.mdl");check(!readSessionModelProxyContextCandidate(fresh,UINT32_MAX).knownOrdinaryIfmPuppet());
 for(const std::string name:{std::string(),std::string("?"),std::string("*1"),std::string("*bad"),std::string("a\0b",3)})check(invalid(modelGraph(name)));
 for(unsigned i=0;i<8;++i){auto bad=fresh;switch(i){case 0:bad.elements[0].type=3;break;case 1:bad.elements[0].type=99;break;case 2:bad.elements[0].attributes.clear();break;case 3:bad.elements[0].attributes.push_back(bad.elements[0].attributes[0]);break;case 4:bad.elements[0].attributes[0].type=3;break;case 5:bad.elements[0].attributes[0].values.clear();break;case 6:bad.elements[0].attributes[0].values.push_back(bad.elements[0].attributes[0].values[0]);break;case 7:bad.elements[0].attributes[0].name=99;break;}check(invalid(std::move(bad)));}
 fresh.elements.push_back(fresh.elements[0]);fresh.elements[1].id[1]=1;auto other=readSessionModelProxyContextCandidate(fresh,1);check(!other.matches(0,fresh.elements[0].id,"models/a.mdl")&&other.matches(1,fresh.elements[1].id,"models/a.mdl"));
 std::cout<<"PASS model_context_safety_checks="<<n<<'\n';
}
static void benchmark(const char*path){
 auto graph=loadGraph(path);RetainedShotTimelineCandidate timeline(graph,sessionRenderRootCandidate(graph));unsigned frames=0,total=0,unknown=0;
 for(int32_t tick:{50000,50167,50333}){auto step=evaluateLiveSessionSceneCandidate(timeline,tick,512,512);need(!step.scene.models.empty(),"Benchmark models");for(const auto&m:step.scene.models){
  // The real benchmark includes inline brush instances. Their additional
  // engine lookup is outside the established ordinary-model provenance.
  if(m.placement.asset.front()=='*'||m.placement.asset=="?"){
   need(!m.proxyContext.knownOrdinaryIfmPuppet()&&!m.proxyContext.gameEntityPresent().has_value(),"Exceptional benchmark source remains unknown");++unknown;
   if(frames==0)std::cout<<"unknown_context_model="<<m.id<<" asset="<<m.placement.asset<<'\n';
  }else need(m.proxyContext.matches(m.id,timeline.graph().elements.at(m.id).id,m.placement.asset)&&m.proxyContext.gameEntityPresent()==std::optional<bool>(false),"Benchmark per-instance source binding");++total;
 }++frames;}
 std::cout<<"PASS benchmark_context_frames="<<frames<<" model_snapshots="<<total<<" unknown_snapshots="<<unknown<<" material_evaluation=NOT_RUN visual_parity=UNVERIFIED\n";
}
#endif
int main(int argc,char**argv){try{
#ifndef SFM64_NATIVE_MODEL_CONTEXT
 static_assert(sizeof(void*)==8,"Owned context x64");
 if(argc==2&&std::string(argv[1])=="--safety"){safety();return 0;}
 if(argc==3&&std::string(argv[1])=="--benchmark"){benchmark(argv[2]);return 0;}
#else
 static_assert(sizeof(void*)==4,"Private original metadata reference x86");
#endif
 need(argc==4&&!std::ifstream(argv[3]).good(),"original-or-absent query fresh-output");std::ifstream input(argv[2],std::ios::binary);std::ofstream output(argv[3],std::ios::binary);need(bool(input)&&bool(output),"IO");
#ifdef SFM64_NATIVE_MODEL_CONTEXT
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);NativePrivateWorldImageFixture image(argv[1]);image.seal();
 using Initialize=void(__cdecl*)();reinterpret_cast<Initialize>(0x10a6e3b0)();
 auto word=[](uintptr_t p){uint32_t v=0;std::memcpy(&v,reinterpret_cast<void*>(p),4);return v;};
 need(word(0x10f62eac+0x10)==0x10eb52d8&&word(0x10f62eac+0x14)==0x10b948cc&&word(0x10f4ad58)==0x10f62eac,"Original type registration record");
 auto*descriptor=reinterpret_cast<void*>(0x10eb52d8);auto*methods=*reinterpret_cast<uintptr_t**>(descriptor);using SetSymbol=void(__thiscall*)(void*,uint32_t);using Compare=int(__cdecl*)(const char*,const char*);
 std::array<uint32_t,2>shell{0x10adb9b8,0x10adb904};using Entity=void*(__thiscall*)(void*);auto*unknownMethods=*reinterpret_cast<uintptr_t**>(shell.data());need(reinterpret_cast<Entity>(unknownMethods[7])(shell.data())==nullptr,"Actual puppet no-entity accessor");
#endif
 const auto count=get(input);need(count>0&&count<10000,"Case count");put(output,count);unsigned known=0;
 for(uint32_t i=0;i<count;++i){const auto size=get(input);need(size<=4096,"Name extent");std::string name(size,'\0');input.read(name.data(),size);need(bool(input)&&name.find('\0')==std::string::npos,"Canonical C string corpus");bool admitted=false;
#ifdef SFM64_NATIVE_MODEL_CONTEXT
  // The class symbol precedes its factory object. The following DWORD belongs
  // to the next class (DmeGameSprite), not to this object's name at +0x18.
  reinterpret_cast<SetSymbol>(methods[5])(descriptor,i+17);need(word(0x10eb52d4)==i+17,"Original name-associated type-symbol target");
  admitted=!name.empty()&&reinterpret_cast<Compare>(0x10975120)(name.c_str(),"?")!=0&&name[0]!='*';image.verify();
#else
  auto g=modelGraph(name);auto c=readSessionModelProxyContextCandidate(g,0);admitted=c.knownOrdinaryIfmPuppet();need(!admitted||c.matches(0,g.elements[0].id,name),"Owned model identity");
#endif
  put(output,i);put(output,admitted);put(output,admitted?1:0);put(output,admitted?0:UINT32_MAX);known+=admitted;
 }
 need(input.peek()==std::char_traits<char>::eof(),"Query tail");output.flush();need(bool(output),"Output complete");
#ifdef SFM64_NATIVE_MODEL_CONTEXT
 image.verify();need(VirtualFree(image.base,0,MEM_RELEASE)!=0,"Private image release");
#endif
 std::cout<<"PASS model_context_cases="<<count<<" ordinary_puppet="<<known<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

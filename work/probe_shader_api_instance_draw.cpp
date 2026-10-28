// Whole original instance draw orchestrator, controlled quiescent graphics
// preparation/observers, versus owned x64 protocol. No original code arrays,
// instruction patches, DLL entry/import initialization, live SFM or GPU.
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
#include "shader_api_instance_draw_candidate.h"
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct Query{uint32_t index,present,count,firstMode,secondMode,mutations,disabled,third,restore,alpha,configurationMode,compression,materialFlag,meshFlag,alphaResult;};
static_assert(sizeof(Query)==60,"reference query wire");
struct Object{uintptr_t table;uint32_t id;};
struct Event{uint32_t kind,id,a,b;};
static Query q;
static ShaderApiInstanceDrawStateCandidate state;
static std::array<Object,3>meshes,materials;
static std::array<uint32_t,19>instance,alternateInstance;
static std::array<uint32_t,2>metadata;
static std::vector<Event>events;
static uintptr_t liveMaterial=0;
static uint32_t modeCalls=0,suppressions=0,draws=0,checks=0;
#ifndef _WIN64
static std::array<uint8_t,0x4000>world;
static uint8_t*iface=nullptr;
static std::array<uint8_t,0x50>configuration;
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"REFERENCE_FAULT eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);}
static void put(uint8_t*p,size_t at,uintptr_t v){uint32_t b=uint32_t(v);std::memcpy(p+at,&b,4);}
static uint32_t get(const uint8_t*p,size_t at){uint32_t b;std::memcpy(&b,p+at,4);return b;}
#endif
static uintptr_t mesh(uint32_t id){return id?reinterpret_cast<uintptr_t>(&meshes.at(id-1)):0;}
static uintptr_t material(uint32_t id){return reinterpret_cast<uintptr_t>(&materials.at(id-1));}
static uint32_t objectId(uintptr_t p,const std::array<Object,3>&objects){if(!p)return 0;for(const auto&o:objects)if(reinterpret_cast<uintptr_t>(&o)==p)return o.id;throw std::runtime_error("invalid fixture identity");}
static uint32_t mid(uintptr_t p){return objectId(p,materials);}
static uint32_t meshid(uintptr_t p){return objectId(p,meshes);}
static void event(uint32_t kind,uint32_t id=0,uint32_t a=0,uint32_t b=0){events.push_back({kind,id,a,b});}
static uintptr_t current(){
#ifdef _WIN64
 return liveMaterial;
#else
 return get(iface,0x2c20);
#endif
}
static void current(uintptr_t m){
#ifdef _WIN64
 liveMaterial=m;
#else
 put(iface,0x2c20,m);
#endif
}
static void selectedMesh(uintptr_t m){
#ifdef _WIN64
 state.mesh=m;
#else
 put(iface,0x2be4,m);
#endif
}
static void publishedInstance(uintptr_t p){
#ifdef _WIN64
 state.instances=p;
#else
 put(iface,0x2bec,p);
#endif
}
static void postDraw(){
#ifdef _WIN64
 state.restore=uint8_t(q.restore);state.lightingState=0xface;state.lightingValid=1;
#else
 iface[0x2c18]=uint8_t(q.restore);put(iface,0x15a0,0xface);iface[0x159c]=1;
#endif
}
static std::array<uint32_t,2>format(uintptr_t p){event(2,meshid(p));if(q.mutations&1u){selectedMesh(mesh(2));
#ifdef _WIN64
 state.configuration[0]=0xdeadbeef;
#else
 put(iface,0x1a9c,0xdeadbeef);
#endif
 }return {0x12345678,0xfedcba98};}
static uint32_t meshCount(uintptr_t p){event(3,meshid(p),q.count);return q.count;}
static uint32_t queryMode(){const auto value=modeCalls++?q.secondMode:q.firstMode;event(4,0,value);return value;}
static uint64_t materialFlags(uintptr_t p){event(5,mid(p),q.materialFlag);if(q.mutations&2u){current(material(2));selectedMesh(mesh(3));}return uint64_t(q.materialFlag?0x100u:0u)|(uint64_t(0xfeedface)<<32);}
static uint32_t meshQuery(uintptr_t p){event(6,meshid(p));if(q.mutations&4u){current(material(3));selectedMesh(mesh(1));}return 0xcafebabe;}
static uint32_t materialAlpha(uintptr_t p,bool mode,uint32_t value){need(value==0xcafebabe,"mesh query to captured material");event(7,mid(p),mode,q.alphaResult);return q.alphaResult;}
static bool meshFlag(uintptr_t p){event(8,meshid(p),q.meshFlag);return q.meshFlag!=0;}
static void setup(const std::array<uint32_t,2>&fmt,bool flag,bool mf,uint8_t alpha){need(fmt==std::array<uint32_t,2>{0x12345678,0xfedcba98}&&flag==(q.materialFlag!=0)&&mf==(q.meshFlag!=0)&&alpha==uint8_t(q.alphaResult),"captured format/flags/setup arguments");++checks;event(9,0,uint32_t(flag)|(uint32_t(mf)<<8)|(uint32_t(alpha)<<16));if(q.mutations&8u){instance[18]=0x3e800000;publishedInstance(reinterpret_cast<uintptr_t>(alternateInstance.data()));}}
static bool suppress(){event(10,mid(current()));if(++suppressions==1&&(q.mutations&4u))current(material(2));return true;}
static uint32_t caps(){event(11);return 0;}
static void drawMaterial(uintptr_t p,uint32_t compression,bool changed,bool mode){
 const bool wantedMode=q.present&&int32_t(q.count)>0&&(q.firstMode==1||q.secondMode==2);const auto alpha=q.present&&(q.mutations&8u)?0x3e800000u:q.alpha;
 need(compression==q.compression&&changed==(alpha!=0x3f800000u)&&mode==wantedMode,"late alpha/original argument and draw flags");++checks;event(12,mid(p),compression,uint32_t(changed)|(uint32_t(mode)<<8));++draws;postDraw();
}
#ifdef _WIN64
struct Ops{
 bool disabled(){event(1);return q.disabled!=0;}std::array<uint32_t,2>format(uintptr_t p){return ::format(p);}uint32_t meshCount(uintptr_t p){return ::meshCount(p);}uint32_t queryMode(){return ::queryMode();}uintptr_t currentMaterial(){return current();}uint64_t materialFlags(uintptr_t p){return ::materialFlags(p);}uint32_t meshQuery(uintptr_t p){return ::meshQuery(p);}uint32_t materialAlpha(uintptr_t p,bool m,uint32_t v){return ::materialAlpha(p,m,v);}bool meshFlag(uintptr_t p){return ::meshFlag(p);}void setup(const std::array<uint32_t,2>&f,bool a,bool b,uint8_t c){::setup(f,a,b,c);}
 float firstAlpha(uintptr_t p){float f;std::memcpy(&f,reinterpret_cast<const uint32_t*>(p)+18,4);return f;}
 void prepareA(){}void prepareB(){}bool needsPrepareC(){return q.third!=0;}void prepareC(){caps();caps();}void drainCallbacks(){suppress();}void drawMaterial(uintptr_t p,uint32_t c,bool a,bool m){::drawMaterial(p,c,a,m);}void restore(uint8_t flag,const std::array<uint32_t,7>&){for(unsigned i=0;i<(flag?8u:1u);++i)suppress();}
};
#else
static void*__fastcall configNative(void*,void*){event(1);return configuration.data();}
static uint64_t __fastcall formatNative(void*p,void*){auto f=format(reinterpret_cast<uintptr_t>(p));return f[0]|(uint64_t(f[1])<<32);}
static uint32_t __fastcall countNative(void*p,void*){return meshCount(reinterpret_cast<uintptr_t>(p));}
static uint32_t __fastcall modeNative(void*,void*){return queryMode();}
static uint64_t __fastcall flagsNative(void*p,void*){return materialFlags(reinterpret_cast<uintptr_t>(p));}
static uint32_t __fastcall queryNative(void*p,void*){return meshQuery(reinterpret_cast<uintptr_t>(p));}
static uint32_t __fastcall alphaNative(void*p,void*,uint32_t mode,uint32_t value){return materialAlpha(reinterpret_cast<uintptr_t>(p),uint8_t(mode)!=0,value);}
static bool __fastcall meshFlagNative(void*p,void*){return meshFlag(reinterpret_cast<uintptr_t>(p));}
static void __fastcall setupNative(void*,void*,uint32_t lo,uint32_t hi,uint32_t flag,uint32_t mf,uint32_t alpha){setup({lo,hi},uint8_t(flag)!=0,uint8_t(mf)!=0,uint8_t(alpha));}
static bool __fastcall suppressNative(void*,void*){return suppress();}
static uint32_t __fastcall capsNative(void*,void*){return caps();}
static void __fastcall drawNative(void*p,void*,uint32_t c,uint32_t a,uint32_t m){drawMaterial(reinterpret_cast<uintptr_t>(p),c,uint8_t(a)!=0,uint8_t(m)!=0);}
#endif
int main(int argc,char**argv){try{
#ifdef _WIN64
 need(argc==3&&!std::ifstream(argv[2]).good(),"query/fresh owned trace");const char*inputPath=argv[1],*outputPath=argv[2];
#else
 need(argc==4&&!std::ifstream(argv[3]).good(),"read-only image/query/fresh trace");const char*inputPath=argv[2],*outputPath=argv[3];SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);auto handler=AddVectoredExceptionHandler(1,&fault);NativePrivateWorldImageFixture image(argv[1],0x100000);image.seal();iface=world.data()+0x2d0;
 std::array<uintptr_t,37>primary{};primary[36]=reinterpret_cast<uintptr_t>(&suppressNative);std::array<uintptr_t,277>secondary{};secondary[65]=reinterpret_cast<uintptr_t>(&modeNative);secondary[276]=reinterpret_cast<uintptr_t>(&setupNative);std::array<uintptr_t,16>hardware{};hardware[8]=reinterpret_cast<uintptr_t>(&configNative);hardware[15]=reinterpret_cast<uintptr_t>(&capsNative);uintptr_t hardwareObject=reinterpret_cast<uintptr_t>(hardware.data());auto hp=reinterpret_cast<uintptr_t>(&hardwareObject);put(image.base,0x183b58,hp);put(image.base,0x16d130,hp);
 std::array<uintptr_t,34>meshMethods{};meshMethods[1]=reinterpret_cast<uintptr_t>(&formatNative);meshMethods[32]=reinterpret_cast<uintptr_t>(&countNative);meshMethods[33]=reinterpret_cast<uintptr_t>(&queryNative);meshMethods[30]=reinterpret_cast<uintptr_t>(&meshFlagNative);std::array<uintptr_t,84>materialMethods{};materialMethods[20]=reinterpret_cast<uintptr_t>(&flagsNative);materialMethods[83]=reinterpret_cast<uintptr_t>(&alphaNative);materialMethods[71]=reinterpret_cast<uintptr_t>(&drawNative);
#endif
 std::ifstream input(inputPath,std::ios::binary);std::ofstream output(outputPath,std::ios::binary);need(bool(input)&&bool(output),"trace IO");uint32_t cases=0,eventTotal=0;
 while(input.read(reinterpret_cast<char*>(&q),sizeof(q))){need(q.index==cases&&q.present<=1&&q.mutations<16&&q.disabled<=1&&q.third<=1&&q.restore<=1,"query bounds");events.clear();modeCalls=suppressions=draws=0;for(uint32_t i=0;i<3;++i){meshes[i]={0,i+1};materials[i]={0,i+1};}instance.fill(0);instance[18]=q.alpha;alternateInstance.fill(0);metadata={111,222};state={};state.mesh=mesh(3);state.count=0x1111;state.instances=reinterpret_cast<uintptr_t>(alternateInstance.data());state.metadata1=reinterpret_cast<uintptr_t>(metadata.data());state.metadata2=reinterpret_cast<uintptr_t>(metadata.data()+1);state.restore=0x80;state.lightingValid=0x7f;state.lightingState=99;state.configurationMode=q.configurationMode;state.savedMode=0x9b;for(unsigned i=0;i<7;++i){state.configuration[i]=q.index*7+i;state.savedConfiguration[i]=0xabcdef00u+i;}current(material(1));
#ifdef _WIN64
  Ops ops;shaderApiInstanceDrawCandidate(state,q.present?mesh(1):0,0,reinterpret_cast<uintptr_t>(instance.data()),q.compression,reinterpret_cast<uintptr_t>(metadata.data()),reinterpret_cast<uintptr_t>(metadata.data()+1),ops);
#else
  world.fill(0);put(world.data(),0,reinterpret_cast<uintptr_t>(primary.data()));put(iface,0,reinterpret_cast<uintptr_t>(secondary.data()));put(iface,0x2be4,state.mesh);put(iface,0x2be8,state.count);put(iface,0x2bec,state.instances);put(iface,0x2bf0,state.metadata1);put(iface,0x2bf4,state.metadata2);iface[0x2c18]=state.restore;iface[0x159c]=state.lightingValid;put(iface,0x15a0,state.lightingState);put(iface,0x1a98,q.configurationMode);for(unsigned i=0;i<7;++i){put(iface,0x1a9c+i*4,state.configuration[i]);put(iface,0x2bfc+i*4,state.savedConfiguration[i]);}iface[0x2bf8]=state.savedMode;put(iface,0x15e0,q.third);current(material(1));configuration.fill(0);configuration[0x4c]=uint8_t(q.disabled);for(auto&m:meshes)m.table=reinterpret_cast<uintptr_t>(meshMethods.data());for(auto&m:materials)m.table=reinterpret_cast<uintptr_t>(materialMethods.data());using Draw=void(__thiscall*)(void*,void*,uint32_t,void*,uint32_t,void*,void*);reinterpret_cast<Draw>(0x10025b50)(iface,reinterpret_cast<void*>(q.present?mesh(1):0),0,instance.data(),q.compression,metadata.data(),metadata.data()+1);image.verify();state.mesh=get(iface,0x2be4);state.count=get(iface,0x2be8);state.instances=get(iface,0x2bec);state.metadata1=get(iface,0x2bf0);state.metadata2=get(iface,0x2bf4);state.restore=iface[0x2c18];state.lightingValid=iface[0x159c];state.lightingState=get(iface,0x15a0);state.savedMode=iface[0x2bf8];for(unsigned i=0;i<7;++i)state.savedConfiguration[i]=get(iface,0x2bfc+i*4);
#endif
  const auto expectedModes=!q.disabled&&q.present&&int32_t(q.count)>0?(q.firstMode==1?1u:2u):0u;need(modeCalls==expectedModes&&draws==uint32_t(!q.disabled),"independent mode query count and config skip");++checks;
  if(q.disabled){need(state.mesh==mesh(3)&&state.count==0x1111&&state.instances==reinterpret_cast<uintptr_t>(alternateInstance.data())&&state.restore==0x80&&state.lightingValid==0x7f&&state.lightingState==99&&state.savedMode==0x9b,"disabled preserves prior published state");}
  else{need(!state.mesh&&!state.count&&!state.instances&&!state.metadata1&&!state.metadata2&&!state.restore&&!state.lightingValid&&state.lightingState==0xface&&state.savedMode==uint8_t(q.configurationMode==1),"successful cleanup and intentionally retained lighting field");for(unsigned i=0;i<7;++i)need(state.savedConfiguration[i]==q.index*7+i,"configuration captured before callbacks");}
  ++checks;const uint32_t header[15]{cases,modeCalls,suppressions,draws,meshid(state.mesh),state.count,state.instances?1u:0u,state.metadata1?1u:0u,state.metadata2?1u:0u,state.restore,state.lightingValid,state.lightingState,state.savedMode,instance[18],uint32_t(events.size())};output.write(reinterpret_cast<const char*>(header),sizeof(header));output.write(reinterpret_cast<const char*>(state.savedConfiguration.data()),28);for(const auto&e:events)output.write(reinterpret_cast<const char*>(&e),sizeof(e));eventTotal+=uint32_t(events.size());++cases;
 }
 need(input.eof()&&input.gcount()==0&&cases,"query framing");output.flush();need(bool(output),"complete trace");
#ifndef _WIN64
 image.verify();need(VirtualFree(image.base,0,MEM_RELEASE)!=0,"private image release");RemoveVectoredExceptionHandler(handler);
#endif
 std::cout<<"PASS shader_instance_draw_cases="<<cases<<" events="<<eventTotal<<" independent_checks="<<checks<<" late_material=LIVE original_alpha_pointer=PASS graphics_preparation=QUIESCENT GPU=NOT_RUN";
#ifdef _WIN64
 std::cout<<" target=OWNED_X64\n";
#else
 std::cout<<" target=ORIGINAL_X86_REFERENCE original_code=UNCHANGED_RX\n";
#endif
 return 0;}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}

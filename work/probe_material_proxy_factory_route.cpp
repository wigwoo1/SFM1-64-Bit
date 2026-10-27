#include <array>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include "material_proxy_factory_route_candidate.h"
#ifdef SFM64_NATIVE_PROXY_FACTORY_ROUTE
#include <windows.h>
#include "native_private_world_image_fixture.h"
#endif
static void need(bool ok,const char*message){if(!ok)throw std::runtime_error(message);}
struct Query {uint32_t client=0,local=0,global=0,suppress=0;int32_t count=0,hit=-1;std::string name;};
static std::vector<Query>queries(const char*path){std::ifstream input(path);need(bool(input),"query input");std::vector<Query>result;Query q;while(input>>q.client>>q.local>>q.global>>q.suppress>>q.count>>q.hit){input.get();std::getline(input,q.name);need(q.client<=1&&q.local<=255&&q.global<=255&&q.suppress<=255&&q.count<=4,"bounded native query");result.push_back(q);}need(input.eof()&&!result.empty(),"complete query input");return result;}
#ifdef SFM64_NATIVE_PROXY_FACTORY_ROUTE
struct EngineRouteFixture {
 NativePrivateWorldImageFixture image;inline static EngineRouteFixture*active=nullptr;
 std::array<uintptr_t,73>clientMethods{};std::array<uintptr_t,48>toolMethods{};std::array<uintptr_t,33>leafMethods{};
 uintptr_t client=0;std::array<uint8_t,64>tools{};struct Leaf{uintptr_t table;int32_t index;};std::array<Leaf,4>leaves{};std::array<uintptr_t,4>leafPointers{};
 Query query;std::string trace;
 static void __fastcall unexpected(void*,void*){ExitProcess(91);}
 static EngineRouteFixture&self(){return *active;}
 static uintptr_t __fastcall clientCreate(void*p,void*,const char*name){auto&f=self();need(p==&f.client&&name==f.query.name,"client receiver/request");f.trace+="C ";return f.query.client;}
 static bool __fastcall enabled(void*p,void*){auto&f=self();need(p==f.tools.data(),"tool enabled receiver");f.trace+="E ";using Enabled=bool(__thiscall*)(void*);return reinterpret_cast<Enabled>(0x101e6680)(p);}
 static int32_t __fastcall count(void*p,void*){auto&f=self();need(p==f.tools.data(),"tool count receiver");f.trace+="N ";return f.query.count;}
 static uintptr_t __fastcall toolCreate(void*p,void*,const char*name){auto&f=self();auto*leaf=static_cast<Leaf*>(p);need(leaf->index>=0&&leaf->index<4&&name==f.query.name,"tool request");f.trace+="T"+std::to_string(leaf->index)+" ";return leaf->index==f.query.hit?uintptr_t(2+leaf->index):0;}
 static void __cdecl warning(const char*format,const char*name){auto&f=self();need(format==reinterpret_cast<char*>(0x1035c67c)&&name==f.query.name,"original warning format/request");f.trace+="W ";}
 void word(size_t offset,uintptr_t value){std::memcpy(image.base+offset,&value,4);}
 explicit EngineRouteFixture(const char*path):image(path,0x100000){
  need(!active,"single fixture");active=this;clientMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));toolMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));leafMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));
  clientMethods[72]=reinterpret_cast<uintptr_t>(&clientCreate);client=reinterpret_cast<uintptr_t>(clientMethods.data());word(0x4b5594,reinterpret_cast<uintptr_t>(&client));
  toolMethods[41]=reinterpret_cast<uintptr_t>(&enabled);toolMethods[44]=0x101e6ac0;toolMethods[47]=reinterpret_cast<uintptr_t>(&count);leafMethods[32]=reinterpret_cast<uintptr_t>(&toolCreate);
  const auto table=reinterpret_cast<uintptr_t>(toolMethods.data()),list=reinterpret_cast<uintptr_t>(leafPointers.data());std::memcpy(tools.data(),&table,4);std::memcpy(tools.data()+4,&list,4);for(int32_t i=0;i<4;++i){leaves[i]={reinterpret_cast<uintptr_t>(leafMethods.data()),i};leafPointers[i]=reinterpret_cast<uintptr_t>(&leaves[i]);}
  word(0x40c360,reinterpret_cast<uintptr_t>(tools.data()));word(0x31a690,reinterpret_cast<uintptr_t>(&warning));image.seal();
 }
 MaterialProxyFactoryRouteResultCandidate run(const Query&q){query=q;trace.clear();tools[0x30]=uint8_t(q.local);image.base[0x67039c]=uint8_t(q.global);image.base[0x60d610]=uint8_t(q.suppress);using Create=uintptr_t(__thiscall*)(void*,const char*);auto token=reinterpret_cast<Create>(0x1019d580)(image.base+0x4010f8,q.name.c_str());image.verify();return {token,trace.find("W ")!=std::string::npos};}
 ~EngineRouteFixture(){image.verify();VirtualFree(image.base,0,MEM_RELEASE);active=nullptr;}
};
static int ifmRegistry(const char*imagePath,const char*registrations){
 NativePrivateWorldImageFixture image(imagePath,0x100000);image.seal();std::ifstream input(registrations);need(bool(input),"IFM registrations");uint32_t callback=0,object=0,creator=0,name=0;unsigned count=0;
 while(input>>std::hex>>callback>>object>>creator>>name){using Register=void(__cdecl*)();reinterpret_cast<Register>(callback)();need(*reinterpret_cast<uint32_t*>(object)==creator&&reinterpret_cast<uint32_t*>(object)[1]==name,"real IFM interface registration");++count;}
 need(input.eof()&&count==14,"all14 IFM registrations");std::vector<uint32_t>seen;uint32_t head=*reinterpret_cast<uint32_t*>(image.base+0xfd4200);while(head){need(seen.size()<14,"IFM registry cycle");seen.push_back(head);head=reinterpret_cast<uint32_t*>(head)[2];}need(seen.size()==14,"complete native IFM registry list");
 // Keep original names/list and actual lookup code. Explicit spy creators
 // identify successful selection without running unmeasured shader objects.
 static unsigned selected=0;using Creator=uintptr_t(__cdecl*)();static auto spy=+[]()->uintptr_t{++selected;return 0x5151;};for(auto node:seen)*reinterpret_cast<uint32_t*>(node)=reinterpret_cast<uint32_t>(static_cast<Creator>(spy));
 using Find=uintptr_t(__cdecl*)(const char*,int*);auto find=reinterpret_cast<Find>(0x10980bb0);unsigned checks=0;
 using ToolCreate=uintptr_t(__thiscall*)(void*,const char*);auto toolCreate=reinterpret_cast<ToolCreate>(0x102bc2c0);unsigned bridges=0;
 for(auto node:seen){const auto*full=reinterpret_cast<const char*>(reinterpret_cast<uint32_t*>(node)[1]);int status=-1;auto before=selected;need(find(full,&status)==0x5151&&status==0&&selected==before+1,"real IFM registered lookup with explicit creator spy");++checks;std::string fullName=full;const std::string suffix="_IMaterialProxy003";if(fullName.size()>=suffix.size()&&fullName.substr(fullName.size()-suffix.size())==suffix){auto nameText=fullName.substr(0,fullName.size()-suffix.size());before=selected;need(toolCreate(nullptr,nameText.c_str())==0x5151&&selected==before+1,"whole original IFM name suffix/lookup bridge with spy creator");++checks;++bridges;}}
 for(const char*nameText:{"invis","INVIS","Invis","AnimatedWeaponSheen","animatedweaponsheen","weapon_invis","Equals","does_not_exist"}){auto full=std::string(nameText)+"_IMaterialProxy003";int status=-1;auto before=selected;need(!find(full.c_str(),&status)&&status==1&&selected==before,"real IFM fallback registry miss");need(!toolCreate(nullptr,nameText)&&selected==before,"whole original IFM bridge miss");checks+=2;std::cout<<"IFM_MISSING\t"<<full<<'\n';}
 image.verify();VirtualFree(image.base,0,MEM_RELEASE);std::cout<<"PASS ifm_registrations="<<count<<" checks="<<checks<<" bridge_hits="<<bridges<<" lookup=ORIGINAL creators=SPIES code=UNCHANGED_RX\n";return 0;
}
#else
static int safety(){unsigned checks=0;auto check=[&](bool ok){need(ok,"owned routing safety");++checks;};MaterialProxyFactoryRouteServicesCandidate s;check(!materialProxyFactoryRouteCandidate("invis",0,s));s.client=[](const auto&)->std::optional<uintptr_t>{return 0;};check(!materialProxyFactoryRouteCandidate("invis",0,s));s.toolsEnabled=[]()->std::optional<bool>{return true;};check(!materialProxyFactoryRouteCandidate("invis",0,s));s.toolCount=[]()->std::optional<int32_t>{return 1;};check(!materialProxyFactoryRouteCandidate("invis",0,s));s.tool=[](int32_t,const auto&)->std::optional<uintptr_t>{return {};};check(!materialProxyFactoryRouteCandidate("invis",0,s));s.tool=[](int32_t,const auto&)->std::optional<uintptr_t>{return 0;};check(materialProxyFactoryRouteCandidate("invis",0,s)->warning);s.toolsEnabled=[]()->std::optional<bool>{return {};};check(!materialProxyFactoryRouteCandidate("invis",0,s));s.toolsEnabled=[]()->std::optional<bool>{return false;};s.client=[](const auto&)->std::optional<uintptr_t>{throw std::runtime_error("provider");};check(!materialProxyFactoryRouteCandidate("invis",0,s));s.client=[](const auto&)->std::optional<uintptr_t>{return {};};check(!materialProxyFactoryRouteCandidate("invis",0,s));s.client=[](const auto&)->std::optional<uintptr_t>{return 1;};s.toolsEnabled=[]()->std::optional<bool>{throw std::bad_alloc();};check(!materialProxyFactoryRouteCandidate("invis",0,s));s.toolsEnabled=[]()->std::optional<bool>{return false;};for(auto name:{std::string(256,'a'),std::string("invis\0hidden",12),std::string("\xff",1)})check(!materialProxyFactoryRouteCandidate(name,0,s));s.client=[](const auto&)->std::optional<uintptr_t>{return 0;};s.toolsEnabled=[]()->std::optional<bool>{return true;};s.toolCount=[]()->std::optional<int32_t>{return 4097;};check(!materialProxyFactoryRouteCandidate("invis",0,s));s.toolCount=[]()->std::optional<int32_t>{return -1;};check(materialProxyFactoryRouteCandidate("invis",0,s)->warning);s.tool={};check(materialProxyFactoryRouteCandidate("invis",0,s)->warning);s.toolCount=[]()->std::optional<int32_t>{return 0;};check(materialProxyFactoryRouteCandidate("invis",0,s)->warning);s.toolCount=[]()->std::optional<int32_t>{return 1;};check(!materialProxyFactoryRouteCandidate("invis",0,s));std::cout<<"PASS owned_route_safety="<<checks<<" editor=NOT_CONNECTED\n";return 0;}
#endif
int main(int argc,char**argv){try{
#ifdef SFM64_NATIVE_PROXY_FACTORY_ROUTE
 static_assert(sizeof(void*)==4,"original reference only");SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);need(argc==4,"--engine/--ifm private-image query");if(std::string(argv[1])=="--ifm")return ifmRegistry(argv[2],argv[3]);need(std::string(argv[1])=="--engine","engine mode");auto rows=queries(argv[3]);EngineRouteFixture f(argv[2]);
#else
 static_assert(sizeof(void*)==8,"owned native x64");if(argc==1)return safety();need(argc==2,"query file");auto rows=queries(argv[1]);
#endif
 unsigned index=0;for(const auto&q:rows){
#ifdef SFM64_NATIVE_PROXY_FACTORY_ROUTE
  auto result=f.run(q);const auto&trace=f.trace;
#else
  std::string trace;MaterialProxyFactoryRouteServicesCandidate s;s.client=[&](const auto&name)->std::optional<uintptr_t>{need(name==q.name,"owned client name");trace+="C ";return q.client;};s.toolsEnabled=[&]()->std::optional<bool>{trace+="E ";return bool(q.local||q.global);};s.toolCount=[&]()->std::optional<int32_t>{trace+="N ";return q.count;};s.tool=[&](int32_t i,const auto&name)->std::optional<uintptr_t>{need(name==q.name,"owned tool name");trace+="T"+std::to_string(i)+" ";return i==q.hit?uintptr_t(2+i):0;};auto attempted=materialProxyFactoryRouteCandidate(q.name,uint8_t(q.suppress),s);need(bool(attempted),"owned supported route");auto result=*attempted;if(result.warning)trace+="W ";
#endif
  std::cout<<"CASE\t"<<index++<<'\t'<<result.token<<'\t'<<result.warning<<'\t'<<trace<<'\n';
 }std::cout<<"PASS route_cases="<<rows.size()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

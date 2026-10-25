#include "vertexlit_shared_init_candidate.h"
#include "material_var_defined_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <cstdarg>
#ifdef SFM64_NATIVE_VERTEXLIT_SHARED_INIT
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
#endif
using P=VertexLitInitParamCandidate;
static constexpr size_t count=125;
static constexpr std::array<const char*,6>strings{"","fixture/base","fixture/albedo",
    "fixture/custom-flashlight","effects/flashlight001","fixture/other"};
struct State{uint32_t type,integer;std::array<uint32_t,4>vec;uint32_t text;};
static_assert(sizeof(State)==28,"wire state");
using Event=std::array<uint32_t,8>;
struct Access;
#ifdef SFM64_NATIVE_VERTEXLIT_SHARED_INIT
struct NativeVar{uintptr_t table;uint32_t id,integer;std::array<uint32_t,4>vec;uint32_t type;};
static_assert(offsetof(NativeVar,type)==0x1c&&sizeof(NativeVar)==32,"native input fields");
static std::array<NativeVar,count>vars{};
#endif
struct Access final:VertexLitInitAccessCandidate{
    std::array<State,count>states{};std::vector<Event>events;int32_t hardware=92;size_t schemaCount=count;
    size_t parameterCount()const override{return schemaCount;}
    static uint32_t bits(float v){uint32_t r;std::memcpy(&r,&v,4);return r;}
    void trace(uint32_t kind,uint32_t id,uint32_t a=0,uint32_t b=0,uint32_t c=0,uint32_t d=0){
        if(events.size()>=256)throw std::runtime_error("trace bound");events.push_back({kind,id,a,b,c,d,0,0});
    }
    State&state(P p){return states.at(size_t(p));}
    void sync(P p){
#ifdef SFM64_NATIVE_VERTEXLIT_SHARED_INIT
        auto&n=vars.at(size_t(p));const auto&s=state(p);n.integer=s.integer;n.vec=s.vec;n.type=s.type;
#else
        (void)p;
#endif
    }
    bool defined(P p)override{const bool value=materialVarDefinedTypeCandidate(uint8_t(state(p).type));trace(1,uint32_t(p),value);return value;}
    uint32_t integer(P p)const override{return states.at(size_t(p)).integer;}
    uint8_t typeByte(P p)const override{return uint8_t(states.at(size_t(p)).type);}
    std::string_view string(P p)override{const auto id=state(p).text;trace(3,uint32_t(p),id);return strings.at(id);}
    void setString(P p,std::string_view name)override{
        uint32_t id=0;for(;id<strings.size()&&name!=strings[id];++id){}
        if(id==strings.size())throw std::runtime_error("fixture string domain");
        auto&s=state(p);s.text=id;s.type=(s.type&0xf0)|1;trace(2,uint32_t(p),id);sync(p);
    }
    void setInt(P p,uint32_t n)override{auto&s=state(p);s.integer=n;s.vec[0]=bits(float(int32_t(n)));s.type=(s.type&0xf0)|4;trace(4,uint32_t(p),n);sync(p);}
    void setFloat(P p,float n)override{auto&s=state(p);s.vec[0]=bits(n);s.integer=uint32_t(int32_t(n));s.type&=0xf0;trace(5,uint32_t(p),bits(n));sync(p);}
    void setVector3(P p,float a,float b,float c)override{auto&s=state(p);s.vec[0]=bits(a);s.vec[1]=bits(b);s.vec[2]=bits(c);s.type=(s.type&0xf0)|2;trace(6,uint32_t(p),bits(a),bits(b),bits(c));sync(p);}
    void setVector4(P p,float a,float b,float c,float d)override{auto&s=state(p);s.vec={bits(a),bits(b),bits(c),bits(d)};s.type=(s.type&0xf0)|2;trace(7,uint32_t(p),bits(a),bits(b),bits(c),bits(d));sync(p);}
    void unset(P p)override{auto&s=state(p);s.type=(s.type&0xf0)|6;trace(8,uint32_t(p));sync(p);}
    int32_t hardwareLevel()override{trace(9,125,uint32_t(hardware));return hardware;}
    void warning(unsigned n)override{if(n<1||n>2)throw std::runtime_error("warning domain");trace(10,125,n);}
};
#ifdef SFM64_NATIVE_VERTEXLIT_SHARED_INIT
static Access*active=nullptr;static bool tools=false,seenTools=false;
static std::array<uintptr_t,4>commandTable{};static uintptr_t commandObject=0;
static P id(void*self){return P(static_cast<NativeVar*>(self)->id);}
static void __fastcall unexpectedSharedCallback(void*,void*){ExitProcess(92);}
static uint8_t __fastcall isDefined(void*self,void*){return active->defined(id(self));}
static void __fastcall setInt(void*self,void*,uint32_t n){active->setInt(id(self),n);}
static void __fastcall setFloat(void*self,void*,float n){active->setFloat(id(self),n);}
static void __fastcall setString(void*self,void*,const char*n){active->setString(id(self),n);}
static const char* __fastcall getString(void*self,void*){return active->string(id(self)).data();}
static void __fastcall setVec3(void*self,void*,float a,float b,float c){active->setVector3(id(self),a,b,c);}
static void __fastcall setVec4(void*self,void*,float a,float b,float c,float d){active->setVector4(id(self),a,b,c,d);}
static void __fastcall unset(void*self,void*){active->unset(id(self));}
static int32_t __fastcall hardware(void*,void*){return active->hardwareLevel();}
static int __fastcall checkTools(void*,void*,const char*key,void*value){
    if(!key||std::string_view(key)!="-tools"||value)ExitProcess(93);seenTools=true;return tools?7:0;
}
static void* __cdecl getCommandLine(){return &commandObject;}
static void __cdecl warning(const char*format,...){
    va_list args;va_start(args,format);const char*name=va_arg(args,const char*);va_end(args);
    if(!name||std::string_view(name)!="fixture")ExitProcess(94);
    if(reinterpret_cast<uintptr_t>(format)==0x1009bdd0)active->warning(1);
    else if(reinterpret_cast<uintptr_t>(format)==0x1009bd78)active->warning(2);
    else ExitProcess(95);
}
#endif
static void analytical(){
    auto initial=[](){Access v;for(auto&s:v.states)s={6,0,{0,0,0,0},1};return v;};
    unsigned checks=0;auto need=[&](bool b){if(!b)throw std::runtime_error("shared init analytical check");++checks;};
    auto v=initial();auto r=vertexLitSharedInitCandidate(v,{});need(r==VertexLitInitRouteCandidate::Generic);
    need(v.states[48].integer==0&&v.states[9].text==4);need(v.states[111].vec[0]==Access::bits(1000));
    need(v.states[107].vec==std::array<uint32_t,4>{0,0x3f800000,0x40000000,0});
    v=initial();v.states[48].type=4;v.states[48].integer=1;v.states[43].type=3;
    VertexLitInitSettingsCandidate s;s.matPhong=1;s.toolsSwitchPresent=true;
    r=vertexLitSharedInitCandidate(v,s);need(r==VertexLitInitRouteCandidate::Phong);need(v.states[100].vec[0]==Access::bits(1));
    v=initial();v.states[48].type=4;v.states[48].integer=1;v.states[43].type=3;v.hardware=91;
    r=vertexLitSharedInitCandidate(v,s);need(r==VertexLitInitRouteCandidate::Generic&&v.states[48].integer==0);
    v=initial();s={};s.forceVertexFog=1;vertexLitSharedInitCandidate(v,s);need((v.states[0].integer&0x80000000)!=0);
    v=initial();v.states[1].integer=0x80000000;vertexLitSharedInitCandidate(v,s);need((v.states[0].integer&0x80000000)==0);
    v=initial();v.schemaCount=124;const auto before=v.states;bool refused=false;
    try{vertexLitSharedInitCandidate(v,{});}catch(const std::runtime_error&){refused=true;}
    need(refused&&v.events.empty()&&std::memcmp(v.states.data(),before.data(),sizeof(before))==0);
    std::cout<<"PASS shared_init_analytical_checks="<<checks<<" schema_refusals=1\n";
}
int main(int argc,char**argv){try{
    if(argc==2&&std::string_view(argv[1])=="--test"){analytical();return 0;}
    if(argc!=5||std::ifstream(argv[4],std::ios::binary).good())throw std::runtime_error("library info queries fresh-output required");
    std::array<uint32_t,110>info{};std::ifstream mapping(argv[2],std::ios::binary);mapping.read(reinterpret_cast<char*>(info.data()),sizeof(info));
    if(!mapping||mapping.peek()!=std::char_traits<char>::eof())throw std::runtime_error("fixed family mapping extent");
    for(auto n:info)if(n!=0xffffffffu&&n>=125)throw std::runtime_error("mapping bound");const auto beforeInfo=info;
    std::ifstream in(argv[3],std::ios::binary);std::ofstream out(argv[4],std::ios::binary);if(!in||!out)throw std::runtime_error("shared init IO");
#ifdef SFM64_NATIVE_VERTEXLIT_SHARED_INIT
    static_assert(sizeof(void*)==4,"private test oracle");NativePrivateWorldImageFixture image(argv[1],0xf0000);
    std::array<uintptr_t,19>table{};table.fill(reinterpret_cast<uintptr_t>(&unexpectedSharedCallback));
    table[3]=reinterpret_cast<uintptr_t>(&setFloat);table[4]=reinterpret_cast<uintptr_t>(&setInt);table[5]=reinterpret_cast<uintptr_t>(&setString);
    table[6]=reinterpret_cast<uintptr_t>(&getString);table[9]=reinterpret_cast<uintptr_t>(&setVec4);table[10]=reinterpret_cast<uintptr_t>(&setVec3);
    table[17]=reinterpret_cast<uintptr_t>(&isDefined);table[18]=reinterpret_cast<uintptr_t>(&unset);
    std::array<uintptr_t,18>hwTable{};hwTable.fill(reinterpret_cast<uintptr_t>(&unexpectedSharedCallback));hwTable[17]=reinterpret_cast<uintptr_t>(&hardware);
    uintptr_t hwObject=reinterpret_cast<uintptr_t>(hwTable.data());
    std::array<uint32_t,32>config{},phong{},fog{};
    auto pointer=[&](size_t offset,const void*p){const auto word=reinterpret_cast<uintptr_t>(p);std::memcpy(image.base+offset,&word,4);};
    pointer(0xdb9dc,config.data());pointer(0xcaff4,phong.data());pointer(0xcb054,fog.data());pointer(0xdb9d8,&hwObject);
    commandTable.fill(reinterpret_cast<uintptr_t>(&unexpectedSharedCallback));commandTable[3]=reinterpret_cast<uintptr_t>(&checkTools);commandObject=reinterpret_cast<uintptr_t>(commandTable.data());
    const auto commandImport=reinterpret_cast<uintptr_t>(&getCommandLine),warnImport=reinterpret_cast<uintptr_t>(&warning);
    std::memcpy(image.base+0x890fc,&commandImport,4);std::memcpy(image.base+0x890f8,&warnImport,4);image.seal();
#else
    static_assert(sizeof(void*)==8,"owned x64");
#endif
    // Six settings words +125 seven-word variable states =3524 bytes.
    std::array<uint32_t,881>q{};size_t cases=0,events=0,phongCases=0;
    while(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))){
        if(q[4]>1||q[5]>1)throw std::runtime_error("boolean setting domain");
        Access v;std::memcpy(v.states.data(),q.data()+6,sizeof(v.states));std::memcpy(&v.hardware,&q[3],4);
        for(const auto&state:v.states)if(state.type>255||state.text>=strings.size())throw std::runtime_error("variable input domain");
        VertexLitInitSettingsCandidate s;s.configFlagsAt34=q[0];std::memcpy(&s.matPhong,&q[1],4);std::memcpy(&s.forceVertexFog,&q[2],4);s.vertexLit=q[4]!=0;s.toolsSwitchPresent=q[5]!=0;
        uint32_t route=0;
#ifdef SFM64_NATIVE_VERTEXLIT_SHARED_INIT
        std::array<NativeVar*,count>params{};
        for(uint32_t i=0;i<count;++i){const auto&state=v.states[i];vars[i]={reinterpret_cast<uintptr_t>(table.data()),i,state.integer,state.vec,state.type};params[i]=&vars[i];}
        config[13]=q[0];phong[12]=q[1];fog[12]=q[2];const auto cb=config,pb=phong,fb=fog;
        tools=s.toolsSwitchPresent;seenTools=false;image.base[0xbebdc]=0;image.base[0xbebd8]=0xa5;active=&v;
        using Fn=void(__cdecl*)(void*,NativeVar**,const char*,bool,uint32_t*);
        reinterpret_cast<Fn>(image.base+0x54490)(nullptr,params.data(),"fixture",s.vertexLit,info.data());active=nullptr;
        route=seenTools?1u:0u;if(config!=cb||phong!=pb||fog!=fb||info!=beforeInfo)throw std::runtime_error("native context mutation");
        for(size_t i=0;i<count;++i)if(vars[i].id!=i||vars[i].table!=reinterpret_cast<uintptr_t>(table.data())||
            vars[i].integer!=v.states[i].integer||vars[i].vec!=v.states[i].vec||vars[i].type!=v.states[i].type)throw std::runtime_error("native variable mutation outside callbacks");
#else
        route=uint32_t(vertexLitSharedInitCandidate(v,s));
#endif
        const uint32_t n=uint32_t(v.events.size());out.write(reinterpret_cast<const char*>(&route),4);out.write(reinterpret_cast<const char*>(&n),4);
        out.write(reinterpret_cast<const char*>(v.states.data()),sizeof(v.states));out.write(reinterpret_cast<const char*>(v.events.data()),std::streamsize(n*sizeof(Event)));
        ++cases;events+=n;phongCases+=route;
    }
    if(!in.eof()||in.gcount()||!cases)throw std::runtime_error("shared input framing");out.flush();if(!out)throw std::runtime_error("shared output write");
#ifdef SFM64_NATIVE_VERTEXLIT_SHARED_INIT
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS shared_init_cases="<<cases<<" phong="<<phongCases<<" generic="<<cases-phongCases<<" events="<<events<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

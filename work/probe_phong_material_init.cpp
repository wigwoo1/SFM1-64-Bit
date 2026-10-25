#include "phong_material_init_candidate.h"
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#ifdef SFM64_NATIVE_PHONG_INIT
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
#endif

using Param=PhongInitParamCandidate;
static constexpr size_t paramCount=size_t(Param::Count);
// Fixture string identity is serialized as a token, never as a native pointer.
static constexpr std::array<const char*,6>strings{"","fixture/base","fixture/albedo",
    "fixture/custom-flashlight","effects/flashlight001","fixture/other"};
struct State{uint32_t declared,defined,integer,scalar;std::array<uint32_t,3>vector;uint32_t text;};
static_assert(sizeof(State)==32&&paramCount==19,"fixture wire layout");
using Event=std::array<uint32_t,8>;
struct Access:PhongInitAccessCandidate{
    std::array<State,paramCount>states{};std::vector<Event>events;
    static uint32_t bits(float value){uint32_t word;std::memcpy(&word,&value,4);return word;}
    void event(uint32_t kind,Param p,uint32_t a=0,uint32_t b=0,uint32_t c=0){
        if(events.size()>=128)throw std::runtime_error("callback trace bound");
        events.push_back({kind,uint32_t(p),a,b,c,0,0,0});
    }
    State&state(Param p){auto&s=states.at(size_t(p));if(!s.declared)throw std::runtime_error("undeclared variable accessed");return s;}
    bool declared(Param p)const override{return states.at(size_t(p)).declared!=0;}
    bool defined(Param p)override{const bool result=state(p).defined!=0;event(1,p,uint32_t(result));return result;}
    uint32_t integer(Param p)const override{return states.at(size_t(p)).integer;}
    std::string_view string(Param p)override{auto token=state(p).text;event(3,p,token);return strings.at(token);}
    void setString(Param p,std::string_view value)override{
        uint32_t token=0;for(;token<strings.size()&&value!=strings[token];++token){}
        if(token==strings.size())throw std::runtime_error("fixture string not admitted");
        auto&s=state(p);s.text=token;s.defined=1;event(2,p,token);
    }
    void setInt(Param p,uint32_t value)override{auto&s=state(p);s.integer=value;s.defined=1;event(4,p,value);}
    void setFloat(Param p,float value)override{auto&s=state(p);s.scalar=bits(value);s.defined=1;event(5,p,s.scalar);}
    void setVector3(Param p,float a,float b,float c)override{auto&s=state(p);s.vector={bits(a),bits(b),bits(c)};s.defined=1;event(6,p,s.vector[0],s.vector[1],s.vector[2]);}
};
#ifdef SFM64_NATIVE_PHONG_INIT
struct NativeVar{uintptr_t table;uint32_t unused;uint32_t integer;uint32_t scalar;uint32_t id;};
static_assert(offsetof(NativeVar,integer)==8,"native direct integer input");
static Access*current=nullptr;
static bool toolsPresent=false;static size_t toolsLookupCalls=0;
static std::array<uintptr_t,4>commandTable{};static uintptr_t commandObject=0;
static int __fastcall checkTools(void*,void*,const char*key,void*value){
    if(!key||std::string_view(key)!="-tools"||value)ExitProcess(93);
    ++toolsLookupCalls;return toolsPresent?7:0;
}
static void* __cdecl getCommandLine(){return &commandObject;}
static Param param(void*self){return Param(static_cast<NativeVar*>(self)->id);}
static void __fastcall unexpectedPhongInitCallback(void*,void*){ExitProcess(92);}
static uint8_t __fastcall isDefined(void*self,void*){return current->defined(param(self));}
static const char* __fastcall getString(void*self,void*){const auto result=current->string(param(self));return result.data();}
static void __fastcall setString(void*self,void*,const char*value){current->setString(param(self),value);}
static void __fastcall setInt(void*self,void*,uint32_t value){current->setInt(param(self),value);static_cast<NativeVar*>(self)->integer=value;}
static void __fastcall setFloat(void*self,void*,float value){current->setFloat(param(self),value);static_cast<NativeVar*>(self)->scalar=Access::bits(value);}
static void __fastcall setVector3(void*self,void*,float a,float b,float c){current->setVector3(param(self),a,b,c);}
#endif

static void analytical(){
    auto initial=[](){Access a;for(auto&s:a.states)s={1,0,0x501000,0x3e800000,{1,2,3},1};return a;};
    unsigned checks=0,refusals=0;
    auto need=[&](bool b){if(!b)throw std::runtime_error("analytical Phong init check");++checks;};
    auto a=initial();phongMaterialInitCandidate(a,0,false);need(a.states[2].text==4);
    need((a.states[1].integer&0x40042)==0x40042);need(a.states[0].integer==0x1000);
    need(a.states[9].vector==std::array<uint32_t,3>{0,0x3f800000,0x3f800000});
    need(a.states[11].scalar==0&&a.states[17].integer==1);
    a=initial();phongMaterialInitCandidate(a,0,true);need(a.states[11].scalar==0x3f800000);
    a=initial();for(size_t i:{size_t(3),size_t(4),size_t(5)})a.states[i].defined=1;a.states[4].text=2;
    phongMaterialInitCandidate(a,0,false);need(a.states[3].text==2);
    a=initial();for(size_t i:{size_t(3),size_t(4),size_t(5)})a.states[i].defined=1;a.states[4].text=2;
    phongMaterialInitCandidate(a,0x100,false);need(a.states[3].text==1);
    a=initial();a.states[8].defined=1;a.states[8].integer=0;phongMaterialInitCandidate(a,0,false);
    need((a.states[0].integer&0x400000)!=0); // IsDefined, not integer truth.
    a=initial();a.states[0].integer=0x10000;phongMaterialInitCandidate(a,0,false);need((a.states[0].integer&2)!=0);
    a=initial();for(auto&s:a.states)s.defined=1;const auto prior=a.states;phongMaterialInitCandidate(a,0,false);
    need(a.states[11].scalar==prior[11].scalar&&a.states[14].integer==prior[14].integer);
    for(size_t i:{size_t(0),size_t(1),size_t(2),size_t(3)}){
        a=initial();a.states[i].declared=0;const auto before=a.states;
        bool refused=false;try{phongMaterialInitCandidate(a,0,false);}
        catch(const std::runtime_error&){refused=true;}
        need(refused&&std::memcmp(a.states.data(),before.data(),sizeof(before))==0&&a.events.empty());++refusals;
    }
    std::cout<<"PASS analytical_checks="<<checks<<" required_variable_refusals="<<refusals<<'\n';
}
int main(int argc,char**argv){try{
    if(argc==2&&std::string_view(argv[1])=="--test"){analytical();return 0;}
    if(argc!=4||std::ifstream(argv[3],std::ios::binary).good())throw std::runtime_error("private-image queries fresh-output required");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);
    if(!in||!out)throw std::runtime_error("Phong init fixture IO");
#ifdef SFM64_NATIVE_PHONG_INIT
    static_assert(sizeof(void*)==4,"test oracle only");NativePrivateWorldImageFixture image(argv[1],0xf0000);
    std::array<uint32_t,32>config{};const auto configPointer=reinterpret_cast<uintptr_t>(config.data());std::memcpy(image.base+0xdb9dc,&configPointer,4);image.seal();
    std::array<uintptr_t,18>table{};table.fill(reinterpret_cast<uintptr_t>(&unexpectedPhongInitCallback));
    table[3]=reinterpret_cast<uintptr_t>(&setFloat);table[4]=reinterpret_cast<uintptr_t>(&setInt);
    table[5]=reinterpret_cast<uintptr_t>(&setString);table[6]=reinterpret_cast<uintptr_t>(&getString);
    table[10]=reinterpret_cast<uintptr_t>(&setVector3);table[17]=reinterpret_cast<uintptr_t>(&isDefined);
    commandTable.fill(reinterpret_cast<uintptr_t>(&unexpectedPhongInitCallback));commandTable[3]=reinterpret_cast<uintptr_t>(&checkTools);commandObject=reinterpret_cast<uintptr_t>(commandTable.data());
    const auto commandImport=reinterpret_cast<uintptr_t>(&getCommandLine);std::memcpy(image.base+0x890fc,&commandImport,4);
#else
    static_assert(sizeof(void*)==8,"owned native x64");
#endif
    // Query: config flags, tools bool, cache bool +19 eight-word states (620B).
    std::array<uint32_t,155>query{};size_t cases=0,totalEvents=0;
    while(in.read(reinterpret_cast<char*>(query.data()),sizeof(query))){
        if(query[1]>1||query[2]>1)throw std::runtime_error("tools helper input domain");
        Access access;std::memcpy(access.states.data(),query.data()+3,sizeof(access.states));
        for(auto&s:access.states)if(s.declared>1||s.defined>1||s.text>=strings.size())throw std::runtime_error("Phong init input domain");
        for(size_t i=0;i<4;++i)if(!access.states[i].declared)throw std::runtime_error("required variable absent");
#ifdef SFM64_NATIVE_PHONG_INIT
        std::array<NativeVar,paramCount>vars{};std::array<NativeVar*,125>params{};
        const std::array<uint32_t,paramCount>indices{0,2,9,6,13,26,20,43,48,35,51,100,103,34,49,53,99,106,109};
        for(uint32_t i=0;i<paramCount;++i){vars[i]={reinterpret_cast<uintptr_t>(table.data()),0,access.states[i].integer,access.states[i].scalar,i};params[indices[i]]=&vars[i];}
        std::array<uint32_t,110>info{};info.fill(0xffffffffu);
        const std::array<uint32_t,paramCount>offsets{0xffffffff,0xffffffff,0xffffffff,0,0x14,0x3c,0x24,0x8c,0xa8,0x74,0xb4,0x158,0x174,0x70,0xac,0xb0,0x154,0x164,0x178};
        for(uint32_t i=3;i<paramCount;++i)if(access.states[i].declared)info[offsets[i]/4]=indices[i];
        const auto beforeInfo=info;
        config[0x34/4]=query[0];const auto configBefore=config;current=&access;
        toolsPresent=query[1]!=0;std::memcpy(image.base+0xbebdc,&query[2],4);image.base[0xbebd8]=query[2]?uint8_t(query[1]):0xa5;
        using Fn=void(__cdecl*)(void*,NativeVar**,const char*,uint32_t*);
        reinterpret_cast<Fn>(image.base+0x37850)(nullptr,params.data(),"fixture",info.data());current=nullptr;
        if(image.base[0xbebd8]!=query[1]||!(image.base[0xbebdc]&1))throw std::runtime_error("tools helper cache state");
        if(config!=configBefore)throw std::runtime_error("native config mutation");
        if(info!=beforeInfo)throw std::runtime_error("native declaration mutation");
        for(size_t i=0;i<paramCount;++i)if(vars[i].integer!=access.states[i].integer||
            vars[i].scalar!=access.states[i].scalar||vars[i].id!=i||vars[i].unused||
            vars[i].table!=reinterpret_cast<uintptr_t>(table.data()))throw std::runtime_error("direct native state mismatch");
#else
        phongMaterialInitCandidate(access,query[0],query[1]!=0);
#endif
        const uint32_t count=uint32_t(access.events.size());out.write(reinterpret_cast<const char*>(&count),4);
        out.write(reinterpret_cast<const char*>(access.states.data()),sizeof(access.states));
        out.write(reinterpret_cast<const char*>(access.events.data()),std::streamsize(count*sizeof(Event)));
        ++cases;totalEvents+=count;
    }
    if(!in.eof()||in.gcount()||!cases)throw std::runtime_error("Phong init query framing");
    out.flush();if(!out)throw std::runtime_error("Phong init output");
#ifdef SFM64_NATIVE_PHONG_INIT
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
    std::cout<<"tools_lookup_calls="<<toolsLookupCalls<<'\n';
#endif
    std::cout<<"PASS phong_init_cases="<<cases<<" callback_events="<<totalEvents<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

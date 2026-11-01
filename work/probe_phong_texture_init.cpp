// Complete private original texture-init stage and guarded wrappers vs owned x64.
// All texture services are owned test callbacks. No real texture/GPU/SFM access.
#define NOMINMAX
#include <windows.h>
#include "phong_texture_init_candidate.h"
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#endif
using P=PhongTextureParamCandidate;using K=PhongTextureLoadKindCandidate;
static constexpr size_t N=size_t(P::Count);
struct State{uint32_t declared,definedResult,integer,textureResult,loadedTextureResult;};
static_assert(sizeof(State)==20&&N==19,"wire state");
using Event=std::array<uint32_t,4>;
struct Access:PhongTextureInitAccessCandidate {
    std::array<State,N>states{};std::vector<Event>events;uint32_t hdr=0;
    void event(uint32_t kind,P p,uint32_t a=0,uint32_t b=0){
        if(events.size()>=128)throw std::runtime_error("event bound");events.push_back({kind,uint32_t(p),a,b});
    }
    bool declared(P p)const override{return states.at(size_t(p)).declared!=0;}
    bool defined(P p)override{const auto&s=states.at(size_t(p));if(!s.declared)throw std::runtime_error("absent definition access");
        const bool result=(s.definedResult&255u)!=0;event(1,p,uint32_t(result));return result;}
    uint32_t integer(P p)const override{return states.at(size_t(p)).integer;}
    void setInteger(P p,uint32_t x)override{states.at(size_t(p)).integer=x;event(2,p,x);}
    bool baseTextureQuery()override{event(3,P::Base);const bool b=(states[size_t(P::Base)].textureResult&255u)!=0;event(4,P::Base,uint32_t(b));return b;}
    uint32_t hdrQuery()override{event(5,P::EnvMap,hdr);return hdr;}
    void load(K k,P p,uint32_t flags)override{event(6,p,uint32_t(k),flags);auto&s=states.at(size_t(p));s.textureResult=s.loadedTextureResult;}
};
#ifndef _WIN64
struct NativeVar{uintptr_t table;uint32_t unused,integer,pad,id;};
static_assert(offsetof(NativeVar,integer)==8,"native raw integer");
static Access*current=nullptr;static std::array<NativeVar,N>*nativeVars=nullptr;
static void*servicePointer=nullptr;static void*hardwarePointer=nullptr;static void*paramsPointer=nullptr;
static constexpr char groupName[]="controlled-fixture-group";
static P param(void*self){auto*v=static_cast<NativeVar*>(self);if(!nativeVars||v<nativeVars->data()||v>=nativeVars->data()+N||v->id>=N)ExitProcess(92);return P(v->id);}
static void __fastcall unexpectedTextureInitCallback(void*,void*){ExitProcess(93);}
static uint32_t __fastcall defined(void*self,void*){const auto p=param(self);current->defined(p);return current->states[size_t(p)].definedResult;}
static void __fastcall setInteger(void*self,void*,uint32_t x){const auto p=param(self);current->setInteger(p,x);static_cast<NativeVar*>(self)->integer=x;}
static std::array<uintptr_t,7>textureTable{};static uintptr_t textureObject=0;
static uint32_t __fastcall textureQuery(void*self,void*){if(self!=&textureObject)ExitProcess(94);
    const auto raw=current->states[size_t(P::Base)].textureResult;current->event(4,P::Base,uint32_t((raw&255u)!=0));return raw;}
static void* __fastcall getTexture(void*self,void*){if(param(self)!=P::Base)ExitProcess(94);current->event(3,P::Base);return &textureObject;}
static uint32_t __fastcall hdrQuery(void*self,void*){if(self!=hardwarePointer)ExitProcess(95);return current->hdrQuery();}
static void __fastcall loadTexture(void*self,void*,NativeVar*v,const char*group,uint32_t flags){
    if(self!=servicePointer||group!=groupName)ExitProcess(96);current->load(K::Texture,param(v),flags);
}
static void __fastcall loadBump(void*self,void*,NativeVar*v,const char*group){
    if(self!=servicePointer||group!=groupName)ExitProcess(97);current->load(K::Bump,param(v),0);
}
static void __fastcall loadCube(void*self,void*,void*params,NativeVar*v,uint32_t flags){
    if(self!=servicePointer||params!=paramsPointer)ExitProcess(98);current->load(K::Cube,param(v),flags);
}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xc0000005){std::cerr<<"PRIVATE_FAULT eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(89);}return EXCEPTION_CONTINUE_SEARCH;}
#endif
int main(int argc,char**argv){try{
    if(argc!=4||std::ifstream(argv[3]).good())throw std::runtime_error("usage: stdshader queries fresh-output");
    std::ifstream in(argv[2],std::ios::binary);if(!in)throw std::runtime_error("query input");
    std::ofstream out(argv[3],std::ios::binary);if(!out)throw std::runtime_error("trace output");
#ifndef _WIN64
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);auto handler=AddVectoredExceptionHandler(0,&fault);
    NativePrivateWorldImageFixture image(argv[1],0x1000);
    std::array<uintptr_t,18>varTable{};varTable.fill(reinterpret_cast<uintptr_t>(&unexpectedTextureInitCallback));
    varTable[0]=reinterpret_cast<uintptr_t>(&getTexture);varTable[4]=reinterpret_cast<uintptr_t>(&setInteger);varTable[17]=reinterpret_cast<uintptr_t>(&defined);
    // Texture receiver is a separate object; preserve native GetTextureValue/query order.
    textureTable.fill(reinterpret_cast<uintptr_t>(&unexpectedTextureInitCallback));textureTable[6]=reinterpret_cast<uintptr_t>(&textureQuery);textureObject=reinterpret_cast<uintptr_t>(textureTable.data());
    std::array<uintptr_t,3>serviceTable{reinterpret_cast<uintptr_t>(&loadTexture),reinterpret_cast<uintptr_t>(&loadBump),reinterpret_cast<uintptr_t>(&loadCube)};
    uintptr_t serviceObject=reinterpret_cast<uintptr_t>(serviceTable.data());servicePointer=&serviceObject;
    std::array<uintptr_t,35>hardwareTable{};hardwareTable.fill(reinterpret_cast<uintptr_t>(&unexpectedTextureInitCallback));hardwareTable[34]=reinterpret_cast<uintptr_t>(&hdrQuery);
    uintptr_t hardwareObject=reinterpret_cast<uintptr_t>(hardwareTable.data());hardwarePointer=&hardwareObject;
    std::array<uint32_t,32>config{};
    auto put=[&](size_t offset,uintptr_t value){std::memcpy(image.base+offset,&value,4);};
    put(0xdb950,reinterpret_cast<uintptr_t>(servicePointer));put(0xdb954,reinterpret_cast<uintptr_t>(groupName));
    put(0xdb9d8,reinterpret_cast<uintptr_t>(hardwarePointer));put(0xdb9dc,reinterpret_cast<uintptr_t>(config.data()));image.seal();
#else
    static_assert(sizeof(void*)==8,"owned x64");(void)argv[1];
#endif
    // serial, config, raw HDR query, then 19 five-word explicit states.
    std::array<uint32_t,3+N*5>query{};uint32_t cases=0,totalEvents=0;
    while(in.read(reinterpret_cast<char*>(query.data()),sizeof(query))){
        if(query[0]!=cases)throw std::runtime_error("serial");const auto before=query;
        Access a;a.hdr=query[2];std::memcpy(a.states.data(),query.data()+3,sizeof(a.states));
        for(const auto&s:a.states)if(s.declared>1)throw std::runtime_error("declaration domain");
        for(auto p:{P::Flags,P::Flags2,P::Base,P::EnvMap})if(!a.declared(p))throw std::runtime_error("required index absent");
#ifndef _WIN64
        std::array<NativeVar,N>vars{};std::array<NativeVar*,125>params{};std::array<uint32_t,110>info{};info.fill(0xffffffffu);
        const uint32_t indices[N]={0,2,9,6,14,15,47,48,43,44,17,54,26,27,28,20,34,52,102};
        const uint32_t offsets[N]={0xffffffff,0xffffffff,0x64,0,4,8,0x88,0xa8,0x8c,0x90,0x18,0xb8,0x3c,0x40,0x44,0x24,0x70,0x148,0x170};
        for(uint32_t i=0;i<N;++i){vars[i]={reinterpret_cast<uintptr_t>(varTable.data()),0,a.states[i].integer,0,i};params[indices[i]]=&vars[i];if(i>=2&&a.states[i].declared)info[offsets[i]/4]=indices[i];}
        const auto varsBefore=vars;const auto paramsBefore=params;const auto infoBefore=info;
        config.fill(0xa5a5a5a5);config[0x34/4]=query[1];const auto configBefore=config;
        paramsPointer=params.data();put(0xdb944,reinterpret_cast<uintptr_t>(paramsPointer));current=&a;nativeVars=&vars;
        using Fn=void(__cdecl*)(void*,NativeVar**,uint32_t*);
        reinterpret_cast<Fn>(image.base+0x37bd0)(servicePointer,params.data(),info.data());current=nullptr;nativeVars=nullptr;
        if(params!=paramsBefore||info!=infoBefore||config!=configBefore)throw std::runtime_error("native inputs changed");
        for(size_t i=0;i<N;++i){auto expected=varsBefore[i];expected.integer=a.states[i].integer;if(std::memcmp(&vars[i],&expected,sizeof(expected)))throw std::runtime_error("native variable guard");}
#else
        phongTextureInitCandidate(a,query[1]);
#endif
        if(query!=before)throw std::runtime_error("query changed");
        const std::array<uint32_t,2>header{cases,uint32_t(a.events.size())};out.write(reinterpret_cast<const char*>(header.data()),sizeof(header));
        out.write(reinterpret_cast<const char*>(a.states.data()),sizeof(a.states));out.write(reinterpret_cast<const char*>(a.events.data()),std::streamsize(a.events.size()*sizeof(Event)));
        ++cases;totalEvents+=uint32_t(a.events.size());
    }
    if(!in.eof()||in.gcount()||!cases)throw std::runtime_error("query framing");out.flush();if(!out)throw std::runtime_error("trace write");
#ifndef _WIN64
    image.verify();RemoveVectoredExceptionHandler(handler);VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS texture_init_cases="<<cases<<" ordered_events="<<totalEvents<<" real_texture_loads=0 gpu_calls=0\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

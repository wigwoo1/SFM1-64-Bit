#define NOMINMAX
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "texture_format_policy_candidate.h"
#ifndef _WIN64
#include <windows.h>
#include "material_construction_native_fixture.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
using Event=std::array<uint32_t,4>;
struct State {uint32_t format=0,enabled=0,hdr=0,selected=0;std::vector<Event>events;};
#ifndef _WIN64
static State*current=nullptr;static void*apiPointer=nullptr;static void*debugPointer=nullptr;static void*hardwarePointer=nullptr;
static uint32_t __fastcall enabled(void*p,void*){need(p==debugPointer,"debug receiver");current->events.push_back({0,current->enabled,0,0});return current->enabled;}
static uint32_t __fastcall nearest(void*p,void*,uint32_t request,uint32_t flag){need(p==apiPointer&&flag==1,"API receiver/constant");current->events.push_back({1,request,flag,current->selected});return current->selected;}
static uint32_t __fastcall hdr(void*p,void*){need(p==hardwarePointer,"hardware receiver");current->events.push_back({2,current->hdr,0,0});return current->hdr;}
static uint32_t __fastcall getFormat(void*p,void*){need(p==current,"VTF receiver");current->events.push_back({3,current->format,0,0});return current->format;}
static void __fastcall convert(void*p,void*,uint32_t format,uint32_t normalize,uint32_t normalToDuDv){need(p==current&&normalize==0&&normalToDuDv==0,"conversion receiver/arguments");current->events.push_back({4,format,normalize,normalToDuDv});current->format=format;}
struct Vtf{uintptr_t table;};static Vtf*vtf=nullptr;
static uint32_t __fastcall nativeFormat(void*p,void*){need(p==vtf,"VTF object");return getFormat(current,nullptr);}
static void __fastcall nativeConvert(void*p,void*,uint32_t f,uint32_t a,uint32_t b){need(p==vtf,"VTF object");convert(current,nullptr,f,a,b);}
#endif
static int safety(){
    TextureFormatPolicyInputCandidate input;input.vtfFormat=13;input.textureFormat=0;
    TextureFormatPolicyServicesCandidate calls;calls.enabled=[]()->std::optional<uint32_t>{return 1;};calls.nearestFormat=[](uint32_t,uint32_t)->std::optional<uint32_t>{return 12;};calls.hdrType=[]()->std::optional<uint32_t>{return 0;};
    TextureFormatPolicyResultCandidate output;output.textureFormat=0x76543210;unsigned checks=0;
    for(unsigned mode=0;mode<10;++mode){auto broken=calls;auto v=input;
        if(mode==0)broken.enabled={};if(mode==1)broken.enabled=[]()->std::optional<uint32_t>{return {};};
        if(mode==2)broken.nearestFormat={};if(mode==3)broken.nearestFormat=[](uint32_t,uint32_t)->std::optional<uint32_t>{return {};};
        if(mode==4)broken.nearestFormat=[](uint32_t,uint32_t)->std::optional<uint32_t>{return 64;};
        if(mode==5){v.textureFormat=12;broken.hdrType={};}if(mode==6){v.textureFormat=12;broken.hdrType=[]()->std::optional<uint32_t>{return {};};}
        if(mode==7)broken.enabled=[]()->std::optional<uint32_t>{throw std::runtime_error("service");};
        if(mode==8)broken.nearestFormat=[](uint32_t,uint32_t)->std::optional<uint32_t>{throw std::bad_alloc();};if(mode==9)v.vtfFormat=64;
        need(!textureFormatPolicyCandidate(v,broken,output)&&output.textureFormat==0x76543210,"transaction preservation");++checks;
    }
    auto frozen=calls;frozen.enabled=[&]()->std::optional<uint32_t>{frozen.nearestFormat={};return 1;};need(textureFormatPolicyCandidate(input,frozen,output)&&output.textureFormat==12,"service snapshot");++checks;
    TextureFormatPolicyServicesCandidate disabled;disabled.enabled=[]()->std::optional<uint32_t>{return 0x100;};need(textureFormatPolicyCandidate(input,disabled,output)&&!output.changed&&output.textureFormat==input.textureFormat,"disabled skips unavailable services");++checks;
    std::cout<<"PASS owned_format_policy_safety_checks="<<checks<<" pixel_conversion=NOT_PERFORMED\n";return 0;
}
int main(int argc,char**argv){try{
    if(argc==2&&std::string(argv[1])=="--test")return safety();need(argc==4&&!std::ifstream(argv[3]).good(),"image queries fresh-output");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"probe IO");
#ifndef _WIN64
    static_assert(sizeof(void*)==4,"native private x86");MaterialConstructionNativeFixture image(argv[1]);
    std::array<uintptr_t,110>api{};std::array<uintptr_t,6>debug{};std::array<uintptr_t,35>hardware{};std::array<uintptr_t,41>vt{};
    for(auto*p:{&api[0],&debug[0],&hardware[0],&vt[0]})(void)p;
    api.fill(reinterpret_cast<uintptr_t>(&MaterialConstructionNativeFixture::unexpected));debug.fill(api[0]);hardware.fill(api[0]);vt.fill(api[0]);
    api[109]=reinterpret_cast<uintptr_t>(&nearest);debug[5]=reinterpret_cast<uintptr_t>(&enabled);hardware[34]=reinterpret_cast<uintptr_t>(&hdr);vt[19]=reinterpret_cast<uintptr_t>(&nativeFormat);vt[40]=reinterpret_cast<uintptr_t>(&nativeConvert);
    uintptr_t apiObject=reinterpret_cast<uintptr_t>(api.data()),debugObject=reinterpret_cast<uintptr_t>(debug.data()),hardwareObject=reinterpret_cast<uintptr_t>(hardware.data());
    apiPointer=&apiObject;debugPointer=&debugObject;hardwarePointer=&hardwareObject;image.word(0x11b098,reinterpret_cast<uintptr_t>(apiPointer));image.word(0x11b0a0,reinterpret_cast<uintptr_t>(debugPointer));image.word(0x1059f4,reinterpret_cast<uintptr_t>(hardwarePointer));
    Vtf object{reinterpret_cast<uintptr_t>(vt.data())};vtf=&object;
#else
    static_assert(sizeof(void*)==8,"owned x64");(void)argv[1];
#endif
    std::array<uint32_t,7>q{};uint32_t count=0,totalEvents=0;
    while(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))){const auto before=q;State s;s.format=q[0];s.enabled=q[4];s.hdr=q[5];s.selected=q[6];need(q[0]<64&&q[6]<64,"format domain");uint32_t changed=0,textureFormat=q[2];
#ifndef _WIN64
        std::array<uint8_t,0x68>texture{};std::memcpy(texture.data()+0x14,&q[1],4);std::memcpy(texture.data()+0x20,&q[2],4);const auto original=texture;image.image.base[0xfc7fa]=uint8_t(q[3]);current=&s;using Fn=uint32_t(__thiscall*)(void*,void*);changed=reinterpret_cast<Fn>(0x1002d070)(texture.data(),&object)&255u;std::memcpy(&textureFormat,texture.data()+0x20,4);auto expected=original;std::memcpy(expected.data()+0x20,&textureFormat,4);need(texture==expected,"native texture guard");current=nullptr;image.image.verify();
#else
        TextureFormatPolicyInputCandidate input{q[0],q[2],q[1],q[3]};TextureFormatPolicyServicesCandidate calls;
        calls.enabled=[&]()->std::optional<uint32_t>{s.events.push_back({0,s.enabled,0,0});return s.enabled;};
        calls.nearestFormat=[&](uint32_t request,uint32_t flag)->std::optional<uint32_t>{s.events.push_back({3,s.format,0,0});s.events.push_back({1,request,flag,s.selected});return s.selected;};
        calls.hdrType=[&]()->std::optional<uint32_t>{s.events.push_back({2,s.hdr,0,0});return s.hdr;};
        TextureFormatPolicyResultCandidate plan;need(textureFormatPolicyCandidate(input,calls,plan),"owned format policy");for(uint32_t i=0;i<plan.conversionCount;++i){s.events.push_back({4,plan.conversions[i],0,0});s.format=plan.conversions[i];}changed=plan.changed;textureFormat=plan.textureFormat;
#endif
        need(q==before,"query preserved");const std::array<uint32_t,5>record{count,changed,textureFormat,s.format,uint32_t(s.events.size())};out.write(reinterpret_cast<const char*>(record.data()),sizeof(record));out.write(reinterpret_cast<const char*>(s.events.data()),std::streamsize(s.events.size()*sizeof(Event)));totalEvents+=uint32_t(s.events.size());++count;
    }
    need(in.eof()&&!in.gcount()&&count,"query framing");out.flush();need(bool(out),"trace write");std::cout<<"PASS format_policy_cases="<<count<<" ordered_events="<<totalEvents<<" conversion_service=CONTROLLED gpu_calls=0\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

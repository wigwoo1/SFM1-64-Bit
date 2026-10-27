// Actual private original dispatch methods with explicit boundary observers.
// No original instruction patches, DLL entry points, live SFM or GPU calls.
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
#include "native_private_world_image_fixture.h"
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"Route child fault eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);}
template<size_t N>static void field(std::array<uint8_t,N>&a,size_t offset,uint32_t v){need(offset+4<=N,"Fixture field");std::memcpy(a.data()+offset,&v,4);}
struct Query{uint32_t route,flags,count,mask,token,skin,lod,body,extra;};
static_assert(sizeof(Query)==36,"Explicit test wire");
struct Event{uint32_t kind,id,data;};
static std::vector<Event>events;
static Query query{};
#ifdef SFM64_MATERIAL_ROUTE
static std::array<uint8_t,0x90>material{};
struct Proxy{uintptr_t table;uint32_t id;};
static bool __fastcall guarded(void*,void*){events.push_back({0,0,0});return false;}
static void __fastcall bindProxy(void*p,void*,void*data){
    auto id=static_cast<Proxy*>(p)->id;events.push_back({1,id,uint32_t(reinterpret_cast<uintptr_t>(data))});
    if(id==1&&(query.flags&2u))material[0x23]=0;
    if(id==1&&(query.flags&4u))material[0x23]=3;
}
#else
struct Material{uintptr_t table;uint32_t id,hasProxy;};
static bool __fastcall hasProxy(void*p,void*){auto&m=*static_cast<Material*>(p);events.push_back({0,m.id,m.hasProxy});return m.hasProxy!=0;}
static void __fastcall callProxy(void*p,void*,void*data){auto&m=*static_cast<Material*>(p);events.push_back({1,m.id,uint32_t(reinterpret_cast<uintptr_t>(data))});}
#endif
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==4,"Separate original x86 reference utility");
    need(argc==4&&!std::ifstream(argv[3]).good(),"read-only library query fresh-output");SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);auto handler=AddVectoredExceptionHandler(1,&fault);
    NativePrivateWorldImageFixture image(argv[1],0x100000);
#ifdef SFM64_MATERIAL_ROUTE
    std::array<uintptr_t,145>systemMethods{};systemMethods[144]=reinterpret_cast<uintptr_t>(&guarded);uintptr_t system=reinterpret_cast<uintptr_t>(systemMethods.data());auto sp=reinterpret_cast<uintptr_t>(&system);std::memcpy(image.base+0xfc7b0,&sp,4);image.base[0xfc7f9]=0;
#endif
    image.seal();std::ifstream input(argv[2],std::ios::binary);std::ofstream output(argv[3],std::ios::binary);need(bool(input)&&bool(output),"Route IO");uint32_t cases=0,total=0;
    while(input.read(reinterpret_cast<char*>(&query),sizeof(query))){events.clear();uint32_t finalCount=query.count;
#ifdef SFM64_MATERIAL_ROUTE
        need(query.count<=255&&(!(query.flags&4u)||query.count==1),"Checked proxy capacity");
        material.fill(0);field(material,0,0x10095d74);material[0x23]=uint8_t(query.count);
        std::array<uintptr_t,2>methods{0,reinterpret_cast<uintptr_t>(&bindProxy)};std::array<Proxy,255>proxies{};std::array<Proxy*,255>list{};
        for(uint32_t i=0;i<255;++i){proxies[i]={reinterpret_cast<uintptr_t>(methods.data()),i+1};list[i]=&proxies[i];}
        field(material,0x28,uint32_t(reinterpret_cast<uintptr_t>(list.data())));
        std::array<uint32_t,2>forward{0x10095a9c,uint32_t(reinterpret_cast<uintptr_t>(material.data()))};
        auto*object=query.flags&1u?static_cast<void*>(forward.data()):static_cast<void*>(material.data());auto*table=*static_cast<uintptr_t**>(object);
        need(table[53]==(query.flags&1u?0x10007700u:0x10004980u),"Actual material/forward slot53");
        using Call=void(__thiscall*)(void*,void*);reinterpret_cast<Call>(table[53])(object,reinterpret_cast<void*>(uintptr_t(query.token)));finalCount=material[0x23];
#else
        need(query.count<=2&&query.route<=2&&query.lod<=1,"Checked studio metadata");
        std::array<uint8_t,0x258>context{};std::array<uint8_t,0x2000>header{};std::array<uint8_t,16>hardware{};std::array<uint8_t,56>lods{};std::array<uint8_t,32>info{};
        std::array<uintptr_t,54>methods{};methods[21]=reinterpret_cast<uintptr_t>(&hasProxy);methods[53]=reinterpret_cast<uintptr_t>(&callProxy);
        std::array<Material,17>materials{};std::array<std::array<Material*,8>,2>lists{};
        for(uint32_t i=0;i<17;++i)materials[i]={reinterpret_cast<uintptr_t>(methods.data()),i,(query.mask>>(i%8))&1u};
        for(uint32_t lod=0;lod<2;++lod){for(uint32_t i=0;i<8;++i)lists[lod][i]=&materials[8*lod+((query.flags&2u)&&i==2?0:i)];field(lods,28*lod+12,uint32_t(reinterpret_cast<uintptr_t>(lists[lod].data())));}
        field(hardware,8,uint32_t(reinterpret_cast<uintptr_t>(lods.data())));
        field(header,0xcc,8);field(header,0xdc,8);field(header,0xe0,2);field(header,0xe4,0x200);field(header,0xe8,query.count);field(header,0xec,0x300);
        for(uint16_t skin=0;skin<2;++skin)for(uint16_t i=0;i<8;++i){uint16_t v=skin?uint16_t(7-i):i;std::memcpy(header.data()+0x200+skin*16+i*2,&v,2);}
        const uint32_t pattern[2][4]={{0,1,0,2},{2,3,7,3}};
        for(uint32_t part=0;part<2;++part){auto bp=0x300+part*16,model=part?0x1000u:0x400u;field(header,bp+4,1);field(header,bp+8,1);field(header,bp+12,model-bp);field(header,model+0x48,4);field(header,model+0x4c,0x94);for(uint32_t i=0;i<4;++i)field(header,model+0x94+i*0x74,pattern[part][i]);}
        field(context,0x20,query.flags&1u?1u:0u);if(query.route){field(context,0x250,uint32_t(reinterpret_cast<uintptr_t>(&materials[16])));field(context,0x254,query.route-1);}
        field(info,0,uint32_t(reinterpret_cast<uintptr_t>(header.data())));field(info,4,uint32_t(reinterpret_cast<uintptr_t>(hardware.data())));field(info,0xc,query.skin);field(info,0x10,query.body);field(info,0x18,query.token);field(info,0x1c,query.lod);
        const auto beforeContext=context;const auto beforeHeader=header;const auto beforeInfo=info;using Call=void(__thiscall*)(void*,void*);reinterpret_cast<Call>(0x10053cd0)(context.data(),info.data());need(context==beforeContext&&header==beforeHeader&&info==beforeInfo,"Read-only routing fixture");
#endif
        output.write(reinterpret_cast<const char*>(&cases),4);output.write(reinterpret_cast<const char*>(&finalCount),4);auto count=uint32_t(events.size());output.write(reinterpret_cast<const char*>(&count),4);for(const auto&e:events)output.write(reinterpret_cast<const char*>(&e),sizeof(e));total+=count;++cases;image.verify();
    }
    need(input.eof()&&input.gcount()==0&&cases,"Complete query framing");output.flush();need(bool(output),"Complete route output");image.verify();need(VirtualFree(image.base,0,MEM_RELEASE)!=0,"Private image release");RemoveVectoredExceptionHandler(handler);std::cout<<"PASS routing_cases="<<cases<<" events="<<total<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

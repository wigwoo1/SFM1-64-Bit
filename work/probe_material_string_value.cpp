#include "material_string_value_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
#ifdef SFM64_NATIVE_MATERIAL_STRING
#define NOMINMAX
#include <windows.h>
#include <float.h>
#include <xmmintrin.h>
#include "native_private_world_image_fixture.h"
static std::array<uint8_t,0x214>threadData{};
static uint32_t queueCalls=0;
struct Block{void*pointer;uint32_t id,size;bool live;};
struct Event{uint32_t kind,id,size;};
static std::vector<Block>blocks;
static std::vector<Event>events;
static uint32_t nextId=1;
static void __fastcall unexpectedService(void*,void*){ExitProcess(92);}
static void* __fastcall noQueue(void*,void*){++queueCalls;return nullptr;}
static void* __fastcall allocateString(void*,void*,uint32_t size){
    if(size>8192)ExitProcess(93);void*p=std::malloc(size);if(!p)ExitProcess(94);
    blocks.push_back({p,nextId++,size,true});events.push_back({1,blocks.back().id,size});return p;
}
static void __fastcall freeString(void*,void*,void*p){
    for(auto i=blocks.rbegin();i!=blocks.rend();++i)if(i->pointer==p&&i->live){
        i->live=false;events.push_back({2,i->id,i->size});std::free(p);return;
    }
    ExitProcess(95);
}
static void* __stdcall fixtureThread(DWORD){return threadData.data();}
static void* __stdcall fixtureTlsGet(DWORD index){
    if(index!=77)ExitProcess(96);return reinterpret_cast<void*>(&fixtureThread);
}
static DWORD __stdcall fixtureGetError(){return 0;}
static void __stdcall fixtureSetError(DWORD){}
#else
struct Event{uint32_t kind,id,size;};
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static void analytical(){
    unsigned checks=0,refusals=0;auto check=[&](bool b){need(b,"String analytical failure");++checks;};
    MaterialStringValueCandidate v;
    check(materialStringSetCandidate(v,7,{},0,"12.5suffix")&&v.numeric.integer==12&&v.numeric.vector[0]==0x41480000&&v.text=="12.5suffix");
    check(materialStringSetCandidate(v,7,{},0,"-2147483649")&&v.numeric.integer==0x80000000u);
    check(materialStringSetCandidate(v,7,{},0,"2147483648")&&v.numeric.integer==0x7fffffffu);
    check(materialStringSetCandidate(v,7,{},0,"textures/pyro")&&v.numeric.integer==0&&v.numeric.vector[0]==0);
    check(materialStringSetCandidate(v,7,{},0,std::string_view("3\0ignored",9))&&v.text=="3");
    check(materialStringSetCandidate(v,0,{7,0,0,0})&&v.text.empty()&&(v.numeric.typeAndMetadata&15)==4&&v.numeric.vector[3]==0x40e00000);
    check(materialStringSetCandidate(v,7,{},0,std::string(128,'a'))&&materialStringSetCandidate(v,7,{},0,v.text)&&v.text==std::string(128,'a'));
    check(materialStringSetCandidate(v,7,{},0,std::string_view(v.text).substr(1))&&v.text==std::string(127,'a'));
    const auto cached=v.numeric;check(materialStringSetCandidate(v,6)&&v.text.empty()&&v.numeric.integer==cached.integer&&v.numeric.vector==cached.vector);
    for(uint32_t type:{3u,5u,7u,8u,9u,10u,11u,12u,13u,14u,15u}){
        v.numeric.typeAndMetadata=type;const auto old=v;
        check(!materialStringSetCandidate(v,7,{},0,"bad")&&v.text==old.text&&!std::memcmp(&v.numeric,&old.numeric,24));++refusals;
    }
    v.numeric.typeAndMetadata=6;
    for(auto bad:std::array<std::array<uint32_t,2>,4>{{{8,0},{7,1},{5,0},{0,1}}}){
        const auto old=v;check(!materialStringSetCandidate(v,bad[0],{},bad[1])&&v.text==old.text&&!std::memcmp(&v.numeric,&old.numeric,24));++refusals;
    }
    const auto old=v;check(!materialStringSetCandidate(v,7,{},0,std::string(4096,'a'))&&v.text==old.text&&!std::memcmp(&v.numeric,&old.numeric,24));++refusals;
    std::cout<<"PASS string_analytical_checks="<<checks<<" refusal_cases="<<refusals<<'\n';
}
int main(int argc,char**argv){try{
    if(argc==2&&std::string(argv[1])=="--test"){analytical();return 0;}
    need(argc==4&&!std::ifstream(argv[3],std::ios::binary).good(),"library queries fresh-output required");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"string fixture IO");
    auto read=[&](void*p,size_t size){need(bool(in.read(static_cast<char*>(p),std::streamsize(size))),"string input framing");};
    auto text=[&](uint32_t size){need(size<=4095,"string length");std::string s(size,'\0');if(size)read(&s[0],size);return s;};
#ifdef SFM64_NATIVE_MATERIAL_STRING
    static_assert(sizeof(void*)==4,"private oracle only");NativePrivateWorldImageFixture image(argv[1],0x100000);
    auto word=[&](size_t offset,uintptr_t v){std::memcpy(image.base+offset,&v,4);};
    std::array<uintptr_t,145>serviceTable{};serviceTable.fill(reinterpret_cast<uintptr_t>(&unexpectedService));serviceTable[144]=reinterpret_cast<uintptr_t>(&noQueue);
    uintptr_t service=reinterpret_cast<uintptr_t>(serviceTable.data());word(0xfc7b0,reinterpret_cast<uintptr_t>(&service));
    std::array<uintptr_t,6>allocatorTable{};allocatorTable.fill(reinterpret_cast<uintptr_t>(&unexpectedService));allocatorTable[1]=reinterpret_cast<uintptr_t>(&allocateString);allocatorTable[5]=reinterpret_cast<uintptr_t>(&freeString);
    uintptr_t allocator=reinterpret_cast<uintptr_t>(allocatorTable.data()),allocatorPointer=reinterpret_cast<uintptr_t>(&allocator);word(0x951ec,reinterpret_cast<uintptr_t>(&allocatorPointer));
    word(0x95078,reinterpret_cast<uintptr_t>(&fixtureTlsGet));word(0x9511c,reinterpret_cast<uintptr_t>(&fixtureGetError));word(0x9506c,reinterpret_cast<uintptr_t>(&fixtureSetError));
    word(0x104be8,77);word(0x104be4,78);
    const uint32_t locale=0x10104b00,multibyte=0x10104470,threadLocaleFlags=2;
    std::memcpy(threadData.data()+0x6c,&locale,4);std::memcpy(threadData.data()+0x68,&multibyte,4);std::memcpy(threadData.data()+0x70,&threadLocaleFlags,4);
    image.seal();const unsigned oldMxcsr=_mm_getcsr();unsigned oldControl=0;_controlfp_s(&oldControl,0,0);
    unsigned control=0;_controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);
#else
    static_assert(sizeof(void*)==8,"owned native x64");
#endif
    size_t cases=0,steps=0;std::array<uint32_t,8>h{};
    while(in.read(reinterpret_cast<char*>(h.data()),sizeof(h))){
        MaterialStringValueCandidate value;std::memcpy(&value.numeric,h.data(),24);value.text=text(h[6]);
        need(materialStringDomainCandidate(h[0])&&h[7]>0&&h[7]<=64,"string domain/steps");
        need((h[0]&15)==1||value.text.empty(),"nonstring initial text");
#ifdef SFM64_NATIVE_MATERIAL_STRING
        blocks.clear();events.clear();nextId=1;std::array<uint8_t,40>object{};const uintptr_t table=reinterpret_cast<uintptr_t>(image.base+0x98f34);
        std::memcpy(object.data(),&table,4);std::memcpy(object.data()+8,&value.numeric.integer,4);std::memcpy(object.data()+12,value.numeric.vector.data(),16);std::memcpy(object.data()+28,&value.numeric.typeAndMetadata,4);
        if((h[0]&15)==1){void*p=allocateString(nullptr,nullptr,uint32_t(value.text.size()+1));std::memcpy(p,value.text.c_str(),value.text.size()+1);std::memcpy(object.data()+4,&p,4);}
#else
        uint32_t activeId=0,activeSize=0,nextOwnedId=1;
        if((h[0]&15)==1){activeId=nextOwnedId++;activeSize=uint32_t(value.text.size()+1);}
#endif
        for(uint32_t step=0;step<h[7];++step){
            std::array<uint32_t,7>q{};read(q.data(),sizeof(q));const auto s=text(q[6]);const std::array<uint32_t,4>a{q[2],q[3],q[4],q[5]};
            need(q[0]<=7&&((q[0]==5&&q[1]>=1&&q[1]<=4)||(q[0]!=5&&!q[1]))&&(q[0]==7||s.empty()),"operation domain");
            std::vector<Event>trace;uint32_t queues=1;
#ifdef SFM64_NATIVE_MATERIAL_STRING
            events.clear();queueCalls=0;_mm_setcsr(0x1f80);
            constexpr unsigned slots[]{4,3,11,10,9,12,18,5};uintptr_t method=0;std::memcpy(&method,image.base+0x98f34+4*slots[q[0]],4);
            if(q[0]<2){using Fn=void(__thiscall*)(void*,uint32_t);reinterpret_cast<Fn>(method)(object.data(),a[0]);}
            else if(q[0]==2){using Fn=void(__thiscall*)(void*,uint32_t,uint32_t);reinterpret_cast<Fn>(method)(object.data(),a[0],a[1]);}
            else if(q[0]==3){using Fn=void(__thiscall*)(void*,uint32_t,uint32_t,uint32_t);reinterpret_cast<Fn>(method)(object.data(),a[0],a[1],a[2]);}
            else if(q[0]==4){using Fn=void(__thiscall*)(void*,uint32_t,uint32_t,uint32_t,uint32_t);reinterpret_cast<Fn>(method)(object.data(),a[0],a[1],a[2],a[3]);}
            else if(q[0]==5){using Fn=void(__thiscall*)(void*,const void*,int);reinterpret_cast<Fn>(method)(object.data(),a.data(),int(q[1]));}
            else if(q[0]==6){using Fn=void(__thiscall*)(void*);reinterpret_cast<Fn>(method)(object.data());}
            else{using Fn=void(__thiscall*)(void*,const char*);reinterpret_cast<Fn>(method)(object.data(),s.c_str());}
            need(queueCalls==1,"queue query count");std::memcpy(&value.numeric.integer,object.data()+8,4);std::memcpy(value.numeric.vector.data(),object.data()+12,16);std::memcpy(&value.numeric.typeAndMetadata,object.data()+28,4);
            value.text.clear();if((value.numeric.typeAndMetadata&15)==1){void*p=nullptr;std::memcpy(&p,object.data()+4,4);bool live=false;for(const auto&b:blocks)if(b.pointer==p&&b.live)live=true;need(live,"string pointer not owned");value.text=static_cast<const char*>(p);}
            trace=events;
            for(size_t i=0;i<4;++i)need(object[i]==reinterpret_cast<const uint8_t*>(&table)[i],"vtable mutation");
            for(size_t i=32;i<40;++i)need(object[i]==0,"owner/resource mutation");
#else
            // Logical storage-lifetime events for comparison, not hooks into the
            // std::string allocator. Native events are actual captured callbacks.
            const bool wasString=(value.numeric.typeAndMetadata&15)==1;
            need(materialStringSetCandidate(value,q[0],a,q[1],s),"owned operation refusal");
            if(wasString){trace.push_back({2,activeId,activeSize});activeId=activeSize=0;}
            if(q[0]==7){activeId=nextOwnedId++;activeSize=uint32_t(value.text.size()+1);trace.push_back({1,activeId,activeSize});}
#endif
            const uint32_t length=uint32_t(value.text.size()),count=uint32_t(trace.size());
            out.write(reinterpret_cast<const char*>(&value.numeric),24);out.write(reinterpret_cast<const char*>(&length),4);out.write(reinterpret_cast<const char*>(&count),4);out.write(reinterpret_cast<const char*>(&queues),4);
            out.write(value.text.data(),length);out.write(reinterpret_cast<const char*>(trace.data()),std::streamsize(count*sizeof(Event)));++steps;
        }
#ifdef SFM64_NATIVE_MATERIAL_STRING
        uintptr_t undef=0;std::memcpy(&undef,image.base+0x98f34+18*4,4);using Clear=void(__thiscall*)(void*);reinterpret_cast<Clear>(undef)(object.data());
        for(const auto&b:blocks)need(!b.live,"native string leaked");
#endif
        ++cases;
    }
    need(in.eof()&&in.gcount()==0&&cases,"query framing");out.flush();need(bool(out),"output write");
#ifdef SFM64_NATIVE_MATERIAL_STRING
    _mm_setcsr(oldMxcsr);_controlfp_s(&control,oldControl,_MCW_RC|_MCW_PC);image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS material_string_cases="<<cases<<" steps="<<steps<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

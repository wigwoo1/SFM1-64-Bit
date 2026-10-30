// Private original static/dynamic selector slices and owned x64 equivalents.
// No native SFM process, original code patches, material guessing or GPU calls.
#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "phong_pixel_variant_candidate.h"
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
static uint32_t doneSp=0,doneIp=0,kind=0,argument=0,fogCalls=0,bindCalls=0,program=0;
static int32_t fog=0;static void*apiPointer=nullptr;
static uint32_t rawHardware=0,hardwareCalls=0;
static uint32_t __fastcall hardwareQuery(void*,void*){++hardwareCalls;return rawHardware;}
__declspec(naked) static void __cdecl runSlice(uint32_t,void*,void*){
    __asm {
        push ebp
        push ebx
        push esi
        push edi
        mov doneSp,esp
        mov eax,offset finished
        mov doneIp,eax
        mov eax,[esp+20]
        mov ebp,[esp+24]
        mov edi,[esp+28]
        mov esi,edi
        jmp eax
    finished:
        pop edi
        pop esi
        pop ebx
        pop ebp
        ret
    }
}
static int32_t __fastcall fogQuery(void*o,void*){if(o!=apiPointer)throw std::runtime_error("fog receiver");++fogCalls;return fog;}
static void __fastcall staticBind(void*o,void*,const char*name,uint32_t value){
    if(o!=apiPointer||bindCalls)throw std::runtime_error("static receiver/count");
    if(std::strcmp(name,"phong_ps20b")==0)program=20;else if(std::strcmp(name,"phong_ps30")==0)program=30;else throw std::runtime_error("static shader name");
    argument=value;++bindCalls;
}
static void __fastcall dynamicBind(void*o,void*,uint32_t value){if(o!=apiPointer||bindCalls)throw std::runtime_error("dynamic receiver/count");argument=value;++bindCalls;}
static void boundary(unsigned index,CONTEXT&c){
    if(kind==6){if(index>1||bindCalls||hardwareCalls!=1)throw std::runtime_error("hardware gate exit");program=index?30u:20u;c.Esp=doneSp;c.Eip=doneIp;return;}
    const unsigned expected=kind<2?0:kind==2?1:2;
    if(index!=expected||bindCalls!=1)throw std::runtime_error("selector exit boundary");
    c.Esp=doneSp;c.Eip=doneIp;
}
static void __cdecl unused(){throw std::runtime_error("unused breakpoint");}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xc0000005){std::cerr<<"PRIVATE_FAULT eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(89);}return EXCEPTION_CONTINUE_SEARCH;}
#endif
struct Query {uint32_t serial,kind;std::array<uint32_t,16>x;};
template<class T>static void put(uint8_t*p,int at,T value){std::memcpy(p+at,&value,sizeof(value));}
int main(int argc,char**argv){try{
    if(argc!=4||std::ifstream(argv[3]).good())throw std::runtime_error("usage: stdshader queries fresh-output");
    std::ifstream in(argv[2],std::ios::binary);if(!in)throw std::runtime_error("query input");
    std::ofstream out(argv[3],std::ios::binary);if(!out)throw std::runtime_error("trace output");
#ifndef _WIN64
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);auto handler=AddVectoredExceptionHandler(0,&fault);
    NativePrivateWorldImageFixture image(argv[1],0x1000);image.base[0xdb961]=0;
    std::array<uintptr_t,44>hardwareTable{};hardwareTable[43]=reinterpret_cast<uintptr_t>(&hardwareQuery);
    const uintptr_t hardwareObject=reinterpret_cast<uintptr_t>(hardwareTable.data()),hardwarePointer=reinterpret_cast<uintptr_t>(&hardwareObject);
    put(image.base,0xdb9d8,hardwarePointer);image.seal();
    std::array<uintptr_t,19>vtable{};vtable[2]=reinterpret_cast<uintptr_t>(&fogQuery);vtable[12]=reinterpret_cast<uintptr_t>(&dynamicBind);vtable[18]=reinterpret_cast<uintptr_t>(&staticBind);
    uintptr_t api=reinterpret_cast<uintptr_t>(vtable.data());apiPointer=&api;
    PrivateX86BoundaryObserverFixture observer({0x10038f84,0x1003add8,0x1003ad83,uint32_t(reinterpret_cast<uintptr_t>(&unused))},boundary);observer.configure(true);
    bool routing=false;
#else
    static_assert(sizeof(void*)==8,"owned x64");(void)argv[1];
#endif
    Query q{};uint32_t cases=0;
    while(in.read(reinterpret_cast<char*>(&q),sizeof(q))){
        if(q.serial!=cases||q.kind>6)throw std::runtime_error("query identity");
        const auto saved=q;const unsigned version=q.kind==6?phongPixelProgramVersionCandidate(q.x[0]):q.kind%2?30:20;std::array<uint32_t,16> fields{};
        uint32_t result=0,observedFog=0,observedBinds=0;
        if(q.kind<2){fields=phongPixelStaticFieldsCandidate(version,q.x);result=phongPixelStaticArgumentCandidate(version,fields);observedBinds=1;}
        else if(q.kind<4){fields=phongPixelDynamicFieldsCandidate(version,q.x);result=phongPixelDynamicIndexCandidate(version,q.x);observedFog=observedBinds=1;
            // Native constructors retain these zero fields; inline expressions use registers.
            if(version==20)fields[4]=0;else fields[2]=0;
        }else if(q.kind<6){fields=q.x;result=phongPixelStaticArgumentCandidate(version,fields);}
        else{for(unsigned i=1;i<16;++i)if(q.x[i])throw std::runtime_error("unused routing input");result=version;observedFog=1;}
#ifndef _WIN64
        kind=q.kind;argument=fogCalls=bindCalls=program=hardwareCalls=0;
        if((q.kind==6)!=routing){observer.configure(false);routing=q.kind==6;
            observer.entries=routing?std::array<uint32_t,4>{0x1003a7df,0x1003a9c2,uint32_t(reinterpret_cast<uintptr_t>(&unused)),uint32_t(reinterpret_cast<uintptr_t>(&unused))}:
                std::array<uint32_t,4>{0x10038f84,0x1003add8,0x1003ad83,uint32_t(reinterpret_cast<uintptr_t>(&unused))};observer.configure(true);}
        if(q.kind>=4&&q.kind<6){using Fn=uint32_t(__thiscall*)(void*);result=reinterpret_cast<Fn>(image.base+(version==20?0x376f0:0x377b0))(q.x.data());}
        else if(q.kind==6){std::array<uint8_t,0x700>frame{};frame.fill(0xa5);const auto before=frame;rawHardware=q.x[0];runSlice(0x1003a7c7,frame.data()+0x500,&api);
            if(frame!=before)throw std::runtime_error("routing frame mutation");result=program;observedFog=hardwareCalls;observedBinds=bindCalls;}
        else{
            std::array<uint8_t,0x700>frame{};frame.fill(0xa5);auto*ebp=frame.data()+0x500;
            if(q.kind<2){
                const int offsets[16]={-0x456,-0x485,-0x484,-0x457,-0x464,-0x489,-0x488,-0x487,-0x48b,-0x486,0,-0x482,0,-0x45d,-0x455,-0x465};
                for(unsigned i=0;i<16;++i)if(i!=10&&i!=12)put(ebp,offsets[i],uint8_t(q.x[i]));
                put(ebp,-0x47c,q.x[10]);put(ebp,-0x474,q.x[12]);put(ebp,-0x490,uint32_t(q.x[13]!=0));
            }else{
                std::memcpy(&fog,&q.x[0],4);put(ebp,-0x48c,q.x[1]);put(ebp,-0x456,uint8_t(q.x[2]));put(ebp,-0x455,uint8_t(q.x[3]));
                put(ebp,-0x465,uint8_t(q.x[4]));put(ebp,-0x46e,uint8_t(q.x[5]));put(ebp,-0x498,q.x[6]);
            }
            const auto before=frame;const uint32_t entries[]={0x1003875c,0x10038baf,0x1003a8cb,0x1003ac31};
            runSlice(entries[q.kind],ebp,&api);
            const int fieldStart=q.kind<2?(version==20?-0x4d8:-0x4dc):(version==20?-0x44:-0x48);
            const size_t count=q.kind<2?(version==20?14u:15u):(version==20?5u:6u);
            fields.fill(0);std::memcpy(fields.data(),ebp+fieldStart,count*4);
            const size_t first=size_t(0x500+fieldStart),last=first+count*4;
            for(size_t i=0;i<frame.size();++i)if((i<first||i>=last)&&frame[i]!=before[i])throw std::runtime_error("private frame unexpected mutation");
            result=argument;observedFog=fogCalls;observedBinds=bindCalls;
            if(q.kind<2&&program!=version)throw std::runtime_error("static program identity");
        }
#endif
        if(std::memcmp(&q,&saved,sizeof(q)))throw std::runtime_error("query mutation");
        const std::array<uint32_t,6>header{cases,q.kind,version,result,observedFog,observedBinds};
        out.write(reinterpret_cast<const char*>(header.data()),sizeof(header));out.write(reinterpret_cast<const char*>(fields.data()),sizeof(fields));++cases;
    }
    if(!in.eof()||in.gcount()||!cases)throw std::runtime_error("query framing");out.flush();if(!out)throw std::runtime_error("trace write");
#ifndef _WIN64
    observer.configure(false);image.verify();RemoveVectoredExceptionHandler(handler);VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS cases="<<cases<<" original_draw_not_executed=1 gpu_calls=0\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

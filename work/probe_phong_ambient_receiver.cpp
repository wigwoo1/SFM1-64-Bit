// Offline per-instance opcode7/6 receiver. Upload callees are observers, not GPU calls.
#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "generic_ambient_expansion_candidate.h"
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
struct Query {uint32_t serial,order,present,flags,index,count;std::array<uint32_t,18>rgb;std::array<uint32_t,24>cached;};
struct Upload {uint32_t pixel,index;std::array<uint32_t,24>words;};
static std::vector<Upload>uploads;
static uint32_t expansions=0,flushes=0,queries=0;
static std::array<uint8_t,0x4200>object{};
static std::array<uint8_t,0x4c>instance{};
static std::array<uint8_t,0x224+32>lighting{};
static std::array<uint8_t,0x200>payload{};
static uint8_t flags=0;
template<class T>static void put(uint8_t*p,size_t at,T value){std::memcpy(p+at,&value,sizeof(value));}
#ifndef _WIN64
static uint8_t*api(){return object.data()+0x2d0;}
static uint32_t __fastcall counter(void*,void*){++queries;return 0;}
static void __fastcall flush(void*,void*){++flushes;}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xc0000005){std::cerr<<"REFERENCE_FAULT eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(89);}return EXCEPTION_CONTINUE_SEARCH;}
static void __cdecl unused(){throw std::runtime_error("unexpected unused observer");}
static void boundary(unsigned index,CONTEXT&c){
    if(index==2){++expansions;c.EFlags|=0x10000;return;} // Execute unchanged original expansion.
    need(index<2,"unexpected boundary");auto*s=reinterpret_cast<const uint32_t*>(c.Esp);
    need(c.Ecx==reinterpret_cast<uintptr_t>(object.data()),"upload adjusted receiver");
    need(s[2]==reinterpret_cast<uintptr_t>(lighting.data()),"expanded lighting record identity");
    Upload event{index==0?1u:0u,s[1],{}};std::memcpy(event.words.data(),lighting.data(),96);uploads.push_back(event);
    PrivateX86BoundaryObserverFixture::returnCdecl(c);c.Esp+=8; // Observed upload ABI: ret8.
}
#endif

int main(int argc,char**argv){try{
    need(argc==4&&!std::ifstream(argv[3]).good(),"shaderapi queries fresh-output required");
    std::ifstream input(argv[2],std::ios::binary);std::ofstream output(argv[3],std::ios::binary);need(bool(input)&&bool(output),"probe IO");
#ifndef _WIN64
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);auto handler=AddVectoredExceptionHandler(0,&fault);
    NativePrivateWorldImageFixture image(argv[1],0x1000);
    std::array<uintptr_t,274>apiTable{};apiTable[273]=reinterpret_cast<uintptr_t>(&flush);
    std::array<uintptr_t,9>serviceTable{};serviceTable[8]=reinterpret_cast<uintptr_t>(&counter);
    const uintptr_t servicePointer=reinterpret_cast<uintptr_t>(serviceTable.data());
    put(image.base,0x183b58,reinterpret_cast<uintptr_t>(&servicePointer));
    image.seal();
    PrivateX86BoundaryObserverFixture observer({0x10010e90,0x10010d70,0x10012140,uint32_t(reinterpret_cast<uintptr_t>(&unused))},boundary);observer.configure(true);
#else
    (void)argv[1]; // Owned reference has no original-library dependency.
#endif
    Query q{};uint32_t cases=0,total=0;
    while(input.read(reinterpret_cast<char*>(&q),sizeof(q))){
        need(q.serial==cases&&q.order<6&&q.present<2&&q.flags<256&&q.index<=217,"query domain");
        std::vector<uint32_t>stream;
        switch(q.order){case 0:stream={7,q.index,6,0};break;case 1:stream={6,7,q.index,0};break;
        case 2:stream={7,q.index,7,q.index+1,0};break;case 3:stream={6,0};break;
        case 4:stream={7,q.index,0};break;default:stream={0};}
        const auto saved=q;uploads.clear();expansions=flushes=queries=0;
        object.fill(0);instance.fill(0);lighting.fill(0xa5);payload.fill(0x7c);flags=uint8_t(q.flags);
        std::memcpy(lighting.data(),q.cached.data(),96);std::memcpy(payload.data(),q.rgb.data(),72);put(payload.data(),0x54,q.count);
        const auto beforePayload=payload;
#ifndef _WIN64
        put(api(),0,reinterpret_cast<uintptr_t>(apiTable.data()));
        put(api(),0x2bf0,reinterpret_cast<uintptr_t>(lighting.data()));put(api(),0x2bec,reinterpret_cast<uintptr_t>(instance.data()));put(api(),0x2bf4,reinterpret_cast<uintptr_t>(&flags));
        put(instance.data(),0x18,q.present?reinterpret_cast<uintptr_t>(payload.data()):uintptr_t(0));
        using Fn=void(__thiscall*)(void*,const uint32_t*,uint32_t,uint32_t);
        reinterpret_cast<Fn>(image.base+0x21430)(api(),stream.data(),0,0);
#else
        ++queries;
        size_t at=0;
        while(stream.at(at)){
            const auto op=stream[at++];const auto reg=op==7?stream.at(at++):21;
            if(!q.present)continue;
            if(!(flags&1)){const auto expanded=genericAmbientExpansionCandidate(q.rgb);std::memcpy(lighting.data(),expanded.data(),96);flags|=1;++expansions;}
            Upload event{op==7?1u:0u,reg,{}};std::memcpy(event.words.data(),lighting.data(),96);uploads.push_back(event);
        }
        flushes=uploads.empty()?0u:1u;
#endif
        need(std::memcmp(&q,&saved,sizeof(q))==0,"input mutation");
        for(size_t n=96;n<lighting.size();++n)need(lighting[n]==0xa5,"lighting tail/guard overwritten");
        need(payload==beforePayload,"source lighting payload changed");
        const std::array<uint32_t,6>header{cases,flags,expansions,flushes,queries,uint32_t(uploads.size())};
        output.write(reinterpret_cast<const char*>(header.data()),sizeof(header));
        output.write(reinterpret_cast<const char*>(lighting.data()),96);
        for(const auto&event:uploads)output.write(reinterpret_cast<const char*>(&event),sizeof(event));
        total+=uint32_t(uploads.size());++cases;
    }
    need(input.eof()&&input.gcount()==0&&cases>0,"query framing");output.flush();need(bool(output),"trace output");
#ifndef _WIN64
    observer.configure(false);image.verify();RemoveVectoredExceptionHandler(handler);VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS cases="<<cases<<" uploads_observed="<<total<<" gpu_calls=0 whole_receiver_opcodes=7,6,0 upload_bodies=observers\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

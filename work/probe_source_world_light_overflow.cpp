#include "source_world_light_overflow_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <xmmintrin.h>
struct Fixture {
    int32_t type=1;uint32_t special=0,count=6;
    std::array<float,3>intensity{},normal{};float inner=1,outer=0,exponent=1;
    std::array<float,3>point{},incoming{};float attenuation=1;
    std::array<float,18>initial{};std::array<std::array<float,3>,6>basis{};
};
static_assert(sizeof(Fixture)==220,"test-only wire extent");
struct Result {float angular=0;std::array<float,18>ambient{};std::array<uint32_t,4>events{};};
static_assert(sizeof(Result)==92,"normalized fixture result");
#ifndef NATIVE_SOURCE_WORLD_LIGHT_OVERFLOW
static SourceWorldLightAngularCandidate lightOf(const Fixture&f){return {f.type,f.intensity,f.normal,f.inner,f.outer,f.exponent};}
#endif
static void validate(const Fixture&f){
    if(f.special>1||f.count>6)throw std::runtime_error("overflow fixture range");
    for(const auto*p:{&f.intensity,&f.normal,&f.point,&f.incoming})for(float x:*p)if(!std::isfinite(x))throw std::runtime_error("nonfinite overflow fixture vector");
    for(float x:{f.inner,f.outer,f.exponent,f.attenuation})if(!std::isfinite(x))throw std::runtime_error("nonfinite overflow fixture scalar");
    for(float x:f.initial)if(!std::isfinite(x))throw std::runtime_error("nonfinite fixture ambient");for(const auto&v:f.basis)for(float x:v)if(!std::isfinite(x))throw std::runtime_error("nonfinite fixture basis");
    if(f.type==2&&(f.outer>f.inner||f.outer< -1||f.inner>1||f.exponent<0))throw std::runtime_error("invalid fixture cone");
}
#ifdef NATIVE_SOURCE_WORLD_LIGHT_OVERFLOW
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
static const Fixture*fixture=nullptr;static Result events{};static const void*activeLight=nullptr;static size_t activeCase=0;
static float __cdecl visibility(void*light,const float*point,uint32_t flags,float*incoming,uint32_t ecx){
    if(light!=activeLight||ecx||std::memcmp(point,fixture->point.data(),12)||flags!=(fixture->special?3u:0u))throw std::runtime_error("visibility ABI arguments");++events.events[0];events.events[3]=flags;std::memcpy(incoming,fixture->incoming.data(),12);return fixture->attenuation;
}
// Original10194420 custom ABI: ECX=0, EDX=light, EDI=direction output,
// caller-cleaned stack point/flags; scalar result in XMM0, not x87 ST0.
__declspec(naked)static void visibilityBridge(){__asm{
 push ebp
 mov ebp,esp
 push ecx
 push edi
 push dword ptr[ebp+0ch]
 push dword ptr[ebp+8]
 push edx
 call visibility
 fstp dword ptr[esp]
 movss xmm0,dword ptr[esp]
 add esp,14h
 pop ebp
 ret
}}
static const void* __fastcall directions(void*,void*){if(events.events[2])throw std::runtime_error("basis query order");++events.events[1];return fixture->basis.data();}
static int32_t __fastcall directionCount(void*,void*){if(events.events[1]!=1)throw std::runtime_error("basis count order");++events.events[2];return int32_t(fixture->count);}
static void redirect(uint8_t*base,size_t offset,void*target){auto*p=base+offset;p[0]=0xe9;const int32_t d=int32_t(reinterpret_cast<intptr_t>(target)-reinterpret_cast<intptr_t>(p)-5);std::memcpy(p+1,&d,4);}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"overflow fault case="<<activeCase<<" eip="<<std::hex<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
#else
struct Services {
    const Fixture&f;Result&r;
    SourceWorldLightVisibilityCandidate visibility(const SourceWorldLightAngularCandidate&,const std::array<float,3>&p,uint32_t flags){if(p!=f.point||flags!=(f.special?3u:0u))throw std::runtime_error("owned visibility arguments");++r.events[0];r.events[3]=flags;return {f.incoming,f.attenuation};}
    const auto&directions(){if(r.events[2])throw std::runtime_error("owned basis order");++r.events[1];return f.basis;}
    int32_t directionCount(){if(r.events[1]!=1)throw std::runtime_error("owned count order");++r.events[2];return int32_t(f.count);}
};
#endif
int main(int argc,char**argv){try{
    if(argc!=4)throw std::runtime_error("engine input output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("fixture IO");
#ifdef NATIVE_SOURCE_WORLD_LIGHT_OVERFLOW
    static_assert(sizeof(void*)==4,"native x86");AddVectoredExceptionHandler(1,fault);NativePrivateWorldImageFixture image(argv[1]);redirect(image.base,0x194420,reinterpret_cast<void*>(&visibilityBridge));
    std::array<uint32_t,19>vtable{};vtable[18]=reinterpret_cast<uint32_t>(&directions);vtable[17]=reinterpret_cast<uint32_t>(&directionCount);uint32_t object=reinterpret_cast<uint32_t>(vtable.data());const uint32_t address=reinterpret_cast<uint32_t>(&object);std::memcpy(image.base+0x6941d4,&address,4);image.seal();
    using Angular=float(__cdecl*)(const void*,const float*,const float*,const float*);using Overflow=void(__cdecl*)(const void*,const float*,void*,bool);
    const auto angular=reinterpret_cast<Angular>(image.base+0xd1420);const auto overflow=reinterpret_cast<Overflow>(image.base+0x195510);
#else
    static_assert(sizeof(void*)==8,"owned x64");
#endif
    size_t count=0;Fixture f;
    while(in.read(reinterpret_cast<char*>(&f),sizeof(f))){validate(f);Result result{};const auto before=f;const auto mxcsr=_mm_getcsr();
#ifdef NATIVE_SOURCE_WORLD_LIGHT_OVERFLOW
        activeCase=count;fixture=&f;events={};std::array<uint8_t,0x58>light{};std::memcpy(light.data()+0xc,f.intensity.data(),12);std::memcpy(light.data()+0x18,f.normal.data(),12);std::memcpy(light.data()+0x34,&f.type,4);std::memcpy(light.data()+0x3c,&f.inner,4);std::memcpy(light.data()+0x40,&f.outer,4);std::memcpy(light.data()+0x44,&f.exponent,4);const auto beforeLight=light;activeLight=light.data();
        std::array<uint32_t,27>guarded;guarded.fill(0xa7b8c9dau);std::memcpy(guarded.data()+2,f.initial.data(),72);const auto beforeGuarded=guarded;
        const float a=angular(light.data(),f.normal.data(),f.incoming.data(),f.incoming.data());overflow(light.data(),f.point.data(),guarded.data()+2,f.special!=0);result=events;result.angular=a;std::memcpy(result.ambient.data(),guarded.data()+2,72);
        if(light!=beforeLight)throw std::runtime_error("light mutation");for(unsigned i=0;i<27;++i)if((i<2||i>=20)&&guarded[i]!=beforeGuarded[i])throw std::runtime_error("ambient output boundary");
#else
        const auto light=lightOf(f);result.angular=sourceWorldLightAngularCandidate(light,light.normal,f.incoming,f.incoming);SourceModelLightingStateCandidate state;state.ambient=f.initial;state.lights={uintptr_t(0x123400000001ull),2};const auto beforeIds=state.lights;Services services{f,result};sourceWorldLightOverflowCandidate(light,f.point,state,f.special!=0,services);if(state.lights!=beforeIds)throw std::runtime_error("light identity mutation");result.ambient=state.ambient;
#endif
        if(std::memcmp(&f,&before,sizeof(f))||(_mm_getcsr()&~0x3fu)!=(mxcsr&~0x3fu))throw std::runtime_error("input/floating environment mutation");if(!std::isfinite(result.angular))throw std::runtime_error("nonfinite angular output");for(float x:result.ambient)if(!std::isfinite(x))throw std::runtime_error("nonfinite ambient output");out.write(reinterpret_cast<const char*>(&result),sizeof(result));++count;
    }
    if(!in.eof()||in.gcount()!=0||!count)throw std::runtime_error("overflow fixture truncation");out.flush();if(!out)throw std::runtime_error("overflow output");
#ifdef NATIVE_SOURCE_WORLD_LIGHT_OVERFLOW
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS world light overflow cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

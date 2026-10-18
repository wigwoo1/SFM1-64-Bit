#include "source_world_light_visibility_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
struct Fixture {
    std::array<uint32_t,7>h{}; // type,flags,style index,style value,debug mode,surface flags,excluded entity
    std::array<float,6>p{}; // radius,constant,linear,quadratic,threshold,trace fraction
    std::array<std::array<float,3>,6>v{}; // light position,intensity,normal,query point,initial incoming,trace end
};
static_assert(sizeof(Fixture)==124,"visibility test wire extent");
struct Result {
    float falloff=0;std::array<float,3>incoming{};float attenuation=0;
    std::array<uint32_t,8>events{}; // trace,debug,filter,excluded,mask,isray,swept,debug arguments
    std::array<float,3>start{},delta{},debugStart{},debugEnd{};
};
static_assert(sizeof(Result)==100,"visibility normalized result");
#ifdef WORLD_LIGHT_QUERY_OVERFLOW_COMPOSITE
static const std::array<std::array<float,3>,6>basis{{{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}}};
struct CompositeResult {Result query;std::array<float,18>ambient{};std::array<uint32_t,10>events{};};
static_assert(sizeof(CompositeResult)==212,"complete overflow normalized result");
#endif
static int32_t signedWord(uint32_t x){int32_t y;std::memcpy(&y,&x,4);return y;}
static SourceWorldLightQueryCandidate lightOf(const Fixture&f){SourceWorldLightQueryCandidate l;l.angular.type=signedWord(f.h[0]);l.style=f.h[2];l.radius=f.p[0];l.constant=f.p[1];l.linear=f.p[2];l.quadratic=f.p[3];l.position=f.v[0];l.angular.intensity=f.v[1];l.angular.normal=f.v[2];return l;}
static void validate(const Fixture&f){if(f.h[1]>15||f.h[2]>255||f.h[5]>65535||f.p[5]<0||f.p[5]>1)throw std::runtime_error("visibility fixture range");for(float x:f.p)if(!std::isfinite(x))throw std::runtime_error("nonfinite visibility scalar");for(const auto&v:f.v)for(float x:v)if(!std::isfinite(x))throw std::runtime_error("nonfinite visibility vector");}
#ifdef NATIVE_SOURCE_WORLD_LIGHT_VISIBILITY
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
static const Fixture*fixture=nullptr;static Result events{};static uint8_t*originalBase=nullptr;static size_t activeCase=0;
#ifdef WORLD_LIGHT_QUERY_OVERFLOW_COMPOSITE
static std::array<uint32_t,2>basisEvents{};
static const void* __fastcall directions(void*,void*){++basisEvents[0];return basis.data();}
static int32_t __fastcall directionCount(void*,void*){++basisEvents[1];return 6;}
#endif
static void __fastcall trace(void*,void*,const uint8_t*ray,uint32_t mask,const uint32_t*filter,uint8_t*out){
    if(++events.events[0]!=1||mask!=0x4481)throw std::runtime_error("native trace count/mask");
    const uint32_t vtable=filter[0];const bool props=(fixture->h[1]&4)!=0;
    if(vtable!=reinterpret_cast<uint32_t>(originalBase+(props?0x35bb48:0x31dee4)))throw std::runtime_error("native trace filter identity");
    using TraceType=int32_t(__thiscall*)(const void*);events.events[2]=reinterpret_cast<TraceType>(*reinterpret_cast<const uint32_t*>(vtable+4))(filter);events.events[3]=props?filter[1]:0;events.events[4]=mask;events.events[5]=ray[0x44];events.events[6]=ray[0x45];
    if(props&&(filter[1]!=fixture->h[6]||std::memcmp(reinterpret_cast<const void*>(filter[2]),fixture->v[3].data(),12)))throw std::runtime_error("native filter context");
    std::memcpy(events.start.data(),ray,12);std::memcpy(events.delta.data(),ray+0x10,12);
    for(unsigned i=0;i<3;++i){float offset,extent;std::memcpy(&offset,ray+0x20+i*4,4);std::memcpy(&extent,ray+0x30+i*4,4);if(offset!=0||extent!=0)throw std::runtime_error("nonpoint visibility ray");}uint32_t transform;std::memcpy(&transform,ray+0x40,4);if(transform)throw std::runtime_error("visibility ray transform");
    std::memset(out,0,0x54);std::memcpy(out+0x2c,&fixture->p[5],4);const uint16_t flags=uint16_t(fixture->h[5]);std::memcpy(out+0x42,&flags,2);std::memcpy(out+0xc,fixture->v[5].data(),12);
}
static void __cdecl debug(const float*start,const float*end,int r,int g,int b,int a,bool test,float duration){
    ++events.events[1];if(events.events[1]!=1||r!=255||g||b||a!=255||!test||duration!=3)throw std::runtime_error("debug line arguments");events.events[7]=1;std::memcpy(events.debugStart.data(),start,12);std::memcpy(events.debugEnd.data(),end,12);
}
static void redirect(uint8_t*base,size_t offset,void*target){auto*p=base+offset;p[0]=0xe9;const int32_t d=int32_t(reinterpret_cast<intptr_t>(target)-reinterpret_cast<intptr_t>(p)-5);std::memcpy(p+1,&d,4);}
// ECX optional entity,EDX light,EDI direction; two caller-cleaned arguments.
__declspec(naked)static void invoke(void*,const void*,const float*,uint32_t,uint32_t,float*,float*){__asm{
 push ebp
 mov ebp,esp
 push ebx
 push esi
 push edi
 mov edx,dword ptr[ebp+0ch]
 mov ecx,dword ptr[ebp+18h]
 mov edi,dword ptr[ebp+1ch]
 push dword ptr[ebp+14h]
 push dword ptr[ebp+10h]
 call dword ptr[ebp+8]
 add esp,8
 mov eax,dword ptr[ebp+20h]
 movss dword ptr[eax],xmm0
 pop edi
 pop esi
 pop ebx
 pop ebp
 ret
}}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"visibility fault case="<<activeCase<<" eip="<<std::hex<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
#else
struct Services {
    const Fixture&f;Result&r;
#ifdef WORLD_LIGHT_QUERY_OVERFLOW_COMPOSITE
    unsigned basisCalls=0,countCalls=0;
    const auto&directions(){++basisCalls;return basis;}
    int32_t directionCount(){++countCalls;return 6;}
#endif
    int32_t style(uint32_t index){if(index!=f.h[2])throw std::runtime_error("owned style index");return signedWord(f.h[3]);}
    float intensityThreshold(){return f.p[4];}
    int32_t debugMode(){return signedWord(f.h[4]);}
    void debugLine(const std::array<float,3>&start,const std::array<float,3>&end){++r.events[1];r.events[7]=1;r.debugStart=start;r.debugEnd=end;}
    SourceWorldLightTraceCandidate trace(const SourceWorldLightRayCandidate&ray,uint32_t mask,const SourceWorldLightFilterCandidate&filter){++r.events[0];r.events[2]=uint32_t(filter.mode);r.events[3]=uint32_t(filter.excludedEntity);r.events[4]=mask;r.events[5]=1;r.events[6]=ray.swept;r.start=ray.start;r.delta=ray.delta;return {f.p[5],uint16_t(f.h[5]),f.v[5]};}
};
#endif
int main(int argc,char**argv){try{
    if(argc!=4)throw std::runtime_error("engine input output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("visibility IO");
#ifdef NATIVE_SOURCE_WORLD_LIGHT_VISIBILITY
    static_assert(sizeof(void*)==4,"native x86");AddVectoredExceptionHandler(1,fault);NativePrivateWorldImageFixture image(argv[1]);originalBase=image.base;redirect(image.base,0xa7d20,reinterpret_cast<void*>(&debug));
    std::array<uint8_t,0x34>threshold{},debugMode{};const uint32_t thresholdPtr=reinterpret_cast<uint32_t>(threshold.data()),debugPtr=reinterpret_cast<uint32_t>(debugMode.data());std::memcpy(image.base+0x64aa14,&thresholdPtr,4);std::memcpy(image.base+0x64aac4,&debugPtr,4);
    std::array<uint32_t,6>vtable{};vtable[5]=reinterpret_cast<uint32_t>(&trace);uint32_t object=reinterpret_cast<uint32_t>(vtable.data());const uint32_t objectPtr=reinterpret_cast<uint32_t>(&object);std::memcpy(image.base+0x407e90,&objectPtr,4);
#ifdef WORLD_LIGHT_QUERY_OVERFLOW_COMPOSITE
    std::array<uint32_t,19>basisTable{};basisTable[18]=reinterpret_cast<uint32_t>(&directions);basisTable[17]=reinterpret_cast<uint32_t>(&directionCount);uint32_t basisObject=reinterpret_cast<uint32_t>(basisTable.data());const uint32_t basisPtr=reinterpret_cast<uint32_t>(&basisObject);std::memcpy(image.base+0x6941d4,&basisPtr,4);
#endif
    image.seal();
    using Distance=float(__cdecl*)(const void*,const float*,bool);const auto distance=reinterpret_cast<Distance>(image.base+0xd1240);
#else
    static_assert(sizeof(void*)==8,"owned x64");
#endif
    size_t cases=0;Fixture f;
    while(in.read(reinterpret_cast<char*>(&f),sizeof(f))){validate(f);Result result{};const auto before=f;const auto light=lightOf(f);std::array<float,3>delta{};for(unsigned i=0;i<3;++i)delta[i]=light.position[i]-f.v[3][i];
#ifdef NATIVE_SOURCE_WORLD_LIGHT_VISIBILITY
        activeCase=cases;fixture=&f;events={};std::array<uint8_t,100>raw{};std::memcpy(raw.data(),light.position.data(),12);std::memcpy(raw.data()+0xc,light.angular.intensity.data(),12);std::memcpy(raw.data()+0x18,light.angular.normal.data(),12);std::memcpy(raw.data()+0x34,&f.h[0],4);std::memcpy(raw.data()+0x38,&f.h[2],4);std::memcpy(raw.data()+0x3c,&light.angular.inner,4);std::memcpy(raw.data()+0x40,&light.angular.outer,4);std::memcpy(raw.data()+0x44,&light.angular.exponent,4);std::memcpy(raw.data()+0x48,&f.p[0],16);const auto beforeRaw=raw;
        std::memcpy(threshold.data()+0x2c,&f.p[4],4);std::memcpy(debugMode.data()+0x30,&f.h[4],4);std::memcpy(image.base+0x5cc4a8+f.h[2]*4,&f.h[3],4);const auto beforeThreshold=threshold;const auto beforeDebug=debugMode;
        std::array<uint32_t,5>incoming{0xa7b8c9da,0,0,0,0xa7b8c9da};std::memcpy(incoming.data()+1,f.v[4].data(),12);float attenuation=0;const float falloff=distance(raw.data(),delta.data(),(f.h[1]&2)!=0);invoke(image.base+0x194420,raw.data(),f.v[3].data(),f.h[1],f.h[6],reinterpret_cast<float*>(incoming.data()+1),&attenuation);
        result=events;result.falloff=falloff;result.attenuation=attenuation;std::memcpy(result.incoming.data(),incoming.data()+1,12);
        if(raw!=beforeRaw||threshold!=beforeThreshold||debugMode!=beforeDebug||incoming.front()!=0xa7b8c9da||incoming.back()!=0xa7b8c9da||std::memcmp(image.base+0x5cc4a8+f.h[2]*4,&f.h[3],4))throw std::runtime_error("native query mutation/canary");
#else
        result.falloff=sourceWorldLightDistanceCandidate(light,delta,(f.h[1]&2)!=0);Services services{f,result};const auto q=sourceWorldLightVisibilityQueryCandidate(light,f.v[3],f.h[1],f.h[6],services,f.v[4]);result.incoming=q.incoming;result.attenuation=q.attenuation;
#endif
#ifdef WORLD_LIGHT_QUERY_OVERFLOW_COMPOSITE
        if(f.h[1]!=0&&f.h[1]!=3)throw std::runtime_error("overflow caller accepts query flags0/3 only");CompositeResult combined;combined.query=result;for(unsigned i=0;i<18;++i)combined.ambient[i]=float(i)*.125f-.5f;
#ifdef NATIVE_SOURCE_WORLD_LIGHT_VISIBILITY
        events={};basisEvents={};std::array<uint32_t,27>state;state.fill(0xa7b8c9dau);std::memcpy(state.data()+2,combined.ambient.data(),72);const auto beforeState=state;using Overflow=void(__cdecl*)(const void*,const float*,void*,bool);reinterpret_cast<Overflow>(image.base+0x195510)(raw.data(),f.v[3].data(),state.data()+2,f.h[1]==3);std::memcpy(combined.ambient.data(),state.data()+2,72);std::copy(events.events.begin(),events.events.end(),combined.events.begin());combined.events[8]=basisEvents[0];combined.events[9]=basisEvents[1];for(unsigned i=0;i<27;++i)if((i<2||i>=20)&&state[i]!=beforeState[i])throw std::runtime_error("complete overflow output boundary");if(raw!=beforeRaw)throw std::runtime_error("complete overflow light mutation");
#else
        Result renderEvents{};Services renderServices{f,renderEvents};SourceModelLightingStateCandidate state;state.ambient=combined.ambient;state.lights={uintptr_t(0x123400001234ull)};sourceWorldLightCompleteOverflowCandidate(light,f.v[3],state,f.h[1]==3,renderServices);if(state.lights!=std::vector<uintptr_t>{uintptr_t(0x123400001234ull)})throw std::runtime_error("complete overflow light-list mutation");combined.ambient=state.ambient;std::copy(renderEvents.events.begin(),renderEvents.events.end(),combined.events.begin());combined.events[8]=renderServices.basisCalls;combined.events[9]=renderServices.countCalls;
#endif
        for(float x:combined.ambient)if(!std::isfinite(x))throw std::runtime_error("nonfinite complete overflow output");out.write(reinterpret_cast<const char*>(&combined),sizeof(combined));
#else
        out.write(reinterpret_cast<const char*>(&result),sizeof(result));
#endif
        if(std::memcmp(&f,&before,sizeof(f)))throw std::runtime_error("fixture mutation");if(!std::isfinite(result.falloff)||!std::isfinite(result.attenuation))throw std::runtime_error("nonfinite visibility output");for(float x:result.incoming)if(!std::isfinite(x))throw std::runtime_error("nonfinite incoming output");++cases;
    }
    if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("visibility fixture truncation");out.flush();if(!out)throw std::runtime_error("visibility output");
#ifdef NATIVE_SOURCE_WORLD_LIGHT_VISIBILITY
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS world light visibility cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

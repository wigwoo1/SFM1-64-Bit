#include "source_model_cached_lighting_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#ifdef WORLD_LIGHT_OVERFLOW_COMPOSITE
#include "world_light_overflow_composite_fixture.h"
#endif
struct Fixture {
    std::array<uint32_t,8>h{}; // index,table extent,gate flags,current count,cached count,config,hardware A/B
    std::array<float,18>current{},cached{},initialDraw{};
    std::array<float,3>point{};
    std::array<uint32_t,4>currentIds{},cachedIds{};
};
static_assert(sizeof(Fixture)==292,"test-only wire input");
struct Result {
    std::array<uint32_t,4>h{}; // selected index/sentinel,active,hardware calls,overflow calls
    std::array<float,18>draw{},scratch{};
    uint32_t count=0;
    std::array<uint32_t,4>ids{},overflow{};
    std::array<uint32_t,8>hardware{};
};
static_assert(sizeof(Result)==228,"test-only normalized output");
static int32_t signedWord(uint32_t x){int32_t r;std::memcpy(&r,&x,4);return r;}
#ifndef WORLD_LIGHT_OVERFLOW_COMPOSITE
static float contribution(uint32_t id,unsigned component,const std::array<float,3>&p){
    const float a=float(id)*0.125f;const float b=p[component%3]*0.0009765625f;return a+b;
}
#endif
#ifdef NATIVE_SOURCE_MODEL_CACHED_LIGHTING
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
static const Fixture*fixture=nullptr;static Result events{};static size_t activeCase=0;
static std::array<std::array<uint32_t,22>,8>lightObjects{};
#ifdef WORLD_LIGHT_OVERFLOW_COMPOSITE
static uint8_t*originalBase=nullptr;
static std::array<std::array<float,3>,6>ambientBasis{{{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}}};
#endif
static uint32_t lightId(uint32_t address){for(unsigned i=0;i<lightObjects.size();++i)if(address==reinterpret_cast<uint32_t>(lightObjects[i].data()))return i+1;throw std::runtime_error("unknown native light identity");}
static int32_t __fastcall hardware(void*,void*){const unsigned n=events.h[2]++;if(n>=8)throw std::runtime_error("too many hardware calls");const uint32_t x=fixture->h[6+(n%2)];events.hardware[n]=x;return signedWord(x);}
static void __cdecl overflow(void*light,const float*point,void*state,bool special){
    if(special||std::memcmp(point,fixture->point.data(),12))throw std::runtime_error("overflow arguments");
    const auto id=lightId(reinterpret_cast<uint32_t>(light));const unsigned n=events.h[3]++;if(n>=4)throw std::runtime_error("overflow count");events.overflow[n]=id;
#ifdef WORLD_LIGHT_OVERFLOW_COMPOSITE
    using Original=void(__cdecl*)(void*,const float*,void*,bool);reinterpret_cast<Original>(originalBase+0x195510)(light,point,state,special);
#else
    float*p=static_cast<float*>(state);for(unsigned i=0;i<18;++i)p[i]=p[i]+contribution(id,i,fixture->point);
#endif
}
#ifdef WORLD_LIGHT_OVERFLOW_COMPOSITE
static float __cdecl visibility(void*light,const float*point,uint32_t flags,float*out,uint32_t ecx){
    if(ecx||flags||std::memcmp(point,fixture->point.data(),12))throw std::runtime_error("native composite visibility arguments");const auto q=overflowFixtureVisibility(lightId(reinterpret_cast<uint32_t>(light)),fixture->point);std::memcpy(out,q.incoming.data(),12);return q.attenuation;
}
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
static const void* __fastcall basis(void*,void*){return ambientBasis.data();}
static int32_t __fastcall basisCount(void*,void*){return 6;}
#endif
static void redirect(uint8_t*base,size_t offset,void*target){auto*p=base+offset;p[0]=0xe9;const int32_t d=int32_t(reinterpret_cast<intptr_t>(target)-reinterpret_cast<intptr_t>(p)-5);std::memcpy(p+1,&d,4);}
struct NativeContext {void*packet;void*renderer;void*output;void*current;void*scratch;const float*point;uint32_t selected;uint32_t active;};
__declspec(naked)static void invoke(void*,NativeContext*){__asm{
 push ebp
 mov ebp,esp
 sub esp,200h
 push ebx
 push esi
 push edi
 mov ebx,dword ptr[ebp+0ch]
 mov eax,dword ptr[ebx+4]
 mov dword ptr[ebp-144h],eax
 mov esi,dword ptr[ebx+0ch]
 lea edi,[ebp-0d8h]
 mov ecx,23
 rep movsd
 mov esi,dword ptr[ebx+10h]
 lea edi,[ebp-134h]
 mov ecx,23
 rep movsd
 mov esi,dword ptr[ebx+14h]
 lea edi,[ebp-150h]
 mov ecx,3
 rep movsd
 mov esi,dword ptr[ebx+8]
 mov eax,dword ptr[ebx]
 call dword ptr[ebp+8]
 mov ebx,dword ptr[ebp+0ch]
 mov eax,dword ptr[ebp-16ch]
 mov dword ptr[ebx+18h],eax
 xor eax,eax
 mov al,byte ptr[ebp-135h]
 mov dword ptr[ebx+1ch],eax
 lea esi,[ebp-134h]
 mov edi,dword ptr[ebx+10h]
 mov ecx,23
 rep movsd
 pop edi
 pop esi
 pop ebx
 mov esp,ebp
 pop ebp
 ret
}}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"cached blend fault case="<<activeCase<<" eip="<<std::hex<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
#else
struct OwnedServices {
    const Fixture&f;Result&r;
    int32_t configuredLimit(){return signedWord(f.h[5]);}
    int32_t hardwareLimit(){const unsigned n=r.h[2]++;if(n>=8)throw std::runtime_error("owned hardware calls");const auto x=f.h[6+n%2];r.hardware[n]=x;return signedWord(x);}
    void overflow(uintptr_t id,const std::array<float,3>&p,SourceModelLightingStateCandidate&s,bool special){if(special)throw std::runtime_error("owned overflow special");r.overflow.at(r.h[3]++)=uint32_t(id);
#ifdef WORLD_LIGHT_OVERFLOW_COMPOSITE
        OverflowFixtureRenderServices render{uint32_t(id),p};sourceWorldLightOverflowCandidate(overflowFixtureLight(uint32_t(id)),p,s,special,render);
#else
        for(unsigned i=0;i<18;++i)s.ambient[i]=s.ambient[i]+contribution(uint32_t(id),i,p);
#endif
    }
};
#endif
int main(int argc,char**argv){try{
    if(argc!=4)throw std::runtime_error("engine input output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("fixture IO");
#ifdef NATIVE_SOURCE_MODEL_CACHED_LIGHTING
    static_assert(sizeof(void*)==4,"native x86");AddVectoredExceptionHandler(1,fault);NativePrivateWorldImageFixture image(argv[1]);
    // Skip previously verified acquisition, then run the actual cached branch,
    // original count/list copy and original identity/capability merge.
    redirect(image.base,0xd5845,image.base+0xd58c0);image.base[0xd5d97]=0xc3;
#ifdef WORLD_LIGHT_OVERFLOW_COMPOSITE
    originalBase=image.base;
    // Count the merge's overflow call, then invoke intact original10195510.
    redirect(image.base,0xd2407,reinterpret_cast<void*>(&overflow));image.base[0xd2407]=0xe8;
    redirect(image.base,0x194420,reinterpret_cast<void*>(&visibilityBridge));
    std::array<uint32_t,19>basisTable{};basisTable[18]=reinterpret_cast<uint32_t>(&basis);basisTable[17]=reinterpret_cast<uint32_t>(&basisCount);uint32_t basisObject=reinterpret_cast<uint32_t>(basisTable.data());const uint32_t basisPtr=reinterpret_cast<uint32_t>(&basisObject);std::memcpy(image.base+0x6941d4,&basisPtr,4);
    for(unsigned i=0;i<8;++i){const auto l=overflowFixtureLight(i+1);auto*p=reinterpret_cast<uint8_t*>(lightObjects[i].data());std::memcpy(p+0xc,l.intensity.data(),12);std::memcpy(p+0x18,l.normal.data(),12);std::memcpy(p+0x34,&l.type,4);std::memcpy(p+0x3c,&l.inner,4);std::memcpy(p+0x40,&l.outer,4);std::memcpy(p+0x44,&l.exponent,4);}
#else
    redirect(image.base,0x195510,reinterpret_cast<void*>(&overflow));
#endif
    std::array<uint8_t,0x34>gate{},limit{};const uint32_t gatePtr=reinterpret_cast<uint32_t>(gate.data()),limitPtr=reinterpret_cast<uint32_t>(limit.data());std::memcpy(image.base+0x5cb1ac,&gatePtr,4);std::memcpy(image.base+0x64ab1c,&limitPtr,4);
    std::array<uint32_t,13>hardwareTable{};hardwareTable[12]=reinterpret_cast<uint32_t>(&hardware);uint32_t hardwareObject=reinterpret_cast<uint32_t>(hardwareTable.data());const uint32_t hardwarePtr=reinterpret_cast<uint32_t>(&hardwareObject);std::memcpy(image.base+0x694194,&hardwarePtr,4);image.seal();
    std::vector<uint8_t>records(size_t(65535)*0x108);
#else
    static_assert(sizeof(void*)==8,"owned x64");std::vector<SourceModelInstanceLightingCandidate>records(65535);
#endif
    size_t count=0;Fixture f;
    while(in.read(reinterpret_cast<char*>(&f),sizeof(f))){
        if(f.h[0]>65535||f.h[1]>65535||f.h[2]>7||f.h[3]>4||f.h[4]>4||(f.h[0]!=65535&&f.h[0]>=f.h[1]))throw std::runtime_error("cached blend fixture range");
        for(auto p:{&f.current,&f.cached,&f.initialDraw})for(float x:*p)if(!std::isfinite(x))throw std::runtime_error("nonfinite fixture cube");for(float x:f.point)if(!std::isfinite(x))throw std::runtime_error("nonfinite fixture point");
        for(unsigned group=0;group<2;++group)for(unsigned i=0;i<f.h[3+group];++i){auto id=(group?f.cachedIds:f.currentIds)[i];if(id<1||id>8)throw std::runtime_error("light identity range");}
        Result result{};result.draw=f.initialDraw;
#ifdef NATIVE_SOURCE_MODEL_CACHED_LIGHTING
        activeCase=count;fixture=&f;events={};const uint32_t gateValue=f.h[2]&1;std::memcpy(gate.data()+0x30,&gateValue,4);std::memcpy(limit.data()+0x30,&f.h[5],4);
        std::array<uint32_t,23>current{},scratch{};std::memcpy(current.data(),f.current.data(),72);current[18]=f.h[3];for(unsigned i=0;i<f.h[3];++i)current[19+i]=reinterpret_cast<uint32_t>(lightObjects[f.currentIds[i]-1].data());
        auto beforeRecords=std::array<uint8_t,0x108>{};uint8_t*record=nullptr;if(f.h[0]!=65535){record=records.data()+size_t(f.h[0])*0x108;std::memset(record,0x7b,0x108);const uint32_t active=(f.h[2]&4)?0x12345678:0;std::memcpy(record+8,&active,4);std::memcpy(record+0x68,f.cached.data(),72);std::memcpy(record+0xb0,&f.h[4],4);for(unsigned i=0;i<f.h[4];++i){const uint32_t p=reinterpret_cast<uint32_t>(lightObjects[f.cachedIds[i]-1].data());std::memcpy(record+0xb4+4*i,&p,4);}std::memcpy(beforeRecords.data(),record,0x108);}
        std::array<uint8_t,0x44>packet{};const uint16_t index=uint16_t(f.h[0]);std::memcpy(packet.data()+0x40,&index,2);std::array<uint32_t,4>renderer{};renderer[3]=reinterpret_cast<uint32_t>(records.data());std::array<uint8_t,0x88>output;output.fill(0xa5);output[0x24]=(f.h[2]&2)?1:0;std::memcpy(output.data()+0x28,f.initialDraw.data(),72);
        const auto beforePacket=packet;const auto beforeOutput=output;const auto beforeRenderer=renderer;const auto beforeCurrent=current;const auto beforeGate=gate;const auto beforeLimit=limit;const auto beforeLights=lightObjects;NativeContext context{packet.data(),renderer.data(),output.data(),current.data(),scratch.data(),f.point.data(),0,0};const auto beforeFixture=f;
        invoke(image.base+0xd5804,&context);
        if(packet!=beforePacket||renderer!=beforeRenderer||current!=beforeCurrent||gate!=beforeGate||limit!=beforeLimit||lightObjects!=beforeLights||std::memcmp(&f,&beforeFixture,sizeof(f))||(record&&std::memcmp(record,beforeRecords.data(),0x108)))throw std::runtime_error("native input mutation");
        for(size_t i=0;i<output.size();++i)if((i<0x28||i>=0x70)&&output[i]!=beforeOutput[i])throw std::runtime_error("native output boundary");
        result=events;result.h[0]=context.selected?uint32_t((context.selected-reinterpret_cast<uint32_t>(records.data()))/0x108):65535;result.h[1]=context.active;std::memcpy(result.draw.data(),output.data()+0x28,72);std::memcpy(result.scratch.data(),scratch.data(),72);result.count=scratch[18];if(result.count>4)throw std::runtime_error("native result count");for(unsigned i=0;i<result.count;++i)result.ids[i]=lightId(scratch[19+i]);
#else
        SourceModelLightingStateCandidate current,scratch;current.ambient=f.current;for(unsigned i=0;i<f.h[3];++i)current.lights.push_back(f.currentIds[i]);const SourceModelInstanceLightingCandidate*selected=nullptr;
        if(f.h[0]!=65535){auto&r=records[f.h[0]];r.active=(f.h[2]&4)!=0;r.cached.ambient=f.cached;r.cached.lights.clear();for(unsigned i=0;i<f.h[4];++i)r.cached.lights.push_back(f.cachedIds[i]);}
        selected=sourceModelInstanceLightingCandidate(uint16_t(f.h[0]),records);result.h[0]=selected?f.h[0]:65535;result.h[1]=selected&&selected->active;OwnedServices services{f,result};sourceModelCachedLightingCandidate((f.h[2]&1)!=0,selected,(f.h[2]&2)!=0,current,f.point,result.draw,scratch,services);result.scratch=scratch.ambient;result.count=uint32_t(scratch.lights.size());for(unsigned i=0;i<result.count;++i)result.ids[i]=uint32_t(scratch.lights[i]);
#endif
        out.write(reinterpret_cast<const char*>(&result),sizeof(result));++count;
    }
    if(!in.eof()||in.gcount()!=0||!count)throw std::runtime_error("cached blend fixture truncation");out.flush();if(!out)throw std::runtime_error("cached blend output");
#ifdef NATIVE_SOURCE_MODEL_CACHED_LIGHTING
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS cached model lighting cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

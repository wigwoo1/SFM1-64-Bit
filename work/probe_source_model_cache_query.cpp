#include "source_model_cache_query_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
using TestLightingState=std::array<uint32_t,23>;
using QueryEvent=std::array<uint32_t,8>;
struct QueryFixture {std::array<uint32_t,8>h{};std::array<float,3>point{};std::array<TestLightingState,2>states{};std::string name,filter;};
struct QueryTrace {
    std::array<uint32_t,4>h{}; // valid calls,filter comparisons,enabled final,query count
    std::array<QueryEvent,2>events{};
    std::array<uint32_t,29>frame{}; // value,aux,scratch-present,3point words,23state words
};
static_assert(sizeof(QueryTrace)==196,"test-only normalized output");
static QueryEvent makeEvent(uint32_t kind,uint32_t flags,bool special,uintptr_t receiver,const float*point,uint32_t token){QueryEvent e{kind,flags,special?1u:0u,uint32_t(receiver),0,0,0,token};if(point)std::memcpy(e.data()+4,point,12);return e;}
static void applyControl(const QueryFixture&f,unsigned ordinal,uint8_t&enabled){const auto setting=f.h.at(3+ordinal);if(setting!=UINT32_MAX)enabled=uint8_t(setting);}
#ifdef NATIVE_SOURCE_MODEL_CACHE_QUERY
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
static const QueryFixture*fixture=nullptr;static QueryTrace trace{};static uint8_t*sharedControl=nullptr;static uint8_t*imageBase=nullptr;static size_t activeCase=0;
static bool __cdecl valid(uint32_t handle){if(handle!=fixture->h[2])throw std::runtime_error("cached validity receiver");++trace.h[0];return (fixture->h[0]&4)!=0;}
static void* __cdecl cached(uint32_t handle,uint32_t*out,uint32_t flags){const unsigned ordinal=++trace.h[3];if(ordinal>2||handle!=fixture->h[2])throw std::runtime_error("cached query count/receiver");*out=0x90000000u+ordinal;trace.events[ordinal-1]=makeEvent(1,flags,false,handle,nullptr,*out);applyControl(*fixture,ordinal,*sharedControl);return const_cast<uint32_t*>(fixture->states[ordinal-1].data());}
static uint32_t __cdecl point(const float*position,uint32_t*state,uint32_t*aux,uint32_t renderable,uint32_t flags,bool special){const unsigned ordinal=++trace.h[3];if(ordinal>2||renderable!=fixture->h[3])throw std::runtime_error("point query count/receiver");const uint32_t token=0x90000000u+ordinal;trace.events[ordinal-1]=makeEvent(2,flags,special,renderable,position,token);std::memcpy(state,fixture->states[ordinal-1].data(),92);*aux=0xa0000000u+ordinal;applyControl(*fixture,ordinal,*sharedControl);return token;}
static void* __cdecl contains(const char*a,const char*b){++trace.h[1];using Find=void*(__cdecl*)(const char*,const char*);return reinterpret_cast<Find>(imageBase+0x250680)(a,b);}
static void redirect(uint8_t*base,size_t at,void*target,uint8_t opcode=0xe9){auto*p=base+at;p[0]=opcode;const int32_t delta=int32_t(reinterpret_cast<intptr_t>(target)-reinterpret_cast<intptr_t>(p)-5);std::memcpy(p+1,&delta,4);}
struct NativeContext {void*cached=nullptr;void*instance=nullptr;void*drawState=nullptr;const float*point=nullptr;};
// Sixth parameter occupies the original shared-control pointer argument1Ch.
__declspec(naked)static void invoke(void*,NativeContext*,void*,void*,void*,uint8_t*,uint32_t*){__asm{
 push ebp
 mov ebp,esp
 sub esp,200h
 push ebx
 push esi
 push edi
 mov eax,dword ptr[ebp+0ch]
 mov ecx,dword ptr[eax]
 mov dword ptr[ebp-168h],ecx
 mov edi,dword ptr[eax+4]
 mov ecx,dword ptr[eax+8]
 mov dword ptr[ebp-13ch],ecx
 mov ecx,dword ptr[eax+0ch]
 mov edx,dword ptr[ecx]
 mov dword ptr[ebp-150h],edx
 mov edx,dword ptr[ecx+4]
 mov dword ptr[ebp-14ch],edx
 mov edx,dword ptr[ecx+8]
 mov dword ptr[ebp-148h],edx
 mov dword ptr[ebp-15ch],0a4a4a4a4h
 mov dword ptr[ebp-158h],0b8b8b8b8h
 mov dword ptr[ebp-154h],0
 mov dword ptr[ebp-164h],0a7b8c9dah
 mov dword ptr[ebp-178h],042ca8000h
 mov dword ptr[ebp-174h],0c34a8000h
 mov dword ptr[ebp-170h],04397e000h
 call dword ptr[ebp+8]
 mov edi,dword ptr[ebp+20h]
 mov eax,dword ptr[ebp-158h]
 mov dword ptr[edi],eax
 mov eax,dword ptr[ebp-15ch]
 mov dword ptr[edi+4],eax
 xor eax,eax
 cmp dword ptr[ebp-154h],0
 setne al
 mov dword ptr[edi+8],eax
 lea esi,[ebp-178h]
 add edi,0ch
 mov ecx,3
 rep movsd
 lea esi,[ebp-0d8h]
 mov ecx,23
 rep movsd
 pop edi
 pop esi
 pop ebx
 mov esp,ebp
 pop ebp
 ret
}}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"cache query fault case="<<activeCase<<" eip="<<std::hex<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
#else
struct OwnedServices {
    using LightingState=TestLightingState;
    const QueryFixture&f;QueryTrace&trace;uint8_t control;
    bool enabled(){return control!=0;}
    bool valid(uintptr_t handle){if(handle!=f.h[2])throw std::runtime_error("owned validity receiver");++trace.h[0];return (f.h[0]&4)!=0;}
    LightingState cached(uintptr_t handle,uint32_t flags,uintptr_t&value){const unsigned ordinal=++trace.h[3];if(ordinal>2)throw std::runtime_error("owned cached count");value=0x90000000u+ordinal;trace.events[ordinal-1]=makeEvent(1,flags,false,handle,nullptr,uint32_t(value));applyControl(f,ordinal,control);return f.states[ordinal-1];}
    uintptr_t point(const std::array<float,3>&p,LightingState&state,uintptr_t&aux,uintptr_t renderable,uint32_t flags,bool special){const unsigned ordinal=++trace.h[3];if(ordinal>2)throw std::runtime_error("owned point count");const uint32_t token=0x90000000u+ordinal;trace.events[ordinal-1]=makeEvent(2,flags,special,renderable,p.data(),token);state=f.states[ordinal-1];aux=0xa0000000u+ordinal;applyControl(f,ordinal,control);return token;}
};
#endif
int main(int argc,char**argv){try{
    if(argc!=4)throw std::runtime_error("engine.dll input.bin output.bin required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("cache query fixture IO");
#ifdef NATIVE_SOURCE_MODEL_CACHE_QUERY
    static_assert(sizeof(void*)==4,"native x86");AddVectoredExceptionHandler(1,fault);NativePrivateWorldImageFixture image(argv[1]);imageBase=image.base;
    redirect(imageBase,0x193e30,reinterpret_cast<void*>(&valid));redirect(imageBase,0x196a40,reinterpret_cast<void*>(&cached));redirect(imageBase,0x197750,reinterpret_cast<void*>(&point));redirect(imageBase,0xd5b4c,reinterpret_cast<void*>(&contains),0xe8);
    imageBase[0xd58c0]=0xc3;imageBase[0xd5b95]=0xc3;
    std::array<uint8_t,0x34>mode{};std::array<uint8_t,0x28>filterParent{};const uint32_t modeAddress=reinterpret_cast<uint32_t>(mode.data()),filterAddress=reinterpret_cast<uint32_t>(filterParent.data());std::memcpy(imageBase+0x5cbbec,&modeAddress,4);std::memcpy(imageBase+0x5cbb94,&filterAddress,4);image.seal();
#else
    static_assert(sizeof(void*)==8,"owned AMD64");
#endif
    size_t cases=0;QueryFixture f;
    while(in.read(reinterpret_cast<char*>(f.h.data()),32)){
        if((f.h[0]&~127u)||f.h[6]>1024||f.h[7]>1024||(f.h[4]!=UINT32_MAX&&f.h[4]>1)||(f.h[5]!=UINT32_MAX&&f.h[5]>1))throw std::runtime_error("cache query fixture range");
        in.read(reinterpret_cast<char*>(f.point.data()),12);in.read(reinterpret_cast<char*>(f.states.data()),184);f.name.resize(f.h[6]);f.filter.resize(f.h[7]);in.read(f.name.data(),std::streamsize(f.name.size()));in.read(f.filter.data(),std::streamsize(f.filter.size()));if(!in)throw std::runtime_error("cache query fixture truncation");
        if(!(f.h[0]&1))for(float x:f.point)if(!std::isfinite(x))throw std::runtime_error("nonfinite selected cache query point");
        for(const auto&state:f.states)for(unsigned i=0;i<18;++i){float x;std::memcpy(&x,&state[i],4);if(!std::isfinite(x))throw std::runtime_error("nonfinite test lighting payload");}
        QueryTrace result{};
#ifdef NATIVE_SOURCE_MODEL_CACHE_QUERY
        activeCase=cases;fixture=&f;trace={};uint8_t control=(f.h[0]&2)?1:0;sharedControl=&control;
        std::array<uint32_t,3>drawState{0,0,f.h[3]};std::array<uint32_t,2>instance{};std::vector<uint8_t>namedObject(5+f.name.size());std::memcpy(namedObject.data()+4,f.name.data(),f.name.size());if(f.h[0]&16)instance[1]=reinterpret_cast<uint32_t>(namedObject.data());
        std::memcpy(mode.data()+0x30,&f.h[1],4);const uint32_t flags=(f.h[0]&32)?0x1000u:0;std::memcpy(imageBase+0x5cbb8c,&flags,4);const uint32_t filterString=(f.h[0]&64)?reinterpret_cast<uint32_t>(f.filter.c_str()):0;std::memcpy(filterParent.data()+0x24,&filterString,4);
        uint32_t handle=f.h[2];NativeContext context{(f.h[0]&1)?&handle:nullptr,(f.h[0]&8)?instance.data():nullptr,drawState.data(),f.point.data()};
        const auto beforeHeader=f.h;const auto beforePoint=f.point;const auto beforeStates=f.states;const auto beforeName=f.name;const auto beforeFilter=f.filter;const auto beforeDraw=drawState;const auto beforeInstance=instance;const auto beforeNamed=namedObject;const auto beforeMode=mode;const auto beforeParent=filterParent;
        std::array<uint32_t,31>guarded{};guarded.front()=guarded.back()=0x5ba7c193u;invoke(imageBase+0xd5845,&context,nullptr,nullptr,nullptr,&control,guarded.data()+1);
        if(guarded.front()!=0x5ba7c193u||guarded.back()!=0x5ba7c193u||f.h!=beforeHeader||std::memcmp(f.point.data(),beforePoint.data(),12)||f.states!=beforeStates||f.name!=beforeName||f.filter!=beforeFilter||drawState!=beforeDraw||instance!=beforeInstance||namedObject!=beforeNamed||mode!=beforeMode||filterParent!=beforeParent||handle!=f.h[2])throw std::runtime_error("native query input/output mutation");
        trace.h[2]=control;std::memcpy(trace.frame.data(),guarded.data()+1,116);result=trace;
#else
        int32_t diagnosticMode;std::memcpy(&diagnosticMode,&f.h[1],4);SourceModelCacheQuerySnapshotCandidate snapshot;snapshot.renderable=f.h[3];snapshot.point=f.point;snapshot.diagnosticMode=diagnosticMode;snapshot.instanceRecordPresent=(f.h[0]&8)!=0;snapshot.neverAsString=(f.h[0]&32)!=0;if(f.h[0]&1)snapshot.cachedHandle=f.h[2];if(f.h[0]&16)snapshot.instanceName=f.name;if(f.h[0]&64)snapshot.filterString=f.filter;
        OwnedServices services{f,result,uint8_t((f.h[0]&2)!=0)};const auto calculated=sourceModelCacheQueryCandidate(snapshot,services,0xa4a4a4a4u);result.h[1]=calculated.filterCompared;result.h[2]=services.control;result.frame[0]=uint32_t(calculated.value);result.frame[1]=uint32_t(calculated.auxiliary);result.frame[2]=calculated.positionScratch?1u:0u;
        const std::array<float,3>scratch{101.25f,-202.5f,303.75f};std::memcpy(result.frame.data()+3,calculated.positionScratch?calculated.positionScratch->data():scratch.data(),12);std::memcpy(result.frame.data()+6,calculated.state.data(),92);
#endif
        out.write(reinterpret_cast<const char*>(&result),sizeof(result));++cases;
    }
    if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("cache query fixture length");out.flush();if(!out)throw std::runtime_error("cache query output");
#ifdef NATIVE_SOURCE_MODEL_CACHE_QUERY
    image.verify();VirtualFree(imageBase,0,MEM_RELEASE);
#endif
    std::cout<<"PASS model cache query routing cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

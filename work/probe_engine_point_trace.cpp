#include "engine_point_trace_test_fixture.h"
#include <fstream>
#include <iostream>
#include <iterator>
static const EnginePointInputFixture*active=nullptr;static EnginePointEventsFixture events;static size_t activeCase=0;static uint32_t forwardedSwept=UINT32_MAX;
#ifdef NATIVE_ENGINE_POINT_TRACE
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
#include "native_vphysics_library_fixture.h"
#include "native_physics_search_fixture.h"
static uint32_t defaultSurface=0;static std::vector<std::array<uint32_t,2>>entities;
static uint32_t label(const void*p){for(size_t i=0;i<entities.size();++i)if(p==&entities[i])return uint32_t(i+1);throw std::runtime_error("unknown native candidate identity");}
static const uint32_t* __fastcall getHandle(void*self,void*){const auto i=label(self);events.calls.push_back({2,i});return &entities[i-1][1];}
static uint32_t __fastcall worldIndex(void*,void*){return 1;}
static void __fastcall annotateWorld(void*,void*,uint32_t index,void*){if(index!=1)throw std::runtime_error("native world index");}
static uint32_t __fastcall partitionMask(void*,void*){return 1;}
static void __cdecl worldTrace(const uint8_t*ray,uint32_t model,uint32_t mask,uint32_t positions,uint8_t*out){if(model||mask!=active->mask||positions!=1||std::memcmp(ray,active->ray.start.data(),12)||std::memcmp(ray+16,active->ray.delta.data(),12))throw std::runtime_error("native world trace contract");++events.world;searchWriteNativeTrace(out,active->world,defaultSurface);}
static void __fastcall enumerate(void*,void*,uint32_t mask,const uint8_t*ray,bool coarse,void*enumerator){if(mask!=1||coarse)throw std::runtime_error("native partition contract");++events.partition;forwardedSwept=ray[69];if(forwardedSwept!=uint32_t(active->ray.swept))throw std::runtime_error("original changed swept flag during shortening");std::memcpy(events.shortDelta.data(),ray+16,12);const auto vt=*reinterpret_cast<const uintptr_t*const*>(enumerator);using Collect=bool(__thiscall*)(void*,void*);for(auto&e:entities)(void)reinterpret_cast<Collect>(vt[0])(enumerator,&e);}
static void __fastcall resolve(void*,void*,void*entity,void**collideable,void**other){const auto i=label(entity);events.calls.push_back({1,i});*collideable=entity;*other=nullptr;}
static void __fastcall clip(void*,void*,const uint8_t*ray,uint32_t mask,void*collideable,uint8_t*out){const auto i=label(collideable);if(mask!=active->mask||std::memcmp(ray+16,events.shortDelta.data(),12))throw std::runtime_error("native clip contract");events.calls.push_back({3,i});searchWriteNativeTrace(out,active->candidates[i-1].trace,defaultSurface);}
static void __cdecl overflow(int,const char*,...){++events.overflows;}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"engine point fault case="<<activeCase<<" eip="<<std::hex<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
#else
static uintptr_t identity(uint32_t i){return i?0x123400000000ull+uintptr_t(i)*0x100000000ull+0x1234:0;}
static uint32_t label(uintptr_t p){for(size_t i=0;i<active->candidates.size();++i)if(p==identity(uint32_t(i+1)))return uint32_t(i+1);throw std::runtime_error("unknown owned candidate identity");}
struct Services {
    SourcePhysicsSearchTraceCandidate traceWorld(const SourceWorldLightRayCandidate&ray,uint32_t mask){if(mask!=active->mask||ray.start!=active->ray.start||ray.delta!=active->ray.delta)throw std::runtime_error("owned world contract");++events.world;return active->world;}
    std::vector<uintptr_t>enumerateCandidates(const SourceWorldLightRayCandidate&ray){++events.partition;forwardedSwept=uint32_t(ray.swept);if(ray.swept!=active->ray.swept)throw std::runtime_error("owned changed swept flag during shortening");events.shortDelta=ray.delta;std::vector<uintptr_t>out;for(size_t i=0;i<active->candidates.size();++i)out.push_back(identity(uint32_t(i+1)));return out;}
    void candidateOverflow(size_t n){events.overflows+=uint32_t(n);}
    uintptr_t resolveCollideable(uintptr_t e){events.calls.push_back({1,label(e)});return e;}
    uint32_t referenceHandle(uintptr_t e){const auto i=label(e);events.calls.push_back({2,i});return active->candidates[i-1].handle;}
    SourcePhysicsSearchTraceCandidate clipToCollideable(const SourceWorldLightRayCandidate&ray,uint32_t mask,uintptr_t c){const auto i=label(c);if(mask!=active->mask||ray.delta!=events.shortDelta)throw std::runtime_error("owned clip contract");events.calls.push_back({3,i});return active->candidates[i-1].trace;}
};
#endif
int main(int argc,char**argv){try{
    if(argc!=4)throw std::runtime_error("engine input output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);std::ofstream sweptOut(std::string(argv[3])+".swept",std::ios::binary);if(!in||!out||!sweptOut)throw std::runtime_error("engine point streams");
#ifdef NATIVE_ENGINE_POINT_TRACE
    static_assert(sizeof(void*)==4,"original engine x86");AddVectoredExceptionHandler(1,fault);NativePrivateWorldImageFixture image(argv[1]);defaultSurface=*reinterpret_cast<uint32_t*>(image.base+0x402fe8);if(defaultSurface!=0x1035067c)throw std::runtime_error("engine default surface changed");auto jump=[&](size_t rva,void*target){auto*p=image.base+rva;*p=0xe9;const int32_t d=int32_t(reinterpret_cast<uintptr_t>(target)-reinterpret_cast<uintptr_t>(p+5));std::memcpy(p+1,&d,4);};jump(0x149eb0,reinterpret_cast<void*>(&worldTrace));uint32_t profile=0;*reinterpret_cast<uint32_t*>(image.base+0x643ee8)|=1;*reinterpret_cast<uint32_t*>(image.base+0x643ee4)=reinterpret_cast<uint32_t>(&profile);*reinterpret_cast<uint32_t*>(image.base+0x31a480)=reinterpret_cast<uint32_t>(&overflow);
    std::array<uintptr_t,28>table{};table[4]=reinterpret_cast<uintptr_t>(&clip);table[24]=reinterpret_cast<uintptr_t>(&resolve);table[25]=reinterpret_cast<uintptr_t>(&worldIndex);table[26]=reinterpret_cast<uintptr_t>(&annotateWorld);table[27]=reinterpret_cast<uintptr_t>(&partitionMask);std::array<uintptr_t,2>object{reinterpret_cast<uintptr_t>(table.data()),0};std::array<uintptr_t,15>partitionTable{};partitionTable[14]=reinterpret_cast<uintptr_t>(&enumerate);*reinterpret_cast<uintptr_t*>(image.base+0x6683c0)=reinterpret_cast<uintptr_t>(partitionTable.data());std::array<uint32_t,3>entityTable{};entityTable[2]=reinterpret_cast<uint32_t>(&getHandle);const auto beforeManager=std::array<uint32_t,3>{*reinterpret_cast<uint32_t*>(image.base+0x40b598),*reinterpret_cast<uint32_t*>(image.base+0x40b59c),*reinterpret_cast<uint32_t*>(image.base+0x40b5a0)};image.seal();using Trace=void(__thiscall*)(void*,const void*,uint32_t,const void*,void*);const auto trace=reinterpret_cast<Trace>(image.base+0x16ece0);
#else
    static_assert(sizeof(void*)==8,"owned AMD64");Services services;
#endif
    EnginePointInputFixture f;size_t cases=0;while(enginePointReadInput(in,f)){active=&f;activeCase=cases;events={};forwardedSwept=UINT32_MAX;std::ostringstream before(std::ios::binary);enginePointWriteInput(before,f);SourcePhysicsSearchTraceCandidate result;
#ifdef NATIVE_ENGINE_POINT_TRACE
        auto validateIds=[](const auto&t){if(t.surfaceId>0x100000000ull||t.entityId>UINT32_MAX)throw std::runtime_error("native trace identifier range");};validateIds(f.world);for(const auto&c:f.candidates)validateIds(c.trace);entities.clear();for(const auto&c:f.candidates)entities.push_back({reinterpret_cast<uint32_t>(entityTable.data()),c.handle});const auto oldEntities=entities;alignas(16)std::array<uint8_t,80>ray{};std::memcpy(ray.data(),f.ray.start.data(),12);std::memcpy(ray.data()+16,f.ray.delta.data(),12);ray[68]=1;ray[69]=uint8_t(f.ray.swept);const auto oldRay=ray;alignas(16)std::array<uint8_t,128>native;native.fill(0xa7);std::array<uint32_t,3>filter{f.mode==3?0x1035bb48u:0x1031dee4u,f.excluded?reinterpret_cast<uint32_t>(&entities.at(f.excluded-1)):0,0};const auto oldFilter=filter;trace(object.data(),ray.data(),f.mask,filter.data(),native.data()+16);result=searchReadNativeTrace(native.data()+16,defaultSurface);if(ray!=oldRay||filter!=oldFilter||entities!=oldEntities||std::memcmp(image.base+0x40b598,beforeManager.data(),12))throw std::runtime_error("original changed query/filter/entity/manager");for(size_t j=0;j<16;++j)if(native[j]!=0xa7)throw std::runtime_error("leading engine trace canary");for(size_t j=100;j<native.size();++j)if(native[j]!=0xa7)throw std::runtime_error("trailing engine trace canary");if(profile!=cases+1||object[1]!=cases+1)throw std::runtime_error("original profiling counter");
#else
        result=sourceEnginePointTraceCandidate(f.ray,f.mask,{SourceWorldLightTraceModeCandidate(f.mode),identity(f.excluded)},services);
#endif
        std::ostringstream after(std::ios::binary);enginePointWriteInput(after,f);if(before.str()!=after.str())throw std::runtime_error("engine fixture changed");enginePointWriteOutput(out,result,events);supportPut(sweptOut,forwardedSwept);++cases;
    }if(!cases)throw std::runtime_error("no engine point cases");out.flush();sweptOut.flush();if(!out||!sweptOut)throw std::runtime_error("engine point output");
#ifdef NATIVE_ENGINE_POINT_TRACE
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS complete engine point policy cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

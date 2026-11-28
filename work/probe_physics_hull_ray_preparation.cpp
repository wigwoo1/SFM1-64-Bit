#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
#include "source_physics_hull_ray_preparation_candidate.h"
#include "physics_search_test_fixture.h"
struct HullPreparationQueryFixture {std::array<uint8_t,80> raw{};SourceServerHullRayCandidate ray;std::array<float,3> origin{},angles{};uint32_t mask=0;};
static void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
static std::vector<uint8_t> readFile(const char* path){std::ifstream input(path,std::ios::binary);require(bool(input),"PHY file read");return {(std::istreambuf_iterator<char>(input)),{}};}
template<class T>static T readAt(const uint8_t* value,size_t offset){T result;std::memcpy(&result,value+offset,sizeof(result));return result;}
static bool readQuery(std::istream& input,HullPreparationQueryFixture& query){
    if(input.peek()==std::char_traits<char>::eof())return false;query={};supportGet(input,query.raw);supportGet(input,query.origin);supportGet(input,query.angles);supportGet(input,query.mask);
    require(query.raw[68]<=1&&query.raw[69]<=1,"physics hull ray flags");
    std::memcpy(query.ray.center.data(),query.raw.data(),12);std::memcpy(query.ray.delta.data(),query.raw.data()+16,12);std::memcpy(query.ray.offset.data(),query.raw.data()+32,12);std::memcpy(query.ray.extents.data(),query.raw.data()+48,12);query.ray.isRay=query.raw[68]!=0;query.ray.swept=query.raw[69]!=0;
    for(const auto& values:{query.ray.center,query.ray.delta,query.ray.offset,query.ray.extents,query.origin,query.angles})for(float value:values)require(std::isfinite(value),"nonfinite physics hull query");
    for(float angle:query.angles)require(angle>=-36000&&angle<=36000,"physics hull angle domain");for(float extent:query.ray.extents)require(extent>=0,"negative physics hull extent");
    const auto& extents=query.ray.extents;const float xx=extents[0]*extents[0],yy=extents[1]*extents[1],zz=extents[2]*extents[2];require(query.ray.isRay||std::isfinite((xx+yy)+zz),"physics hull extent overflow");return true;
}
#ifdef NATIVE_PHYSICS_HULL_RAY_PREPARATION
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "native_vphysics_library_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
static uintptr_t nativeBase=0;
static uint32_t defaultSurface=0,observations=0;
static size_t activeCase=0;
static std::array<uint8_t,196> captured{};
static std::array<uint8_t,196> capture(const uint8_t* state){
    const auto* ray=readAt<const uint8_t*>(state,0x6c);const auto* motion=readAt<const uint8_t*>(state,0x84);const auto* box=readAt<const uint8_t*>(state,0x64);
    std::array<uint8_t,196> result{};std::memcpy(result.data(),ray,64);size_t offset=64;
    for(size_t field:{0xa0u,0xb0u,0xc0u,0xd0u}){std::memcpy(result.data()+offset,state+field,12);offset+=12;}
    std::memcpy(result.data()+offset,state+0xe0,4);offset+=4;std::memcpy(result.data()+offset,state+0x74,16);offset+=16;
    for(unsigned column=0;column<3;++column){std::memcpy(result.data()+offset,motion+0x50+column*16,12);offset+=12;}
    require(offset==168&&readAt<uint32_t>(box,0)==nativeBase+0xc8090,"actual moving-box binding");std::memcpy(result.data()+168,box+4,24);const uint32_t point=box[32];require(point<=1,"actual moving-box point flag");std::memcpy(result.data()+192,&point,4);return result;
}
static void observe(unsigned index,CONTEXT& context){require(index==0&&context.Eip==nativeBase+0x2b45e,"unexpected physics preparation observation");captured=capture(reinterpret_cast<const uint8_t*>(context.Ebp-0x120));++observations;context.EFlags|=0x10000u;}
static SourcePhysicsSearchTraceCandidate nativeTrace(const uint8_t* value){
    SourcePhysicsSearchTraceCandidate trace;std::memcpy(trace.start.data(),value,12);std::memcpy(trace.end.data(),value+12,12);std::memcpy(trace.plane.normal.data(),value+24,12);trace.plane.distance=readAt<float>(value,36);std::memcpy(trace.planeTags.data(),value+40,4);trace.fraction=readAt<float>(value,44);trace.contents=readAt<uint32_t>(value,48);trace.displacementFlags=readAt<uint16_t>(value,52);trace.allSolid=value[54];trace.startSolid=value[55];trace.fractionLeftSolid=readAt<float>(value,56);require(readAt<uint32_t>(value,60)==defaultSurface,"unexpected physics preparation surface identity");trace.surfaceMetadata=readAt<uint32_t>(value,64);trace.hitGroup=readAt<uint32_t>(value,68);trace.physicsBone=readAt<uint16_t>(value,72);trace.worldSurface=readAt<uint16_t>(value,74);trace.entityId=readAt<uint32_t>(value,76);trace.hitBox=readAt<uint32_t>(value,80);return trace;
}
static LONG CALLBACK fault(EXCEPTION_POINTERS* info){if(info->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"physics hull preparation fault case="<<activeCase<<" eip="<<std::hex<<info->ContextRecord->Eip<<'\n';ExitProcess(90);}
#endif
int main(int argc,char** argv){try{
    require(argc==5,"expected copied libraries, PHY, input, output");std::ifstream input(argv[3],std::ios::binary);std::ofstream output(argv[4],std::ios::binary);require(bool(input)&&bool(output),"physics hull preparation streams");
    const auto phy=readFile(argv[2]);const auto oldPhy=phy;require(phy.size()>=32,"PHY header");const uint32_t header=readAt<uint32_t>(phy.data(),0),count=readAt<uint32_t>(phy.data(),8);require(header>=16&&header<phy.size()&&count==1&&phy.size()-header<=INT32_MAX,"one-solid PHY preparation fixture");
#ifdef NATIVE_PHYSICS_HULL_RAY_PREPARATION
    static_assert(sizeof(void*)==4,"original x86 physics");AddVectoredExceptionHandler(1,fault);NativeVphysicsLibraryFixture image(argv[1]);nativeBase=uintptr_t(image.module);require(image.rva(image.table[31])==0x5e00&&image.rva(image.table[36])==0x90a0&&image.rva(image.table[37])==0x5eb0,"pinned physics vtable");
    defaultSurface=readAt<uint32_t>(static_cast<const uint8_t*>(image.address(0xf7cd0)),0);require(image.rva(defaultSurface)==0xc8050&&readAt<uint32_t>(static_cast<const uint8_t*>(image.address(0xf7cd4)),0)==0,"pinned physics default surface");
    alignas(16)std::array<uint8_t,32> loaded;loaded.fill(0xa7);using Load=void(__thiscall*)(void*,void*,int,const void*,int,bool);using Unload=void(__thiscall*)(void*,void*);using Trace=void(__thiscall*)(void*,const void*,uint32_t,const void*,const void*,const float*,const float*,void*);
    reinterpret_cast<Load>(image.table[36])(image.object,loaded.data()+8,1,phy.data()+header,int(phy.size()-header),false);const auto* shapes=readAt<void**>(loaded.data(),12);require((readAt<uint16_t>(loaded.data(),8)&0x7fff)==1&&shapes&&shapes[0],"actual loaded PHY shape");
    std::ofstream unobserved(std::string(argv[4])+".unobserved-traces",std::ios::binary),observed(std::string(argv[4])+".observed-traces",std::ios::binary);require(bool(unobserved)&&bool(observed),"native physics trace outputs");
    PrivateX86BoundaryObserverFixture observer({uint32_t(nativeBase+0x2b45e),uint32_t(nativeBase+0x2b561),uint32_t(nativeBase+0x2b562),uint32_t(nativeBase+0x2b563)},observe);
#else
    static_assert(sizeof(void*)==8,"owned AMD64 preparation");
#endif
    HullPreparationQueryFixture query;size_t cases=0;
    while(readQuery(input,query)){
#ifdef NATIVE_PHYSICS_HULL_RAY_PREPARATION
        activeCase=cases;auto run=[&](bool watch){alignas(16)auto ray=query.raw;const auto oldRay=ray;std::array<uint8_t,116> result;result.fill(0xa7);observations=0;if(watch)observer.configure(true);reinterpret_cast<Trace>(image.table[31])(image.object,ray.data(),query.mask,nullptr,shapes[0],query.origin.data(),query.angles.data(),result.data()+16);if(watch)observer.configure(false);require(ray==oldRay,"actual physics input ray changed");for(size_t i=0;i<16;++i)require(result[i]==0xa7&&result[100+i]==0xa7,"actual physics trace canaries");require(observations==(watch?1u:0u),"actual full TraceBox preparation count");return result;};
        const auto baseline=run(false),withObserver=run(true);require(baseline==withObserver,"physics observation changed complete native trace");searchWriteTrace(unobserved,nativeTrace(baseline.data()+16));searchWriteTrace(observed,nativeTrace(withObserver.data()+16));supportPut(output,captured);
#else
        const auto result=sourcePhysicsHullRayPreparationCandidate(query.ray,query.origin,query.angles);const auto words=sourcePhysicsRayPreparationWordsCandidate(result.prepared);supportPut(output,words);supportPut(output,result.box.axes);supportPut(output,uint32_t(result.box.point));
#endif
        ++cases;
    }
    require(cases>0&&phy==oldPhy,"physics preparation input preservation");output.flush();require(bool(output),"physics preparation output failure");
#ifdef NATIVE_PHYSICS_HULL_RAY_PREPARATION
    unobserved.flush();observed.flush();require(bool(unobserved)&&bool(observed),"native physics trace output failure");reinterpret_cast<Unload>(image.table[37])(image.object,loaded.data()+8);for(size_t i=0;i<8;++i)require(loaded[i]==0xa7&&loaded[24+i]==0xa7,"loaded PHY canaries");image.verify();
#endif
    std::cout<<"PASS physics hull ray preparation cases="<<cases<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}

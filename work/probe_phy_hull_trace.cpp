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
#include "physics_search_test_fixture.h"
#include "source_server_hull_ray_candidate.h"
#ifdef NATIVE_PHY_HULL_TRACE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "native_vphysics_library_fixture.h"
#include "native_physics_search_fixture.h"
static size_t activeCase=0;
static LONG CALLBACK fault(EXCEPTION_POINTERS* info){if(info->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"PHY hull fault case="<<activeCase<<" eip="<<std::hex<<info->ContextRecord->Eip<<'\n';ExitProcess(90);}
#else
#include "source_phy_hull_trace_candidate.h"
#endif
struct HullTraceQueryFixture {std::array<uint8_t,80> rawRay{};std::array<float,3> origin{},angles{};uint32_t mask=0;};
static void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
static bool readQuery(std::istream& in,HullTraceQueryFixture& query){
    if(in.peek()==std::char_traits<char>::eof())return false;
    query={};supportGet(in,query.rawRay);supportGet(in,query.origin);supportGet(in,query.angles);supportGet(in,query.mask);
    require(query.rawRay[68]<=1&&query.rawRay[69]<=1,"PHY hull ray flags");
    for(unsigned offset:{0u,16u,32u,48u})for(unsigned axis=0;axis<3;++axis){float value;std::memcpy(&value,query.rawRay.data()+offset+axis*4,4);require(std::isfinite(value),"nonfinite PHY hull ray");if(offset==48)require(value>=0,"negative PHY hull extent");}
    for(const auto& values:{query.origin,query.angles})for(float value:values)require(std::isfinite(value),"nonfinite PHY hull pose");
    for(float angle:query.angles)require(angle>=-36000&&angle<=36000,"PHY hull angle domain");return true;
}
#ifndef NATIVE_PHY_HULL_TRACE
static SourceServerHullRayCandidate semanticRay(const std::array<uint8_t,80>& bytes){SourceServerHullRayCandidate ray;std::memcpy(ray.center.data(),bytes.data(),12);std::memcpy(ray.delta.data(),bytes.data()+16,12);std::memcpy(ray.offset.data(),bytes.data()+32,12);std::memcpy(ray.extents.data(),bytes.data()+48,12);ray.isRay=bytes[68]!=0;ray.swept=bytes[69]!=0;return ray;}
#endif
int main(int argc,char** argv){try{
    require(argc==5,"expected copied libraries, PHY, queries, output");std::ifstream phyInput(argv[2],std::ios::binary),queries(argv[3],std::ios::binary);require(bool(phyInput)&&bool(queries),"PHY hull inputs");
    const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(phyInput)),{});const auto before=bytes;std::ofstream output(argv[4],std::ios::binary),prefix(std::string(argv[4])+".trace60",std::ios::binary),statsOutput(std::string(argv[4])+".stats",std::ios::binary);require(bool(output)&&bool(prefix)&&bool(statsOutput),"PHY hull outputs");
#ifdef NATIVE_PHY_HULL_TRACE
    static_assert(sizeof(void*)==4,"original x86 PHY trace");AddVectoredExceptionHandler(1,fault);NativeVphysicsLibraryFixture image(argv[1]);const auto defaultSurface=searchDefaultSurfaceNative(image);
    require(image.rva(image.table[31])==0x5e00&&image.rva(image.table[36])==0x90a0&&image.rva(image.table[37])==0x5eb0,"pinned PHY hull vtable");require(bytes.size()>=32,"PHY hull header");
    const uint32_t header=supportReadNative<uint32_t>(bytes.data(),0),count=supportReadNative<uint32_t>(bytes.data(),8);require(header>=16&&header<bytes.size()&&count==1&&bytes.size()-header<=INT32_MAX,"one-solid PHY hull fixture");
    using Load=void(__thiscall*)(void*,void*,int,const void*,int,bool);using Unload=void(__thiscall*)(void*,void*);using Trace=void(__thiscall*)(void*,const void*,uint32_t,const void*,const void*,const float*,const float*,void*);
    alignas(16)std::array<uint8_t,32> loaded;loaded.fill(0xa7);reinterpret_cast<Load>(image.table[36])(image.object,loaded.data()+8,1,bytes.data()+header,int(bytes.size()-header),false);
    const auto shapes=supportReadNative<void**>(loaded.data(),12);require((supportReadNative<uint16_t>(loaded.data(),8)&0x7fff)==1&&shapes&&shapes[0],"actual loaded PHY hull shape");
#else
    static_assert(sizeof(void*)==8,"owned AMD64 PHY hull trace");const SourcePhyHullTraceCandidate shape(bytes);
#endif
    HullTraceQueryFixture query;size_t cases=0;
    while(readQuery(queries,query)){const auto oldQuery=query;SourcePhysicsSearchTraceCandidate result;
#ifdef NATIVE_PHY_HULL_TRACE
        activeCase=cases;alignas(16)auto ray=query.rawRay;const auto oldRay=ray;std::array<uint8_t,116> native;native.fill(0xa7);
        reinterpret_cast<Trace>(image.table[31])(image.object,ray.data(),query.mask,nullptr,shapes[0],query.origin.data(),query.angles.data(),native.data()+16);
        require(ray==oldRay,"native PHY hull ray changed");for(size_t i=0;i<16;++i)require(native[i]==0xa7&&native[100+i]==0xa7,"native PHY hull trace canaries");result=searchReadNativeTrace(native.data()+16,defaultSurface);
#else
        SourcePhysicsHullTraceStatisticsCandidate statistics;result=shape.traceHull(semanticRay(query.rawRay),query.origin,query.angles,query.mask,&statistics);supportPut(statsOutput,statistics.outcomes);supportPut(statsOutput,statistics.packedSearches);supportPut(statsOutput,statistics.walkSearches);
#endif
        require(std::memcmp(&query,&oldQuery,sizeof(query))==0,"PHY hull query changed");std::ostringstream wire(std::ios::binary);searchWriteTrace(wire,result);const auto record=wire.str();require(record.size()==92,"PHY hull trace wire size");output.write(record.data(),92);prefix.write(record.data(),60);++cases;
    }
    require(cases>0&&bytes==before,"PHY hull input preservation");output.flush();prefix.flush();statsOutput.flush();require(bool(output)&&bool(prefix)&&bool(statsOutput),"PHY hull output failure");
#ifdef NATIVE_PHY_HULL_TRACE
    reinterpret_cast<Unload>(image.table[37])(image.object,loaded.data()+8);for(size_t i=0;i<8;++i)require(loaded[i]==0xa7&&loaded[24+i]==0xa7,"loaded PHY hull canaries");image.verify();
#endif
    std::cout<<"PASS composed PHY hull trace cases="<<cases<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}

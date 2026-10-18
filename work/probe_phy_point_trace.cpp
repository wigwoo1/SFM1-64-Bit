#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include "physics_search_test_fixture.h"
#ifdef NATIVE_PHY_POINT_TRACE
#define NOMINMAX
#include <windows.h>
#include "native_vphysics_library_fixture.h"
#include "native_physics_search_fixture.h"
static uint32_t activeQuery=0;
static std::ofstream*visits=nullptr;
static std::map<const uint8_t*,uint32_t>ordinals;
static void(__thiscall*originalLeaf)(void*,void*)=nullptr;
static void __fastcall observeLeaf(void*self,void*,void*node){const auto*p=static_cast<const uint8_t*>(node);const auto ledge=p+supportReadNative<int32_t>(p,4);const auto it=ordinals.find(ledge);if(it==ordinals.end())throw std::runtime_error("point trace unknown leaf");supportPut(*visits,activeQuery);supportPut(*visits,it->second);originalLeaf(self,node);}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"point trace fault query="<<activeQuery<<" eip="<<std::hex<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
#else
#include "source_phy_point_trace_candidate.h"
#endif
struct PointTraceQuery {std::array<float,3>start{},delta{},origin{},angles{};uint32_t mask=0;};static_assert(sizeof(PointTraceQuery)==52,"point trace query");
int main(int argc,char**argv){try{
    if(argc!=6)throw std::runtime_error("DLL directory PHY queries output cache-mode required");const std::string mode=argv[5];if(mode!="fresh"&&mode!="reuse"&&mode!="wrap")throw std::runtime_error("cache mode");std::ifstream phy(argv[2],std::ios::binary),queries(argv[3],std::ios::binary);if(!phy||!queries)throw std::runtime_error("point trace input");const std::vector<uint8_t>b((std::istreambuf_iterator<char>(phy)),{});const auto before=b;std::ofstream out(argv[4],std::ios::binary),prefix(std::string(argv[4])+".trace60",std::ios::binary),leafVisits(std::string(argv[4])+".visits",std::ios::binary),statsOut(std::string(argv[4])+".stats",std::ios::binary);if(!out||!prefix||!leafVisits||!statsOut)throw std::runtime_error("point trace outputs");
#ifdef NATIVE_PHY_POINT_TRACE
    static_assert(sizeof(void*)==4,"original x86 oracle");AddVectoredExceptionHandler(1,fault);NativeVphysicsLibraryFixture image(argv[1]);const auto defaultSurface=searchDefaultSurfaceNative(image);if(b.size()<32)throw std::runtime_error("native PHY header");const uint32_t header=supportReadNative<uint32_t>(b.data(),0),count=supportReadNative<uint32_t>(b.data(),8);if(header<16||header>=b.size()||count!=1||b.size()-header>INT32_MAX)throw std::runtime_error("native PHY one-solid span");using Load=void(__thiscall*)(void*,void*,int,const void*,int,bool);using Unload=void(__thiscall*)(void*,void*);using Trace=void(__thiscall*)(void*,const void*,uint32_t,const void*,const void*,const float*,const float*,void*);using Create=void*(__thiscall*)(void*,void*);using Destroy=void(__thiscall*)(void*,void*);alignas(16)std::array<uint8_t,32>vc;vc.fill(0xa7);reinterpret_cast<Load>(image.table[36])(image.object,vc.data()+8,1,b.data()+header,int(b.size()-header),false);const auto shapes=supportReadNative<void**>(vc.data(),12);if(!shapes||!shapes[0]||(supportReadNative<uint16_t>(vc.data(),8)&0x7fff)!=1)throw std::runtime_error("native shape");void*query=reinterpret_cast<Create>(image.table[43])(image.object,shapes[0]);if(!query)throw std::runtime_error("native query");const auto qp=static_cast<const uint8_t*>(query);const auto ledges=supportReadNative<const uint8_t*const*>(qp,12);const auto ledgeCount=supportReadNative<uint32_t>(qp,8);if(!ledges||!ledgeCount||ledgeCount>1000000)throw std::runtime_error("native ledges");for(uint32_t i=0;i<ledgeCount;++i)if(!ordinals.emplace(ledges[i],i).second)throw std::runtime_error("native repeated ledge");reinterpret_cast<Destroy>(image.table[44])(image.object,query);visits=&leafVisits;originalLeaf=reinterpret_cast<decltype(originalLeaf)>(image.address(0x2a340));image.observeCall(0x2ade6,0x2a340,reinterpret_cast<void*>(&observeLeaf));image.observeCall(0x2b1fa,0x2a340,reinterpret_cast<void*>(&observeLeaf));
#else
    static_assert(sizeof(void*)==8,"owned AMD64");const SourcePhyPointTraceCandidate shape(b);SourcePhysicsSupportCacheCandidate cache;cache.generation=1;if(mode=="wrap"){cache.generation=65535;cache.entries.fill(65535);}
#endif
    PointTraceQuery q;uint32_t cases=0;while(queries.read(reinterpret_cast<char*>(&q),sizeof(q))){const auto old=q;for(const auto&v:{q.start,q.delta,q.origin,q.angles})for(float x:v)if(!std::isfinite(x))throw std::runtime_error("nonfinite point query");SourcePhysicsSearchTraceCandidate result;
#ifdef NATIVE_PHY_POINT_TRACE
        activeQuery=cases;alignas(16)std::array<uint8_t,80>ray{};std::memcpy(ray.data(),q.start.data(),12);std::memcpy(ray.data()+16,q.delta.data(),12);ray[68]=1;ray[69]=((q.delta[0]*q.delta[0]+q.delta[1]*q.delta[1])+q.delta[2]*q.delta[2])!=0;const auto oldRay=ray;alignas(16)std::array<uint8_t,112>trace;trace.fill(0xa7);reinterpret_cast<Trace>(image.table[31])(image.object,ray.data(),q.mask,nullptr,shapes[0],q.origin.data(),q.angles.data(),trace.data()+16);if(ray!=oldRay)throw std::runtime_error("native ray changed");for(size_t i=0;i<16;++i)if(trace[i]!=0xa7)throw std::runtime_error("native leading trace canary");for(size_t i=100;i<trace.size();++i)if(trace[i]!=0xa7)throw std::runtime_error("native trailing trace canary");result=searchReadNativeTrace(trace.data()+16,defaultSurface);
#else
        SourcePhysicsPointTraceStatisticsCandidate statistics;result=shape.tracePoint(q.start,q.delta,q.origin,q.angles,q.mask,mode=="fresh"?nullptr:&cache,&statistics);for(auto ordinal:statistics.leafOrdinals){supportPut(leafVisits,cases);supportPut(leafVisits,ordinal);}supportPut(statsOut,statistics.outcomes);supportPut(statsOut,statistics.packedSearches);supportPut(statsOut,statistics.walkSearches);supportPut(statsOut,cache.generation);
#endif
        if(std::memcmp(&q,&old,sizeof(q)))throw std::runtime_error("point query changed");std::ostringstream wire(std::ios::binary);searchWriteTrace(wire,result);const auto bytes=wire.str();if(bytes.size()!=92)throw std::runtime_error("semantic point result size");out.write(bytes.data(),92);prefix.write(bytes.data(),60);++cases;
    }if(!queries.eof()||queries.gcount()!=0||!cases)throw std::runtime_error("point query extent");if(b!=before)throw std::runtime_error("PHY bytes changed");out.flush();prefix.flush();leafVisits.flush();statsOut.flush();if(!out||!prefix||!leafVisits||!statsOut)throw std::runtime_error("point output write");
#ifdef NATIVE_PHY_POINT_TRACE
    reinterpret_cast<Unload>(image.table[37])(image.object,vc.data()+8);for(size_t i=0;i<8;++i)if(vc[i]!=0xa7||vc[24+i]!=0xa7)throw std::runtime_error("native vcollide canary");image.verify();
#endif
    std::cout<<"PASS complete file-to-point-trace cases="<<cases<<" cache-mode="<<mode<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

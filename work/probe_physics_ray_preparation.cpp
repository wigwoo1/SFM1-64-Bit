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
struct Query {std::array<float,3>start{},delta{},origin{},angles{};uint32_t mask=0x4481;};
static_assert(sizeof(Query)==52,"point physics preparation wire");
#ifdef NATIVE_PHYSICS_RAY_PREPARATION
#define NOMINMAX
#include <windows.h>
#include "native_vphysics_library_fixture.h"
static size_t active=0;
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"physics prep fault case="<<active<<" code="<<std::hex<<p->ExceptionRecord->ExceptionCode<<" eip="<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
static void(__thiscall*originalPrepare)(void*)=nullptr;static std::array<float,42>observed{};static unsigned observations=0;
static std::array<float,42>capture(const uint8_t*state){
    std::array<float,42>result{};const uint8_t*ray=nullptr,*motion=nullptr;std::memcpy(&ray,state+0x6c,4);std::memcpy(&motion,state+0x84,4);std::memcpy(result.data(),ray,64);size_t at=16;for(size_t offset:{0xa0,0xb0,0xc0,0xd0}){std::memcpy(result.data()+at,state+offset,12);at+=3;}std::memcpy(result.data()+at++,state+0xe0,4);std::memcpy(result.data()+at,state+0x74,16);at+=4;for(size_t col=0;col<3;++col){std::memcpy(result.data()+at,motion+0x50+col*16,12);at+=3;}return result;
}
static void __fastcall observePreparation(void*self,void*){originalPrepare(self);observed=capture(static_cast<const uint8_t*>(self));++observations;}
#else
#include "source_physics_ray_preparation_candidate.h"
#endif
int main(int argc,char**argv){try{
    if(argc!=5)throw std::runtime_error("DLL directory PHY query output required");std::ifstream in(argv[3],std::ios::binary);std::ofstream out(argv[4],std::ios::binary);if(!in||!out)throw std::runtime_error("preparation IO");
#ifdef NATIVE_PHYSICS_RAY_PREPARATION
    static_assert(sizeof(void*)==4,"original prep x86");AddVectoredExceptionHandler(1,fault);NativeVphysicsLibraryFixture image(argv[1]);if(image.rva(image.table[36])!=0x90a0||image.rva(image.table[37])!=0x5eb0)throw std::runtime_error("original physics vtable");
    std::ifstream phy(argv[2],std::ios::binary);if(!phy)throw std::runtime_error("prep PHY read");std::vector<uint8_t>b((std::istreambuf_iterator<char>(phy)),{});if(b.size()<32)throw std::runtime_error("prep PHY header");uint32_t header,count;std::memcpy(&header,b.data(),4);std::memcpy(&count,b.data()+8,4);if(header>=b.size()||header<16||count!=1||b.size()-header>INT32_MAX)throw std::runtime_error("prep fixture requires one original solid");const auto beforeB=b;
    alignas(16)std::array<uint8_t,32>vc{};vc.fill(0xa7);using Load=void(__thiscall*)(void*,void*,int,const void*,int,bool);using Unload=void(__thiscall*)(void*,void*);reinterpret_cast<Load>(image.table[36])(image.object,vc.data()+8,int(count),b.data()+header,int(b.size()-header),false);void**shapes;std::memcpy(&shapes,vc.data()+12,4);if(!shapes||!shapes[0])throw std::runtime_error("prep native collision shape");
    using Motion=void*(__thiscall*)(void*,void*,const float*,const float*);using Ray=void(__thiscall*)(void*,const float*,const float*);using Init=void*(__thiscall*)(void*,void*,void*,void*,void*,const float*,uint32_t,void*);using Prepare=void(__thiscall*)(void*);
#else
    static_assert(sizeof(void*)==8,"owned physics prep AMD64");
#endif
#ifdef NATIVE_PHYSICS_RAY_PREPARATION
    originalPrepare=reinterpret_cast<Prepare>(image.address(0x27400));image.observeCall(0x2b459,0x27400,reinterpret_cast<void*>(&observePreparation));std::ofstream traces(std::string(argv[4])+".traces.bin",std::ios::binary);if(!traces)throw std::runtime_error("prep full trace output");using Trace=void(__thiscall*)(void*,const void*,uint32_t,const void*,const void*,const float*,const float*,void*);
#endif
    size_t cases=0;Query q;while(in.read(reinterpret_cast<char*>(&q),sizeof(q))){for(const auto&v:{q.start,q.delta,q.origin,q.angles})for(float x:v)if(!std::isfinite(x))throw std::runtime_error("nonfinite prep fixture");for(float x:q.angles)if(x< -36000||x>36000)throw std::runtime_error("prep angle range");const auto before=q;std::array<float,42>result{};
#ifdef NATIVE_PHYSICS_RAY_PREPARATION
        active=cases;std::array<float,3>relative{},zero{};for(size_t i=0;i<3;++i)relative[i]=q.start[i]+(-q.origin[i]);
        alignas(16)std::array<uint8_t,192>motion{};alignas(16)std::array<uint8_t,80>ray{};alignas(16)std::array<uint8_t,64>box{};alignas(16)std::array<uint8_t,272>collector{};alignas(16)std::array<uint8_t,112>trace{};motion.fill(0xa7);ray.fill(0xa7);box.fill(0);collector.fill(0xa7);trace.fill(0xa7);
        // Original motion initializer sees the actual loaded collision model.
        reinterpret_cast<Motion>(image.address(0x2a120))(motion.data()+16,shapes[0],zero.data(),q.angles.data());
        reinterpret_cast<Ray>(image.address(0x27310))(ray.data()+8,relative.data(),q.delta.data());const auto beforeMotion=motion;const auto beforeRay=ray;const uint32_t table=reinterpret_cast<uint32_t>(image.address(0xc8090));std::memcpy(box.data()+8,&table,4);box[8+32]=1;
        reinterpret_cast<Init>(image.address(0x295f0))(collector.data()+16,trace.data()+16,box.data()+8,ray.data()+8,motion.data()+16,reinterpret_cast<const float*>(ray.data()+8),q.mask,nullptr);
        reinterpret_cast<Prepare>(image.address(0x27400))(collector.data()+16);
        result=capture(collector.data()+16);
        if(motion!=beforeMotion||ray!=beforeRay)throw std::runtime_error("original prep changed inputs");for(size_t i=0;i<16;++i)if(motion[i]!=0xa7||motion[176+i]!=0xa7||collector[i]!=0xa7||collector[256+i]!=0xa7||trace[i]!=0xa7)throw std::runtime_error("physics prep canary");
        alignas(16)std::array<uint8_t,80>fullRay{};std::memcpy(fullRay.data(),q.start.data(),12);std::memcpy(fullRay.data()+16,q.delta.data(),12);const float xy=q.delta[0]*q.delta[0]+q.delta[1]*q.delta[1],squared=xy+q.delta[2]*q.delta[2];fullRay[68]=1;fullRay[69]=squared!=0;const auto beforeFullRay=fullRay;alignas(16)std::array<uint8_t,112>fullTrace;fullTrace.fill(0xa7);observations=0;reinterpret_cast<Trace>(image.table[31])(image.object,fullRay.data(),q.mask,nullptr,shapes[0],q.origin.data(),q.angles.data(),fullTrace.data()+16);
        if(observations!=1||std::memcmp(result.data(),observed.data(),168))throw std::runtime_error("direct preparation differs from actual full physics trace input");if(fullRay!=beforeFullRay)throw std::runtime_error("full physics ray changed");for(size_t i=0;i<16;++i)if(fullTrace[i]!=0xa7)throw std::runtime_error("full physics leading canary");for(size_t i=100;i<fullTrace.size();++i)if(fullTrace[i]!=0xa7)throw std::runtime_error("full physics trailing canary");traces.write(reinterpret_cast<const char*>(fullTrace.data()+16),60);
#else
        result=sourcePhysicsRayPreparationWordsCandidate(sourcePhysicsRayPreparationCandidate(q.start,q.delta,q.origin,q.angles));
#endif
        if(std::memcmp(&q,&before,sizeof(q)))throw std::runtime_error("prep fixture mutation");for(float x:result)if(!std::isfinite(x))throw std::runtime_error("prep result nonfinite");out.write(reinterpret_cast<const char*>(result.data()),sizeof(result));++cases;
    }if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("prep input extent");out.flush();if(!out)throw std::runtime_error("prep output");
#ifdef NATIVE_PHYSICS_RAY_PREPARATION
    reinterpret_cast<Unload>(image.table[37])(image.object,vc.data()+8);for(size_t i=0;i<8;++i)if(vc[i]!=0xa7||vc[24+i]!=0xa7)throw std::runtime_error("vcollide prep canary");if(b!=beforeB)throw std::runtime_error("prep original bytes changed");image.verify();
    traces.flush();if(!traces)throw std::runtime_error("full physics trace output failure");
#endif
    std::cout<<"PASS physics point-ray preparation cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

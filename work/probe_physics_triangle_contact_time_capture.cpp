#define NOMINMAX
#include <windows.h>
#include "physics_triangle_contact_time_test_fixture.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include "native_vphysics_library_fixture.h"
static float(__thiscall*original)(const void*,const float*,float)=nullptr;
static std::ofstream*captures=nullptr;static uint32_t active=0;static size_t captured=0;
static float __fastcall observe(const void*self,void*,const float*direction,float tolerance){
    PhysicsTriangleContactTimeCaptureFixture r;r.query=active;std::memcpy(&r.input.simplex,self,68);std::memcpy(r.input.direction.data(),direction,12);r.input.tolerance=tolerance;const auto before=r.input;r.output=original(self,direction,tolerance);if(std::memcmp(self,&before.simplex,68)||std::memcmp(direction,before.direction.data(),12))throw std::runtime_error("contact-time original mutated inputs");captures->write(reinterpret_cast<const char*>(&r),sizeof(r));++captured;return r.output;
}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"simplex capture fault query="<<active<<" eip="<<std::hex<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
struct Query {std::array<float,3>start{},delta{},origin{},angles{};uint32_t mask=0;};static_assert(sizeof(Query)==52,"query wire");
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==4,"original physics observer x86");if(argc!=6)throw std::runtime_error("DLL directory PHY query trace captures required");AddVectoredExceptionHandler(1,fault);NativeVphysicsLibraryFixture image(argv[1]);original=reinterpret_cast<decltype(original)>(image.address(0x25f50));for(uint32_t site:{0x29da0u})image.observeCall(site,0x25f50,reinterpret_cast<void*>(&observe));
    std::ifstream phy(argv[2],std::ios::binary);if(!phy)throw std::runtime_error("capture PHY read");std::vector<uint8_t>b((std::istreambuf_iterator<char>(phy)),{});if(b.size()<32)throw std::runtime_error("capture PHY header");uint32_t header,count;std::memcpy(&header,b.data(),4);std::memcpy(&count,b.data()+8,4);if(header<16||header>=b.size()||count!=1||b.size()-header>INT32_MAX)throw std::runtime_error("capture needs one original solid");const auto before=b;
    alignas(16)std::array<uint8_t,32>vc;vc.fill(0xa7);using Load=void(__thiscall*)(void*,void*,int,const void*,int,bool);using Unload=void(__thiscall*)(void*,void*);using Trace=void(__thiscall*)(void*,const void*,uint32_t,const void*,const void*,const float*,const float*,void*);reinterpret_cast<Load>(image.table[36])(image.object,vc.data()+8,1,b.data()+header,int(b.size()-header),false);void**shapes=nullptr;std::memcpy(&shapes,vc.data()+12,4);if(!shapes||!shapes[0])throw std::runtime_error("capture collision shape");
    std::ifstream queries(argv[3],std::ios::binary);std::ofstream traces(argv[4],std::ios::binary),records(argv[5],std::ios::binary);if(!queries||!traces||!records)throw std::runtime_error("capture streams");captures=&records;Query q;
    while(queries.read(reinterpret_cast<char*>(&q),sizeof(q))){const auto old=q;for(const auto&v:{q.start,q.delta,q.origin,q.angles})for(float x:v)if(!std::isfinite(x))throw std::runtime_error("nonfinite capture query");alignas(16)std::array<uint8_t,80>ray{};std::memcpy(ray.data(),q.start.data(),12);std::memcpy(ray.data()+16,q.delta.data(),12);const float xy=q.delta[0]*q.delta[0]+q.delta[1]*q.delta[1],squared=xy+q.delta[2]*q.delta[2];ray[68]=1;ray[69]=squared!=0;const auto beforeRay=ray;alignas(16)std::array<uint8_t,112>result;result.fill(0xa7);
        reinterpret_cast<Trace>(image.table[31])(image.object,ray.data(),q.mask,nullptr,shapes[0],q.origin.data(),q.angles.data(),result.data()+16);if(ray!=beforeRay||std::memcmp(&q,&old,sizeof(q)))throw std::runtime_error("capture input mutation");for(size_t i=0;i<16;++i)if(result[i]!=0xa7)throw std::runtime_error("capture leading canary");for(size_t i=100;i<result.size();++i)if(result[i]!=0xa7)throw std::runtime_error("capture trailing canary");traces.write(reinterpret_cast<const char*>(result.data()+16),60);++active;
    }if(!queries.eof()||queries.gcount()!=0||!active)throw std::runtime_error("capture query extent");traces.flush();records.flush();if(!traces||!records)throw std::runtime_error("capture output write");reinterpret_cast<Unload>(image.table[37])(image.object,vc.data()+8);for(size_t i=0;i<8;++i)if(vc[i]!=0xa7||vc[24+i]!=0xa7)throw std::runtime_error("capture vcollide canary");if(b!=before)throw std::runtime_error("capture PHY bytes changed");image.verify();std::cout<<"PASS original physics queries="<<active<<" triangle contact-time calls="<<captured<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

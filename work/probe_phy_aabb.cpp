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
#ifdef NATIVE_PHY_AABB
#define NOMINMAX
#include <Windows.h>
#include "native_vphysics_library_fixture.h"
#else
#include "source_phy_point_trace_candidate.h"
#endif
template<class T>static T word(const void*b,size_t p){T v;std::memcpy(&v,static_cast<const uint8_t*>(b)+p,sizeof(v));return v;}
int main(int argc,char**argv){try{
    if(argc!=5)throw std::runtime_error("library-directory PHY queries output required");std::ifstream phyFile(argv[2],std::ios::binary),in(argv[3],std::ios::binary);std::ofstream out(argv[4],std::ios::binary);if(!phyFile||!in||!out)throw std::runtime_error("bounds streams");const std::vector<uint8_t>bytes((std::istreambuf_iterator<char>(phyFile)),{});
#ifdef NATIVE_PHY_AABB
    static_assert(sizeof(void*)==4,"native original x86");NativeVphysicsLibraryFixture physics(argv[1]);if(physics.rva(physics.table[23])!=0x5e20)throw std::runtime_error("physics bounds ABI");for(size_t offset:{0xfa3ccu,0xfa3d0u,0xfa3d4u})if(word<uint32_t>(physics.address(offset),0)!=0)throw std::runtime_error("runtime zero origin gate changed");if(bytes.size()<16)throw std::runtime_error("PHY span");const auto header=word<uint32_t>(bytes.data(),0);if(header<16||header>=bytes.size()||word<uint32_t>(bytes.data(),8)!=1||bytes.size()-header>INT32_MAX)throw std::runtime_error("PHY solid count/span");alignas(16)std::array<uint8_t,32>vc;vc.fill(0xa7);using Load=void(__thiscall*)(void*,void*,int,const void*,int,bool);using Unload=void(__thiscall*)(void*,void*);reinterpret_cast<Load>(physics.table[36])(physics.object,vc.data()+8,1,bytes.data()+header,int(bytes.size()-header),false);if((word<uint16_t>(vc.data(),8)&0x7fff)!=1)throw std::runtime_error("native load");void*shape=word<void*>(word<void*>(vc.data(),12),0);
#else
    static_assert(sizeof(void*)==8,"owned x64");const SourcePhyPointTraceCandidate shape(bytes);
#endif
    const auto bytesBefore=bytes;size_t cases=0;while(in.peek()!=std::char_traits<char>::eof()){
        std::array<float,6>q{};if(!in.read(reinterpret_cast<char*>(q.data()),24))throw std::runtime_error("truncated bounds query");for(float x:q)if(!std::isfinite(x))throw std::runtime_error("nonfinite bounds query");const auto before=q;std::array<float,6>answer{};
#ifdef NATIVE_PHY_AABB
        std::array<float,10>guard;guard.fill(-123456);using Bounds=void(__thiscall*)(void*,float*,float*,void*,const float*,const float*);reinterpret_cast<Bounds>(physics.table[23])(physics.object,guard.data()+2,guard.data()+5,shape,q.data(),q.data()+3);if(guard[0]!=-123456||guard[1]!=-123456||guard[8]!=-123456||guard[9]!=-123456)throw std::runtime_error("native bounds canary");std::memcpy(answer.data(),guard.data()+2,24);
#else
        answer=shape.bounds({q[0],q[1],q[2]},{q[3],q[4],q[5]});
#endif
        if(q!=before||std::memcmp(q.data(),before.data(),24))throw std::runtime_error("bounds input changed");for(float x:answer)if(!std::isfinite(x))throw std::runtime_error("bounds result nonfinite");out.write(reinterpret_cast<const char*>(answer.data()),24);++cases;
    }if(!cases)throw std::runtime_error("empty queries");out.flush();if(!out)throw std::runtime_error("bounds output");
#ifdef NATIVE_PHY_AABB
    reinterpret_cast<Unload>(physics.table[37])(physics.object,vc.data()+8);for(unsigned i=0;i<8;++i)if(vc[i]!=0xa7||vc[24+i]!=0xa7)throw std::runtime_error("native vcollide canary");physics.verify();
#endif
    if(bytes!=bytesBefore)throw std::runtime_error("PHY input changed");std::cout<<"PASS physics bounds cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

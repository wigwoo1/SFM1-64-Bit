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
template<class T>static T rawRead(const uint8_t*p,size_t at){T value;std::memcpy(&value,p+at,sizeof(value));return value;}
template<class T>static void put(std::ostream&out,const T&value){out.write(reinterpret_cast<const char*>(&value),sizeof(value));}
#ifdef NATIVE_PHY_RAW_COLLISION
#define NOMINMAX
#include <windows.h>
#include "native_vphysics_library_fixture.h"
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"raw PHY fault eip="<<std::hex<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
#else
#include "source_phy_compact_geometry_candidate.h"
#endif
int main(int argc,char**argv){try{
    if(argc!=5)throw std::runtime_error("DLL directory PHY raw-output mesh-output required");std::ifstream input(argv[2],std::ios::binary);if(!input)throw std::runtime_error("raw PHY input read");const std::vector<uint8_t>b((std::istreambuf_iterator<char>(input)),{});const auto before=b;std::ofstream raw(argv[3],std::ios::binary),mesh(argv[4],std::ios::binary);if(!raw||!mesh)throw std::runtime_error("raw PHY output open");size_t convexes=0,totalTriangles=0,totalPoints=0;
#ifdef NATIVE_PHY_RAW_COLLISION
    static_assert(sizeof(void*)==4,"original x86");AddVectoredExceptionHandler(1,fault);NativeVphysicsLibraryFixture image(argv[1]);if(b.size()<32)throw std::runtime_error("raw native PHY short header");const uint32_t header=rawRead<uint32_t>(b.data(),0),count=rawRead<uint32_t>(b.data(),8);if(header<16||header>=b.size()||!count||count>1024||b.size()-header>INT32_MAX)throw std::runtime_error("raw native PHY header");alignas(16)std::array<uint8_t,32>vc;vc.fill(0xa7);
    using Load=void(__thiscall*)(void*,void*,int,const void*,int,bool);using Unload=void(__thiscall*)(void*,void*);using Create=void*(__thiscall*)(void*,void*);using Count=int(__thiscall*)(void*);using Indexed=int(__thiscall*)(void*,int);using Vertices=void(__thiscall*)(void*,int,int,float*);using Destroy=void(__thiscall*)(void*,void*);
    reinterpret_cast<Load>(image.table[36])(image.object,vc.data()+8,int(count),b.data()+header,int(b.size()-header),false);const uint16_t loaded=rawRead<uint16_t>(vc.data(),8)&0x7fff;auto shapes=rawRead<void**>(vc.data(),12);if(loaded!=count||!shapes)throw std::runtime_error("raw native solid count");put(raw,loaded);put(mesh,loaded);
    for(uint16_t s=0;s<loaded;++s){if(!shapes[s])throw std::runtime_error("raw native shape missing");void*query=reinterpret_cast<Create>(image.table[43])(image.object,shapes[s]);if(!query)throw std::runtime_error("raw native query missing");const auto qt=*reinterpret_cast<uintptr_t**>(query);const int countConvex=reinterpret_cast<Count>(qt[1])(query);if(countConvex<0||countConvex>65536)throw std::runtime_error("raw native convex extent");const auto ledges=rawRead<const uint8_t*const*>(static_cast<uint8_t*>(query),12);if(countConvex&&!ledges)throw std::runtime_error("raw native ledge array");put(raw,uint32_t(countConvex));put(mesh,uint32_t(countConvex));
        for(int c=0;c<countConvex;++c){const auto ledge=ledges[c];if(!ledge)throw std::runtime_error("raw native null ledge");const int triangles=reinterpret_cast<Indexed>(qt[2])(query,c),gameData=reinterpret_cast<Indexed>(qt[3])(query,c);if(triangles<1||triangles>32767)throw std::runtime_error("raw native triangles");const auto h=rawRead<std::array<uint32_t,4>>(ledge,0);if((h[3]&0xffff)!=uint32_t(triangles)||h[1]!=uint32_t(gameData))throw std::runtime_error("raw native header/getter disagreement");uint32_t last=0;
            for(int t=0;t<triangles;++t)for(unsigned corner=1;corner<4;++corner){const uint32_t index=rawRead<uint32_t>(ledge,16+size_t(t)*16+corner*4)&0xffff;if(index>last)last=index;}if(last>=32768)throw std::runtime_error("raw native feature range");put(raw,h);put(raw,last+1);raw.write(reinterpret_cast<const char*>(ledge+16),size_t(triangles)*16);const auto points=ledge+rawRead<int32_t>(ledge,0);raw.write(reinterpret_cast<const char*>(points),size_t(last+1)*16);put(mesh,uint32_t(gameData));put(mesh,uint32_t(triangles));
            for(int t=0;t<triangles;++t){std::array<float,11>v;v.fill(-123456);reinterpret_cast<Vertices>(qt[4])(query,c,t,v.data()+1);if(v.front()!=-123456||v.back()!=-123456)throw std::runtime_error("raw native vertex canary");for(size_t j=1;j<10;++j)if(!std::isfinite(v[j]))throw std::runtime_error("raw native nonfinite triangle");mesh.write(reinterpret_cast<const char*>(v.data()+1),36);}++convexes;totalTriangles+=triangles;totalPoints+=last+1;
        }reinterpret_cast<Destroy>(image.table[44])(image.object,query);
    }reinterpret_cast<Unload>(image.table[37])(image.object,vc.data()+8);for(size_t i=0;i<8;++i)if(vc[i]!=0xa7||vc[24+i]!=0xa7)throw std::runtime_error("raw vcollide canary");image.verify();
#else
    static_assert(sizeof(void*)==8,"owned AMD64");const SourcePhyCompactGeometryCandidate geometry(b,true);put(raw,uint16_t(geometry.solids.size()));put(mesh,uint16_t(geometry.solids.size()));
    for(const auto&solid:geometry.solids){put(raw,uint32_t(solid.size()));put(mesh,uint32_t(solid.size()));for(const auto&convex:solid){put(raw,convex.rawLedgeHeader);put(raw,uint32_t(convex.rawPoints.size()));for(const auto&t:convex.rawTriangles)put(raw,t);for(const auto&p:convex.rawPoints)put(raw,p);put(mesh,convex.gameData);put(mesh,uint32_t(convex.triangles.size()));for(const auto&t:convex.triangles)put(mesh,t);++convexes;totalTriangles+=convex.triangles.size();totalPoints+=convex.rawPoints.size();}}
#endif
    if(b!=before)throw std::runtime_error("raw PHY input mutation");raw.flush();mesh.flush();if(!raw||!mesh)throw std::runtime_error("raw PHY output write");std::cout<<"PASS raw PHY convexes="<<convexes<<" triangles="<<totalTriangles<<" point-records="<<totalPoints<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

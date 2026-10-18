#include "source_displacement_ray_triangle_candidate.h"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#ifdef SFM64_NATIVE_RAY_TRIANGLE_ORACLE
#define NOMINMAX
#include <windows.h>
#include "native_displacement_ray_triangle_bytes.h"
#endif
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("input output required");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace files");
#ifdef SFM64_NATIVE_RAY_TRIANGLE_ORACLE
 auto base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x340000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("native base unavailable");for(const auto&f:rayTriangleFragments)std::memcpy(base+f.offset,f.bytes,f.size);for(const auto&c:rayTriangleConstants)std::memcpy(base+c.offset,c.bytes,c.size);DWORD old;if(!VirtualProtect(base+0x140000,0x10000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protection failed");FlushInstructionCache(GetCurrentProcess(),base,0x340000);
 using Function=float(__cdecl*)(const void*,const float*,const float*,const float*,bool);auto native=reinterpret_cast<Function>(base+0x14dd40);
#endif
 std::array<uint32_t,16>query{};size_t cases=0;
 while(in.read(reinterpret_cast<char*>(query.data()),64)){
  if(query[15]>1)throw std::runtime_error("one-sided fixture flag");std::array<std::array<float,3>,3>points{};std::array<float,3>start{},delta{};std::memcpy(points.data(),query.data(),36);std::memcpy(start.data(),query.data()+9,12);std::memcpy(delta.data(),query.data()+12,12);float fraction=0;
#ifdef SFM64_NATIVE_RAY_TRIANGLE_ORACLE
  std::array<uint8_t,80>ray{};std::memcpy(ray.data(),start.data(),12);std::memcpy(ray.data()+16,delta.data(),12);ray[68]=1;ray[69]=1;fraction=native(ray.data(),points[0].data(),points[1].data(),points[2].data(),query[15]!=0);
#else
  fraction=sourceDisplacementRayTriangleCandidate(start,delta,points,query[15]!=0);
#endif
  if(!std::isfinite(fraction))throw std::runtime_error("nonfinite ray result");out.write(reinterpret_cast<const char*>(&fraction),4);++cases;
 }
 if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failure");
#ifdef SFM64_NATIVE_RAY_TRIANGLE_ORACLE
 for(const auto&f:rayTriangleFragments)if(std::memcmp(base+f.offset,f.bytes,f.size))throw std::runtime_error("original code changed");VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS ray triangle cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

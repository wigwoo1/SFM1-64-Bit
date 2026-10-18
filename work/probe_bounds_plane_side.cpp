#include "source_bounds_plane_side_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#ifdef NATIVE_BOUNDS_SIDE
#define NOMINMAX
#include <windows.h>
#include "native_bounds_plane_side_bytes.h"
#endif
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("queries output required");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace files");
#ifdef NATIVE_BOUNDS_SIDE
 auto base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x250000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("base unavailable");std::memcpy(base+0x2446e0,nativeBoundsSideBytes,sizeof(nativeBoundsSideBytes));DWORD old;if(!VirtualProtect(base+0x240000,0x10000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protection failed");FlushInstructionCache(GetCurrentProcess(),base,0x250000);using Function=uint32_t(__cdecl*)(const float*,const float*,const void*);auto native=reinterpret_cast<Function>(base+0x2446e0);
#endif
 std::array<uint32_t,12>q{};size_t count=0;while(in.read(reinterpret_cast<char*>(q.data()),48)){std::array<float,3>mins,maxs,normal;float distance;std::memcpy(mins.data(),q.data(),12);std::memcpy(maxs.data(),q.data()+3,12);std::memcpy(normal.data(),q.data()+6,12);std::memcpy(&distance,q.data()+9,4);uint32_t result;
#ifdef NATIVE_BOUNDS_SIDE
 std::array<uint8_t,20>plane{};std::memcpy(plane.data(),normal.data(),12);std::memcpy(plane.data()+12,&distance,4);plane[16]=uint8_t(q[10]);plane[17]=uint8_t(q[11]);result=native(mins.data(),maxs.data(),plane.data());
#else
 result=sourceBoundsPlaneSideCandidate(mins,maxs,normal,distance,uint8_t(q[10]),uint8_t(q[11]));
#endif
 out.write(reinterpret_cast<char*>(&result),4);++count;
 }if(!in.eof()||in.gcount()!=0||!count)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failure");
#ifdef NATIVE_BOUNDS_SIDE
 if(std::memcmp(base+0x2446e0,nativeBoundsSideBytes,sizeof(nativeBoundsSideBytes)))throw std::runtime_error("body changed");VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

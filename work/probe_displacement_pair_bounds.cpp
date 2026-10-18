#include "source_displacement_pair_bounds_candidate.h"
#include <fstream>
#include <cstring>
#include <iostream>
#ifdef NATIVE_PAIR_BOUNDS
#define NOMINMAX
#include <windows.h>
#include "native_displacement_pair_bounds_bytes.h"
#endif
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("queries output required");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace files");
#ifdef NATIVE_PAIR_BOUNDS
 auto base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x420000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("base unavailable");for(const auto&f:pairBoundsFragments)std::memcpy(base+f.offset,f.bytes,f.size);float high=std::numeric_limits<float>::max(),low=-high;std::memcpy(base+0x31debc,&high,4);std::memcpy(base+0x33ef78,&low,4);DWORD old;for(size_t offset:{size_t(0x150000),size_t(0x240000),size_t(0x2d0000)})if(!VirtualProtect(base+offset,0x10000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protection failed");FlushInstructionCache(GetCurrentProcess(),base,0x420000);using Function=void(__thiscall*)(void*,uint32_t,float*,float*);auto native=reinterpret_cast<Function>(base+0x159a40);
#endif
 std::array<uint32_t,24>q{};size_t count=0;while(in.read(reinterpret_cast<char*>(q.data()),96)){std::vector<std::array<float,3>>vertices(6);std::memcpy(vertices.data(),q.data(),72);std::array<uint16_t,6>packed{};for(size_t i=0;i<6;++i)packed[i]=uint16_t(q[18+i]);std::array<float,6>bounds{};
#ifdef NATIVE_PAIR_BOUNDS
  std::array<uint8_t,0xf4>object{};std::array<std::array<uint8_t,24>,2>triangles{};std::array<int16_t,2>pair{0,1};for(size_t i=0;i<6;++i)std::memcpy(triangles[i/3].data()+2*(i%3),&packed[i],2);auto pointer=[&](size_t offset,const void*p){uint32_t address=reinterpret_cast<uint32_t>(p);std::memcpy(object.data()+offset,&address,4);};pointer(0x70,vertices.data());pointer(0x80,triangles.data());pointer(0xa0,pair.data());native(object.data(),0,bounds.data(),bounds.data()+3);
#else
  bounds=sourceDisplacementPairBoundsCandidate(vertices,packed);
#endif
  out.write(reinterpret_cast<char*>(bounds.data()),24);++count;
 }if(!in.eof()||in.gcount()!=0||!count)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failure");
#ifdef NATIVE_PAIR_BOUNDS
 for(const auto&f:pairBoundsFragments)if(std::memcmp(base+f.offset,f.bytes,f.size))throw std::runtime_error("original body changed");VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

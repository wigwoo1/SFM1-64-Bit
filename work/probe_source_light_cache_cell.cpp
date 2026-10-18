#include "source_light_cache_cell_candidate.h"
#include <fstream>
#include <iostream>
#include <vector>
#ifdef SFM64_NATIVE_CELL_ORACLE
#include <windows.h>
#include <cstring>
#include "native_light_cache_cell_bytes.h"
#endif
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("usage: probe input.bin output.bin");
 std::ifstream input(argv[1],std::ios::binary);std::ofstream output(argv[2],std::ios::binary);
 if(!input||!output)throw std::runtime_error("workspace test file open failed");
#ifdef SFM64_NATIVE_CELL_ORACLE
 auto base=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x6d0000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
 if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("isolated preferred-base allocation unavailable");
 std::memcpy(base+0x192950,nativeCellBytes,sizeof(nativeCellBytes));
 std::memcpy(base+0x2ded50,nativeTruncateBytes,sizeof(nativeTruncateBytes));
 std::memcpy(base+0x3593ec,nativeWidthXY,4);std::memcpy(base+0x322e30,nativeWidthZ,4);
 *reinterpret_cast<uint32_t*>(base+0x6c1228)=1; // Original CRT SSE conversion branch.
 DWORD old=0;if(!VirtualProtect(base+0x190000,0x10000,PAGE_EXECUTE_READ,&old)||!VirtualProtect(base+0x2d0000,0x10000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("oracle executable protection failed");
 FlushInstructionCache(GetCurrentProcess(),base,0x6d0000);
 using Function=void(__cdecl*)(const float*,float*,float*);
 const auto native=reinterpret_cast<Function>(base+0x192950);
#endif
 size_t count=0;std::array<float,3>point{};
 while(input.read(reinterpret_cast<char*>(point.data()),sizeof(point))){
  std::array<float,6>bounds{};
#ifdef SFM64_NATIVE_CELL_ORACLE
  native(point.data(),bounds.data(),bounds.data()+3);
#else
  bounds=sourceLightCacheCellCandidate(point);
#endif
  output.write(reinterpret_cast<const char*>(bounds.data()),sizeof(bounds));++count;
 }
 if(!input.eof()||input.gcount()!=0||!count)throw std::runtime_error("invalid query input length");
 output.flush();if(!output)throw std::runtime_error("bounds output failed");
#ifdef SFM64_NATIVE_CELL_ORACLE
 VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

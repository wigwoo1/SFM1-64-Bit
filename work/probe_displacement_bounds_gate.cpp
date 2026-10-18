#include "source_displacement_bounds_gate_candidate.h"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#ifdef SFM64_NATIVE_DISP_BOUNDS_ORACLE
#define NOMINMAX
#include <windows.h>
#include "native_displacement_bounds_gate_bytes.h"
#endif
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("input output required");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace files");
#ifdef SFM64_NATIVE_DISP_BOUNDS_ORACLE
 auto base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x420000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("native base unavailable");std::memcpy(base+0x14e790,nativeDispBoundsBytes,sizeof(nativeDispBoundsBytes));for(const auto&c:dispBoundsConstants)std::memcpy(base+c.offset,c.bytes,c.size);DWORD old;if(!VirtualProtect(base+0x140000,0x10000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protection failed");FlushInstructionCache(GetCurrentProcess(),base,0x420000);using Function=bool(__fastcall*)(const float*,const float*,const float*,const float*,const float*,float);auto native=reinterpret_cast<Function>(base+0x14e790);
#endif
 std::array<float,21>q{};size_t cases=0;while(in.read(reinterpret_cast<char*>(q.data()),84)){uint32_t result=0;
#ifdef SFM64_NATIVE_DISP_BOUNDS_ORACLE
 result=native(q.data(),q.data()+4,q.data()+8,q.data()+12,q.data()+16,q[20])?1u:0u;
#else
 auto point=[&](size_t at){return std::array<float,3>{q[at],q[at+1],q[at+2]};};result=sourceDisplacementBoundsGateCandidate(point(0),point(4),point(8),point(12),point(16),q[20])?1u:0u;
#endif
 out.write(reinterpret_cast<const char*>(&result),4);++cases;}
 if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failure");
#ifdef SFM64_NATIVE_DISP_BOUNDS_ORACLE
 if(std::memcmp(base+0x14e790,nativeDispBoundsBytes,sizeof(nativeDispBoundsBytes)))throw std::runtime_error("original bounds code changed");VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS bounds gates="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

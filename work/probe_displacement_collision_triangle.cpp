#include "source_displacement_collision_triangle_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#ifdef SFM64_NATIVE_TRIANGLE_METADATA_ORACLE
#define NOMINMAX
#include <windows.h>
#include "native_displacement_triangle_metadata_bytes.h"
#endif
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("input output required");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace files");
#ifdef SFM64_NATIVE_TRIANGLE_METADATA_ORACLE
 auto base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x340000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("native base unavailable");for(const auto&f:triangleMetadataFragments)std::memcpy(base+f.offset,f.bytes,f.size);float one=1;uint32_t epsilon=0x34000000;std::memcpy(base+0x31bdec,&one,4);std::memcpy(base+0x322ae8,&epsilon,4);DWORD old;for(size_t offset:{size_t(0x150000),size_t(0x240000)})if(!VirtualProtect(base+offset,0x10000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protection failed");FlushInstructionCache(GetCurrentProcess(),base,0x340000);
 using Function=void(__thiscall*)(void*,void*);auto plane=reinterpret_cast<Function>(base+0x159170),extrema=reinterpret_cast<Function>(base+0x1592e0);
#endif
 uint32_t vertexCount=0;size_t cases=0;
 while(in.read(reinterpret_cast<char*>(&vertexCount),4)){
  if(vertexCount==0||vertexCount>512)throw std::runtime_error("fixture vertex count");std::vector<std::array<float,3>>vertices(vertexCount);SourceDisplacementCollisionTriangleCandidate initial;
  if(!in.read(reinterpret_cast<char*>(vertices.data()),vertexCount*12)||!in.read(reinterpret_cast<char*>(&initial),24))throw std::runtime_error("truncated fixture");
  SourceDisplacementCollisionTriangleCandidate result;
#ifdef SFM64_NATIVE_TRIANGLE_METADATA_ORACLE
  for(auto index:initial.packedVertices)if(size_t(index&511u)>=vertices.size())throw std::runtime_error("native fixture index");for(const auto&p:vertices)for(float v:p)if(!std::isfinite(v))throw std::runtime_error("native nonfinite fixture");
  struct Guarded{uint32_t before;SourceDisplacementCollisionTriangleCandidate triangle;uint32_t after;}guard{0xabcde123,initial,0xabcde123};uint32_t pointer=reinterpret_cast<uint32_t>(vertices.data());plane(&guard.triangle,&pointer);extrema(&guard.triangle,&pointer);if(guard.before!=0xabcde123||guard.after!=0xabcde123)throw std::runtime_error("triangle guard changed");result=guard.triangle;
#else
  result=sourceDisplacementCollisionTriangleCandidate(vertices,initial);
#endif
  for(float v:result.normal)if(!std::isfinite(v))throw std::runtime_error("nonfinite output normal");if(!std::isfinite(result.distance))throw std::runtime_error("nonfinite output distance");
  out.write(reinterpret_cast<const char*>(&result),24);++cases;
 }
 if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failure");
#ifdef SFM64_NATIVE_TRIANGLE_METADATA_ORACLE
 for(const auto&f:triangleMetadataFragments)if(std::memcmp(base+f.offset,f.bytes,f.size))throw std::runtime_error("native code changed");VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS triangle metadata cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

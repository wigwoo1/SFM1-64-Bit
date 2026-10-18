#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#ifdef SFM64_NATIVE_TRIANGLE_ORACLE
#define NOMINMAX
#include <windows.h>
#include "native_displacement_triangle_bytes.h"
#else
#include "source_displacement_grid_triangles_candidate.h"
#endif

int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("input output required");
 std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);
 if(!in||!out)throw std::runtime_error("workspace files");
#ifdef SFM64_NATIVE_TRIANGLE_ORACLE
 auto base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x150000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
 if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("native base unavailable");
 for(const auto&f:triangleFragments)std::memcpy(base+f.offset,f.bytes,f.size);
 DWORD old;if(!VirtualProtect(base,0x150000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("native protection failed");
 FlushInstructionCache(GetCurrentProcess(),base,0x150000);
 using Function=void(__thiscall*)(void*);
 auto generate=reinterpret_cast<Function>(base+0x13dc30),copy=reinterpret_cast<Function>(base+0x141c70);
#endif
 uint32_t power=0;size_t cases=0;
 while(in.read(reinterpret_cast<char*>(&power),4)){
  if(power<1||power>4)throw std::runtime_error("unsupported fixture power");
  const uint32_t cells=1u<<power,width=cells+1,triangleCount=cells*cells*2,indexCount=triangleCount*3;
  std::vector<uint16_t>tags(triangleCount);
  if(!in.read(reinterpret_cast<char*>(tags.data()),triangleCount*2))throw std::runtime_error("truncated tags");
  std::vector<uint16_t>indices(indexCount);
  std::vector<std::array<uint16_t,4>>records(triangleCount);
#ifdef SFM64_NATIVE_TRIANGLE_ORACLE
  constexpr uint16_t guard=0xa55a;
  std::vector<uint16_t>protectedIndices(indexCount+16,guard);
  std::vector<std::array<uint16_t,4>>protectedRecords(triangleCount+2);
  for(auto&r:protectedRecords)r.fill(guard);
  for(size_t t=0;t<triangleCount;++t)protectedRecords[t+1][3]=tags[t];
  std::array<uint8_t,0x30c>object{};
  auto word=[&](size_t at,uint32_t value){std::memcpy(object.data()+at,&value,4);};
  word(12,power);word(0x304,0xcccccccc);
  word(0x308,reinterpret_cast<uint32_t>(protectedIndices.data()+8));
  word(0x2fc,reinterpret_cast<uint32_t>(protectedRecords.data()+1));
  generate(object.data());uint32_t generated=0;std::memcpy(&generated,object.data()+0x304,4);
  if(generated!=indexCount)throw std::runtime_error("native index count");
  copy(object.data());
  for(size_t g=0;g<8;++g)if(protectedIndices[g]!=guard||protectedIndices[indexCount+8+g]!=guard)throw std::runtime_error("native index guard changed");
  for(size_t a=0;a<4;++a)if(protectedRecords.front()[a]!=guard||protectedRecords.back()[a]!=guard)throw std::runtime_error("native record guard changed");
  std::copy(protectedIndices.begin()+8,protectedIndices.begin()+8+indexCount,indices.begin());
  std::copy(protectedRecords.begin()+1,protectedRecords.end()-1,records.begin());
#else
  const auto triangles=sourceDisplacementGridTrianglesCandidate(power,tags);
  for(size_t t=0;t<triangles.size();++t){for(size_t a=0;a<3;++a)indices[t*3+a]=records[t][a]=triangles[t].vertices[a];records[t][3]=triangles[t].tags;}
#endif
  for(auto index:indices)if(index>=width*width)throw std::runtime_error("vertex outside grid");
  for(size_t t=0;t<triangleCount;++t){if(records[t][3]!=tags[t])throw std::runtime_error("tag word changed");for(size_t a=0;a<3;++a)if(records[t][a]!=indices[t*3+a])throw std::runtime_error("triangle copy mismatch");}
  out.write(reinterpret_cast<const char*>(&indexCount),4);
  out.write(reinterpret_cast<const char*>(indices.data()),indexCount*2);
  out.write(reinterpret_cast<const char*>(records.data()),triangleCount*8);++cases;
 }
 if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failed");
#ifdef SFM64_NATIVE_TRIANGLE_ORACLE
 for(const auto&f:triangleFragments)if(std::memcmp(base+f.offset,f.bytes,f.size))throw std::runtime_error("native code changed");
 VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS triangle grids="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

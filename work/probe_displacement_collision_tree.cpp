#include "source_displacement_collision_tree_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#ifdef SFM64_NATIVE_TREE_ORACLE
#define NOMINMAX
#include <windows.h>
#include <malloc.h>
#include <memory>
#include "native_displacement_tree_bytes.h"
// Enter only the original bounds-finishing block with its original frame.
// Original initializer's allocation/vertex setup calls are deliberately skipped.
__declspec(naked) void __fastcall finishOriginalBounds(void*,void*,void*){
 __asm {push ebp
        mov ebp,esp
        push esi
        mov esi,ecx
        mov eax,1015c6d1h
        jmp eax}
}
#endif

int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("input output required");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace files");
#ifdef SFM64_NATIVE_TREE_ORACLE
 auto base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x420000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("native base unavailable");
 for(const auto&f:treeFragments)std::memcpy(base+f.offset,f.bytes,f.size);
 const float high=std::numeric_limits<float>::max(),low=-high,one=1.f;std::memcpy(base+0x31debc,&high,4);std::memcpy(base+0x33ef78,&low,4);std::memcpy(base+0x31bdec,&one,4);
 DWORD old;for(size_t offset:{size_t(0x150000),size_t(0x240000),size_t(0x2d0000)})if(!VirtualProtect(base+offset,0x10000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protection failed");FlushInstructionCache(GetCurrentProcess(),base,0x420000);
 using Builder=void(__thiscall*)(void*);auto builder=reinterpret_cast<Builder>(base+0x15b8e0);
 using Bounds=void(__thiscall*)(void*,uint32_t,float*,float*);auto bounds=reinterpret_cast<Bounds>(base+0x159a40);
#endif
 uint32_t power=0;size_t cases=0;
 while(in.read(reinterpret_cast<char*>(&power),4)){
  if(power<1||power>4)throw std::runtime_error("unsupported power");
  const uint32_t cells=1u<<power,leafCount=cells*cells,internalCount=(leafCount-1)/3;
  std::vector<std::array<float,3>>vertices(size_t(cells+1)*(cells+1));std::vector<std::array<uint16_t,3>>triangles(size_t(leafCount)*2);
  if(!in.read(reinterpret_cast<char*>(vertices.data()),std::streamsize(vertices.size()*12))||!in.read(reinterpret_cast<char*>(triangles.data()),std::streamsize(triangles.size()*6)))throw std::runtime_error("truncated grid");
  SourceDisplacementCollisionTreeCandidate result;
#ifdef SFM64_NATIVE_TREE_ORACLE
  for(const auto&v:vertices)for(float x:v)if(!std::isfinite(x))throw std::runtime_error("nonfinite native fixture");
  for(const auto&t:triangles)for(auto x:t)if(size_t(x&511u)>=vertices.size())throw std::runtime_error("native fixture reference");
  std::array<uint8_t,0xf4>object{};
  std::vector<std::array<uint8_t,24>>runtimeTriangles(triangles.size());for(size_t i=0;i<triangles.size();++i)std::memcpy(runtimeTriangles[i].data(),triangles[i].data(),6);
  std::vector<std::array<uint16_t,2>>pairs(leafCount+4);for(auto&p:pairs)p.fill(0xa55a);
  auto freeAligned=[](void*p){_aligned_free(p);};std::unique_ptr<void,decltype(freeAligned)>nodeOwner(_aligned_malloc(size_t(internalCount+2)*96,16),freeAligned);if(!nodeOwner)throw std::runtime_error("aligned nodes allocation");auto nodeBytes=static_cast<uint8_t*>(nodeOwner.get());std::memset(nodeBytes,0x5a,size_t(internalCount+2)*96);
  auto word=[&](size_t at,uint32_t value){std::memcpy(object.data()+at,&value,4);};auto pointer=[&](size_t at,const void*p){word(at,reinterpret_cast<uint32_t>(p));};
  word(0x28,power);word(0x78,uint32_t(vertices.size()));pointer(0x70,vertices.data());pointer(0x80,runtimeTriangles.data());
  pointer(0x90,nodeBytes+96);word(0x94,internalCount);pointer(0xa0,pairs.data()+2);word(0xa4,leafCount);
  builder(object.data());uint32_t nativeNodes=0,nativeLeaves=0;std::memcpy(&nativeNodes,object.data()+0x98,4);std::memcpy(&nativeLeaves,object.data()+0xa8,4);if(nativeNodes!=internalCount||nativeLeaves!=leafCount)throw std::runtime_error("native vector counts");
  bounds(object.data(),0,result.tightBounds.data(),result.tightBounds.data()+3);
  finishOriginalBounds(object.data(),nullptr,nullptr);
  for(size_t a=0;a<3;++a){std::memcpy(&result.expandedBounds[a],object.data()+4+4*a,4);std::memcpy(&result.expandedBounds[3+a],object.data()+0x14+4*a,4);}
  for(size_t i=0;i<96;++i)if(nodeBytes[i]!=0x5a||nodeBytes[size_t(internalCount+1)*96+i]!=0x5a)throw std::runtime_error("native node guard");
  for(size_t i:{size_t(0),size_t(1),size_t(leafCount+2),size_t(leafCount+3)})if(pairs[i]!=std::array<uint16_t,2>{0xa55a,0xa55a})throw std::runtime_error("native pair guard");
  result.leafPairs.assign(pairs.begin()+2,pairs.end()-2);result.nodes.resize(internalCount);std::memcpy(result.nodes.data(),nodeBytes+96,size_t(internalCount)*96);
#else
  result=sourceDisplacementCollisionTreeCandidate(power,vertices,triangles);
#endif
  out.write(reinterpret_cast<const char*>(&internalCount),4);out.write(reinterpret_cast<const char*>(&leafCount),4);
  out.write(reinterpret_cast<const char*>(result.leafPairs.data()),std::streamsize(leafCount*4));
  for(const auto&node:result.nodes)out.write(reinterpret_cast<const char*>(node.bounds.data()),96);
  out.write(reinterpret_cast<const char*>(result.tightBounds.data()),24);out.write(reinterpret_cast<const char*>(result.expandedBounds.data()),24);++cases;
 }
 if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failure");
#ifdef SFM64_NATIVE_TREE_ORACLE
 for(const auto&f:treeFragments)if(std::memcmp(base+f.offset,f.bytes,f.size))throw std::runtime_error("native code changed");VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS collision tree grids="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

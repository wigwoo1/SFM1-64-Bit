#include "source_displacement_runtime_mesh_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#ifdef SFM64_NATIVE_RUNTIME_MESH_ORACLE
#define NOMINMAX
#include <windows.h>
#include <malloc.h>
#include <memory>
#include "native_displacement_tree_bytes.h"
#include "native_displacement_triangle_metadata_bytes.h"
#include "native_displacement_runtime_mesh_bytes.h"
#endif
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("input output required");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace files");
#ifdef SFM64_NATIVE_RUNTIME_MESH_ORACLE
 auto base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x420000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("native base unavailable");for(const auto&f:treeFragments)std::memcpy(base+f.offset,f.bytes,f.size);for(const auto&f:triangleMetadataFragments)std::memcpy(base+f.offset,f.bytes,f.size);for(const auto&f:runtimeMeshFragments)std::memcpy(base+f.offset,f.bytes,f.size);for(const auto&c:runtimeMeshConstants)std::memcpy(base+c.offset,c.bytes,c.size);
 DWORD old;for(size_t offset:{size_t(0x140000),size_t(0x150000),size_t(0x240000),size_t(0x2d0000)})if(!VirtualProtect(base+offset,0x10000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protection failed");FlushInstructionCache(GetCurrentProcess(),base,0x420000);
 using Initialize=bool(__thiscall*)(void*,void*);auto initialize=reinterpret_cast<Initialize>(base+0x15c6b0);using Bounds=void(__thiscall*)(void*,uint32_t,float*,float*);auto bounds=reinterpret_cast<Bounds>(base+0x159a40);
#endif
 std::array<uint32_t,3>header{};size_t cases=0;
 while(in.read(reinterpret_cast<char*>(header.data()),12)){
  const uint32_t power=header[0],contents=header[1],flags=header[2];if(power<1||power>4)throw std::runtime_error("fixture power");const uint32_t cells=1u<<power,vertexCount=(cells+1)*(cells+1),triangleCount=2*cells*cells,leafCount=cells*cells;[[maybe_unused]]const uint32_t nodeCount=(leafCount-1)/3;
  std::array<std::array<float,3>,4>corners{};std::vector<std::array<float,3>>positions(vertexCount);std::vector<float>alphas(vertexCount);std::vector<SourceDisplacementGridTriangleCandidate>sourceTriangles(triangleCount);static_assert(sizeof(SourceDisplacementGridTriangleCandidate)==8,"source triangle stride");
  if(!in.read(reinterpret_cast<char*>(corners.data()),48)||!in.read(reinterpret_cast<char*>(positions.data()),vertexCount*12)||!in.read(reinterpret_cast<char*>(alphas.data()),vertexCount*4)||!in.read(reinterpret_cast<char*>(sourceTriangles.data()),triangleCount*8))throw std::runtime_error("truncated fixture");
  SourceDisplacementRuntimeMeshCandidate result;
#ifdef SFM64_NATIVE_RUNTIME_MESH_ORACLE
  std::array<uint8_t,0x310>core{};std::vector<std::array<uint8_t,224>>coreVertices(vertexCount);std::array<uint8_t,0xf4>runtime{};
  auto word=[](auto&object,size_t offset,uint32_t value){std::memcpy(object.data()+offset,&value,4);};auto pointer=[&](auto&object,size_t offset,const void*p){word(object,offset,reinterpret_cast<uint32_t>(p));};
  word(core,12,power);word(core,0x2cc,contents);word(core,0x2c8,flags);std::memcpy(core.data()+24,corners.data(),48);pointer(core,0x2f8,coreVertices.data());pointer(core,0x2fc,sourceTriangles.data());
  for(size_t i=0;i<vertexCount;++i){std::memcpy(coreVertices[i].data()+40,positions[i].data(),12);std::memcpy(coreVertices[i].data()+0x8c,&alphas[i],4);}
  std::vector<std::array<float,3>>runtimeVertices(vertexCount+2);std::memset(runtimeVertices.data(),0x5a,runtimeVertices.size()*12);
  std::vector<SourceDisplacementCollisionTriangleCandidate>runtimeTriangles(triangleCount+2);std::memset(&runtimeTriangles.front(),0x5a,24);std::memset(&runtimeTriangles.back(),0x5a,24);
  std::vector<std::array<uint16_t,2>>pairs(leafCount+4);for(auto&p:pairs)p.fill(0xa55a);
  auto freeAligned=[](void*p){_aligned_free(p);};std::unique_ptr<void,decltype(freeAligned)>nodes(_aligned_malloc(size_t(nodeCount+2)*96,16),freeAligned);if(!nodes)throw std::runtime_error("aligned node storage");auto nodeBytes=static_cast<uint8_t*>(nodes.get());std::memset(nodeBytes,0x5a,size_t(nodeCount+2)*96);
  pointer(runtime,0x70,runtimeVertices.data()+1);word(runtime,0x74,vertexCount);pointer(runtime,0x80,runtimeTriangles.data()+1);word(runtime,0x84,triangleCount);pointer(runtime,0x90,nodeBytes+96);word(runtime,0x94,nodeCount);pointer(runtime,0xa0,pairs.data()+2);word(runtime,0xa4,leafCount);
  if(!initialize(runtime.data(),core.data()))throw std::runtime_error("native initializer rejected fixture");
  auto readWord=[&](size_t at){uint32_t value;std::memcpy(&value,runtime.data()+at,4);return value;};if(readWord(0x78)!=vertexCount||readWord(0x88)!=triangleCount||readWord(0x98)!=nodeCount||readWord(0xa8)!=leafCount)throw std::runtime_error("native array counts");
  result.power=readWord(0x28);result.contents=readWord(0x20);result.runtimeFlags=readWord(0x2c);std::memcpy(result.corners.data(),runtime.data()+0x30,48);std::memcpy(result.baseNormal.data(),runtime.data()+0x60,12);
  result.vertices.assign(runtimeVertices.begin()+1,runtimeVertices.end()-1);result.triangles.assign(runtimeTriangles.begin()+1,runtimeTriangles.end()-1);result.tree.leafPairs.assign(pairs.begin()+2,pairs.end()-2);result.tree.nodes.resize(nodeCount);std::memcpy(result.tree.nodes.data(),nodeBytes+96,size_t(nodeCount)*96);
  for(size_t a=0;a<3;++a){std::memcpy(&result.tree.expandedBounds[a],runtime.data()+4+4*a,4);std::memcpy(&result.tree.expandedBounds[3+a],runtime.data()+0x14+4*a,4);}bounds(runtime.data(),0,result.tree.tightBounds.data(),result.tree.tightBounds.data()+3);
  auto guard=[](const void*p,size_t count){auto b=static_cast<const uint8_t*>(p);for(size_t i=0;i<count;++i)if(b[i]!=0x5a)throw std::runtime_error("native array guard changed");};guard(runtimeVertices.front().data(),12);guard(runtimeVertices.back().data(),12);guard(&runtimeTriangles.front(),24);guard(&runtimeTriangles.back(),24);guard(nodeBytes,96);guard(nodeBytes+size_t(nodeCount+1)*96,96);for(size_t i:{size_t(0),size_t(1),size_t(leafCount+2),size_t(leafCount+3)})if(pairs[i]!=std::array<uint16_t,2>{0xa55a,0xa55a})throw std::runtime_error("native pair guard changed");
#else
  result=sourceDisplacementRuntimeMeshCandidate(power,contents,flags,corners,positions,alphas,sourceTriangles);
#endif
  std::array<uint32_t,3>resultHeader{result.power,result.contents,result.runtimeFlags};out.write(reinterpret_cast<const char*>(resultHeader.data()),12);out.write(reinterpret_cast<const char*>(result.corners.data()),48);out.write(reinterpret_cast<const char*>(result.baseNormal.data()),12);out.write(reinterpret_cast<const char*>(result.vertices.data()),vertexCount*12);out.write(reinterpret_cast<const char*>(result.triangles.data()),triangleCount*24);out.write(reinterpret_cast<const char*>(result.tree.leafPairs.data()),leafCount*4);for(const auto&node:result.tree.nodes)out.write(reinterpret_cast<const char*>(node.bounds.data()),96);out.write(reinterpret_cast<const char*>(result.tree.tightBounds.data()),24);out.write(reinterpret_cast<const char*>(result.tree.expandedBounds.data()),24);++cases;
 }
 if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failure");
#ifdef SFM64_NATIVE_RUNTIME_MESH_ORACLE
 for(const auto&f:treeFragments)if(std::memcmp(base+f.offset,f.bytes,f.size))throw std::runtime_error("tree code changed");for(const auto&f:triangleMetadataFragments)if(std::memcmp(base+f.offset,f.bytes,f.size))throw std::runtime_error("metadata code changed");for(const auto&f:runtimeMeshFragments)if(std::memcmp(base+f.offset,f.bytes,f.size))throw std::runtime_error("initializer code changed");VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS runtime meshes="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

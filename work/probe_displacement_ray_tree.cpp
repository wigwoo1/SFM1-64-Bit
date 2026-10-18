#include "source_displacement_ray_tree_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#ifdef SFM64_NATIVE_RAY_TREE_ORACLE
#define NOMINMAX
#include <windows.h>
#include <malloc.h>
#include <memory>
#include "native_displacement_tree_bytes.h"
#include "native_displacement_triangle_metadata_bytes.h"
#include "native_displacement_ray_triangle_bytes.h"
#include "native_displacement_ray_tree_bytes.h"
#endif
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("input output required");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace files");
#ifdef SFM64_NATIVE_RAY_TREE_ORACLE
 auto base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x420000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("native base unavailable");
 for(const auto&f:treeFragments)std::memcpy(base+f.offset,f.bytes,f.size);for(const auto&f:triangleMetadataFragments)std::memcpy(base+f.offset,f.bytes,f.size);for(const auto&f:rayTriangleFragments)std::memcpy(base+f.offset,f.bytes,f.size);for(const auto&f:rayTreeFragments)std::memcpy(base+f.offset,f.bytes,f.size);for(const auto&c:rayTriangleConstants)std::memcpy(base+c.offset,c.bytes,c.size);for(const auto&c:rayTreeConstants)std::memcpy(base+c.offset,c.bytes,c.size);
 const float high=std::numeric_limits<float>::max(),low=-high;const uint32_t epsilon=0x34000000;std::memcpy(base+0x31debc,&high,4);std::memcpy(base+0x33ef78,&low,4);std::memcpy(base+0x322ae8,&epsilon,4);
 DWORD old;for(size_t offset:{size_t(0x140000),size_t(0x150000),size_t(0x240000),size_t(0x2d0000)})if(!VirtualProtect(base+offset,0x10000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protection failed");FlushInstructionCache(GetCurrentProcess(),base,0x420000);
 using Builder=void(__thiscall*)(void*);auto builder=reinterpret_cast<Builder>(base+0x15b8e0);
 using Bounds=void(__thiscall*)(void*,uint32_t,float*,float*);auto bounds=reinterpret_cast<Bounds>(base+0x159a40);
 using Metadata=void(__thiscall*)(void*,void*);auto plane=reinterpret_cast<Metadata>(base+0x159170),extrema=reinterpret_cast<Metadata>(base+0x1592e0);
 using Trace=void(__thiscall*)(void*,const void*,const float*,uint32_t,void*,bool,void**);auto trace=reinterpret_cast<Trace>(base+0x15a130);
 using Wrapper=bool(__thiscall*)(void*,const void*,const float*,void*,bool);auto wrapper=reinterpret_cast<Wrapper>(base+0x15b520);
#endif
 uint32_t power=0;size_t grids=0,total=0;
 while(in.read(reinterpret_cast<char*>(&power),4)){
  if(power<1||power>4)throw std::runtime_error("fixture power");const uint32_t cells=1u<<power,leaves=cells*cells;[[maybe_unused]]const uint32_t internal=(leaves-1)/3;
  std::vector<std::array<float,3>>vertices(size_t(cells+1)*(cells+1));std::vector<std::array<uint16_t,3>>indices(size_t(leaves)*2);
  if(!in.read(reinterpret_cast<char*>(vertices.data()),std::streamsize(vertices.size()*12))||!in.read(reinterpret_cast<char*>(indices.data()),std::streamsize(indices.size()*6)))throw std::runtime_error("truncated grid");
  std::vector<SourceDisplacementCollisionTriangleCandidate>triangles(indices.size());SourceDisplacementCollisionTreeCandidate tree;
#ifdef SFM64_NATIVE_RAY_TREE_ORACLE
  std::array<uint8_t,0xf4>object{};auto word=[&](size_t at,uint32_t value){std::memcpy(object.data()+at,&value,4);};auto pointer=[&](size_t at,const void*p){word(at,reinterpret_cast<uint32_t>(p));};
  std::vector<std::array<uint16_t,2>>pairs(leaves);
  auto freeAligned=[](void*p){_aligned_free(p);};std::unique_ptr<void,decltype(freeAligned)>nodes(_aligned_malloc(size_t(internal)*96,16),freeAligned);if(!nodes)throw std::runtime_error("aligned nodes");std::memset(nodes.get(),0,size_t(internal)*96);
  word(0x28,power);pointer(0x70,vertices.data());pointer(0x80,triangles.data());pointer(0x90,nodes.get());word(0x94,internal);pointer(0xa0,pairs.data());word(0xa4,leaves);
  uint32_t vertexPointer=reinterpret_cast<uint32_t>(vertices.data());for(size_t i=0;i<indices.size();++i){triangles[i].packedVertices=indices[i];triangles[i].flags=uint16_t((i*113u)&0xffc0u);plane(&triangles[i],&vertexPointer);extrema(&triangles[i],&vertexPointer);}
  builder(object.data());std::array<float,6>tight{};bounds(object.data(),0,tight.data(),tight.data()+3);
#else
  for(size_t i=0;i<indices.size();++i){SourceDisplacementCollisionTriangleCandidate initial;initial.packedVertices=indices[i];initial.flags=uint16_t((i*113u)&0xffc0u);triangles[i]=sourceDisplacementCollisionTriangleCandidate(vertices,initial);}
  tree=sourceDisplacementCollisionTreeCandidate(power,vertices,indices);
#endif
  uint32_t queryCount=0;if(!in.read(reinterpret_cast<char*>(&queryCount),4)||queryCount>65536)throw std::runtime_error("fixture query count");
  for(uint32_t q=0;q<queryCount;++q){
   std::array<uint32_t,14>query{};if(!in.read(reinterpret_cast<char*>(query.data()),56))throw std::runtime_error("truncated query");std::array<float,3>start{},delta{},inverse{};std::memcpy(start.data(),query.data(),12);std::memcpy(delta.data(),query.data()+3,12);std::memcpy(inverse.data(),query.data()+6,12);float prior=0;std::memcpy(&prior,query.data()+9,4);const uint32_t one=query[10],priorTriangle=query[11],runtimeFlags=query[12],contents=query[13];if(one>1)throw std::runtime_error("fixture one-sided flag");SourceDisplacementRayTreeHitCandidate result{prior,priorTriangle};
   std::array<uint8_t,84>wrapperTrace;wrapperTrace.fill(0xcd);const std::array<float,4>oldPlane{99,98,97,96};const uint16_t oldFlags=0xbeef;std::memcpy(wrapperTrace.data()+0x18,oldPlane.data(),16);std::memcpy(wrapperTrace.data()+0x2c,&prior,4);std::memcpy(wrapperTrace.data()+0x34,&oldFlags,2);uint32_t wrapperHit=0;
#ifdef SFM64_NATIVE_RAY_TREE_ORACLE
   std::array<uint8_t,80>ray{};std::memcpy(ray.data(),start.data(),12);std::memcpy(ray.data()+16,delta.data(),12);ray[68]=ray[69]=1;
   std::array<uint8_t,84>hit{};std::memcpy(hit.data()+0x2c,&prior,4);void*selected=priorTriangle==UINT32_MAX?nullptr:&triangles.at(priorTriangle);
   trace(object.data(),ray.data(),inverse.data(),0,hit.data(),one!=0,&selected);std::memcpy(&result.fraction,hit.data()+0x2c,4);result.triangle=selected?uint32_t(static_cast<SourceDisplacementCollisionTriangleCandidate*>(selected)-triangles.data()):UINT32_MAX;
   word(0x2c,runtimeFlags);word(0x20,contents);wrapperHit=wrapper(object.data(),ray.data(),inverse.data(),wrapperTrace.data(),one!=0)?1u:0u;
#else
   result=sourceDisplacementRayTreeCandidate(vertices,triangles,tree,start,delta,inverse,one!=0,result);
   SourceDisplacementRayTraceCandidate priorTrace;priorTrace.fraction=prior;wrapperHit=sourceDisplacementRayTraceCandidate(vertices,triangles,tree,runtimeFlags,contents,start,delta,inverse,one!=0,priorTrace)?1u:0u;std::memcpy(wrapperTrace.data()+0x18,priorTrace.normal.data(),12);std::memcpy(wrapperTrace.data()+0x24,&priorTrace.distance,4);std::memcpy(wrapperTrace.data()+0x2c,&priorTrace.fraction,4);std::memcpy(wrapperTrace.data()+0x34,&priorTrace.displacementFlags,2);
#endif
   out.write(reinterpret_cast<const char*>(&result.fraction),4);out.write(reinterpret_cast<const char*>(&result.triangle),4);
   std::array<float,4>planeOutput{99,98,97,96};if(result.triangle!=UINT32_MAX){const auto&t=triangles.at(result.triangle);std::copy(t.normal.begin(),t.normal.end(),planeOutput.begin());planeOutput[3]=t.distance;}out.write(reinterpret_cast<const char*>(planeOutput.data()),16);out.write(reinterpret_cast<const char*>(&wrapperHit),4);out.write(reinterpret_cast<const char*>(wrapperTrace.data()),84);++total;
  }++grids;
 }
 if(!in.eof()||in.gcount()!=0||!grids)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failure");
#ifdef SFM64_NATIVE_RAY_TREE_ORACLE
 for(const auto&f:treeFragments)if(std::memcmp(base+f.offset,f.bytes,f.size))throw std::runtime_error("tree code changed");for(const auto&f:triangleMetadataFragments)if(std::memcmp(base+f.offset,f.bytes,f.size))throw std::runtime_error("metadata code changed");for(const auto&f:rayTriangleFragments)if(std::memcmp(base+f.offset,f.bytes,f.size))throw std::runtime_error("ray code changed");for(const auto&f:rayTreeFragments)if(std::memcmp(base+f.offset,f.bytes,f.size))throw std::runtime_error("walker code changed");VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS ray tree grids="<<grids<<" queries="<<total<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

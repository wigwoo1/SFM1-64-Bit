#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef near
#undef far
#undef min
#undef max
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
#include "source_prepared_world_trace_candidate.h"

void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(int argc,char** argv){try{
 require(argc==2,"expected disposable benchmark BSP path");
 std::ifstream file(argv[1],std::ios::binary);require(bool(file),"BSP open");
 std::vector<uint8_t> bsp{std::istreambuf_iterator<char>(file),{}};
 auto word=[&](size_t offset){uint32_t value;require(offset<=bsp.size()&&bsp.size()-offset>=4,"BSP header span");std::memcpy(&value,bsp.data()+offset,4);return value;};
 const uint32_t texinfo=word(8+6*16),texinfoBytes=word(8+6*16+4),texdataBytes=word(8+2*16+4);
 require(texinfoBytes%72==0&&texdataBytes%32==0,"BSP material spans");
 std::vector<uint32_t> surfaceIds;std::vector<uint16_t> materialFlags(texdataBytes/32);
 for(uint32_t i=0;i<texinfoBytes/72;++i){const auto material=word(texinfo+72*i+68);require(material<materialFlags.size(),"material ID");surfaceIds.push_back(material);materialFlags[material]|=uint16_t(word(texinfo+72*i+64));}
 SourceBspBrushOnlyTraceCandidate world(bsp,surfaceIds);
 const std::vector<SourceDisplacementRuntimeMeshCandidate> meshes;
 const SourcePreparedHullWorldTablesCandidate emptyTables;
 const std::vector<std::array<uint16_t,2>> properties;
 unsigned calls=0;
 auto excludes=[&](uint32_t)->bool{++calls;throw std::runtime_error("empty world invoked surface callback");};
 auto details=[&](const SourceCollisionSurfaceCandidate&)->std::array<uint16_t,2>{++calls;throw std::runtime_error("empty world invoked detail callback");};
 auto identity=[&](const SourceCollisionSurfaceCandidate&)->uint32_t{++calls;throw std::runtime_error("empty world invoked identity callback");};
 unsigned cases=0;
 for(bool swept:{false,true})for(bool isRay:{false,true})for(bool compute:{false,true}){
  SourceServerHullRayCandidate ray;ray.swept=swept;ray.isRay=isRay;
  ray.center={1.f,2.f,3.f};ray.delta={4.f,5.f,6.f};ray.offset={-1.f,-2.f,-3.f};ray.extents={.25f,.5f,.75f};
  auto result=sourcePreparedWorldTraceCandidate(world,meshes,emptyTables,properties,ray,0xffffffffu,
   {0x3344,0x55aa},compute,false,excludes,details,identity);
  const auto& trace=result.state.trace;
  require(trace.fraction==1.f&&trace.leftSolid==0.f&&!trace.startSolid&&!trace.allSolid&&trace.contents==0,"default trace state");
  require(trace.plane==std::array<float,4>{},"default trace plane");
  require(trace.surface.kind==SourceCollisionSurfaceKindCandidate::Default&&trace.surface.index==0,"default trace surface");
  require(trace.planeTags==std::array<uint8_t,4>{},"default plane tags");
  require(result.state.displacementFlags==0&&result.state.displacementHit==0,"default displacement state");
  require(result.state.surfaceProperty==0x3344&&result.state.surfaceFlags==0x55aa,"default surface properties");
  require(!result.state.worldSurfaceIndexSource&&result.worldSurfaceIndex==0,"default world surface");
  require(result.startPosition==std::array<float,3>{}&&result.endPosition==std::array<float,3>{},"default positions");
  ++cases;
 }
 require(cases==8&&calls==0,"all eight empty-table combinations");
 std::cout<<"{\"cases\":"<<cases<<",\"unpopulated_tables\":true,\"callbacks\":"<<calls<<",\"passed\":true}\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}

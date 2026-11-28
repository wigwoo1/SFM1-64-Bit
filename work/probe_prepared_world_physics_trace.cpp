#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "source_prepared_world_physics_trace_candidate.h"

void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
volatile bool failName=true;
float fromBits(uint32_t bits){float value;std::memcpy(&value,&bits,4);return value;}
template<class T>void put(std::array<uint8_t,92>& bytes,size_t at,const T& value){std::memcpy(bytes.data()+at,&value,sizeof(value));}
std::array<uint8_t,92> pack(const SourcePhysicsSearchTraceCandidate& trace){
 std::array<uint8_t,92> bytes{};
 std::memcpy(bytes.data(),trace.start.data(),12);std::memcpy(bytes.data()+12,trace.end.data(),12);
 std::memcpy(bytes.data()+24,trace.plane.normal.data(),12);put(bytes,36,trace.plane.distance);
 std::memcpy(bytes.data()+40,trace.planeTags.data(),4);put(bytes,44,trace.fraction);put(bytes,48,trace.contents);
 put(bytes,52,trace.displacementFlags);put(bytes,54,trace.allSolid);put(bytes,55,trace.startSolid);
 put(bytes,56,trace.fractionLeftSolid);put(bytes,60,trace.surfaceId);put(bytes,68,trace.surfaceMetadata);
 put(bytes,72,trace.hitGroup);put(bytes,76,trace.physicsBone);put(bytes,78,trace.worldSurface);
 put(bytes,80,trace.entityId);put(bytes,88,trace.hitBox);return bytes;
}
int main(int argc,char** argv){try{
 require(argc==2,"expected disposable output path");std::ofstream records(argv[1],std::ios::binary);require(bool(records),"output open");
 uint32_t count=0,highNames=0,defaultNames=0,thrown=0;
 const std::array<uint32_t,8> patterns{0,0x80000000u,0x3f800000u,0xbf800000u,0x7f7fffffu,0x00800000u,0x00000001u,0x7fc12345u};
 for(uint32_t seed=0;seed<patterns.size();++seed)for(uint32_t kind=0;kind<5;++kind)
 for(uint32_t solids=0;solids<4;++solids)for(uint32_t variant=0;variant<4;++variant){
  SourceSweptHullWorldTraceCandidate value;std::array<uint8_t,92> expected{};
  std::array<float,12> fields{};for(size_t n=0;n<fields.size();++n)fields[n]=fromBits(patterns[(seed+n)%patterns.size()]);
  for(size_t n=0;n<3;++n){value.startPosition[n]=fields[n];value.endPosition[n]=fields[n+3];}
  for(size_t n=0;n<4;++n)value.state.trace.plane[n]=fields[n+6];
  value.state.trace.fraction=fields[10];value.state.trace.leftSolid=fields[11];
  value.state.trace.planeTags={uint8_t(seed),0x80,uint8_t(kind),uint8_t(0xffu-variant)};
  value.state.trace.contents=0xa5c30000u|(seed<<8)|kind;
  value.state.trace.surface={SourceCollisionSurfaceKindCandidate(kind),variant*193u+seed};
  value.state.trace.allSolid=(solids&1)!=0;value.state.trace.startSolid=(solids&2)!=0;
  value.state.surfaceProperty=uint16_t(variant==0?0:variant==1?0xffff:0x8000u+seed);
  value.state.surfaceFlags=uint16_t(variant==0?0xffff:variant==1?0:0x3333u+kind);
  value.state.displacementFlags=uint16_t(0xf000u+variant);value.state.displacementHit=0x5a5a5a5a;
  value.worldSurfaceIndex=uint16_t(variant==3?0xffff:variant*256u+seed);
  value.state.worldSurfaceIndexSource=SourceCollisionSurfaceCandidate{SourceCollisionSurfaceKindCandidate::RuntimeSurface,0xffffffffu};
  uint32_t calls=0;const uint64_t name=(kind==0&&variant==0)?0:0x1234567800000000ull|(uint64_t(kind)<<32)|value.state.trace.surface.index;
  auto result=sourcePreparedWorldPhysicsTraceCandidate(value,[&](const auto& surface)->uint64_t{
   require(surface.kind==SourceCollisionSurfaceKindCandidate(kind)&&surface.index==value.state.trace.surface.index,"name resolver input");++calls;return name;
  });
  require(calls==1,"every name kind must invoke the explicit resolver");
  // Independent wire construction uses the original scalar field sequence;
  // it does not call the bridge or inspect its output to derive expectations.
  std::memcpy(expected.data(),fields.data(),40);std::memcpy(expected.data()+40,value.state.trace.planeTags.data(),4);
  put(expected,44,fields[10]);put(expected,48,value.state.trace.contents);put(expected,52,value.state.displacementFlags);
  const uint8_t all=uint8_t(solids&1),inside=uint8_t((solids>>1)&1);put(expected,54,all);put(expected,55,inside);
  put(expected,56,fields[11]);put(expected,60,name);
  expected[68]=uint8_t(value.state.surfaceProperty);expected[69]=uint8_t(value.state.surfaceProperty>>8);
  expected[70]=uint8_t(value.state.surfaceFlags);expected[71]=uint8_t(value.state.surfaceFlags>>8);
  put(expected,78,value.worldSurfaceIndex);
  const auto bytes=pack(result);require(bytes==expected,"field mapping differs from independent wire bytes");
  records.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());require(bool(records),"record write");
  if(name)++highNames;else ++defaultNames;
  try{sourcePreparedWorldPhysicsTraceCandidate(value,[](const auto&)->uint64_t{if(failName)throw std::runtime_error("resolver failure");return 0;});}catch(const std::runtime_error&){++thrown;}
  // Vary only the private marker; public output must be byte-identical.
  value.state.displacementHit=0;value.state.worldSurfaceIndexSource.reset();
  auto again=sourcePreparedWorldPhysicsTraceCandidate(value,[&](const auto&)->uint64_t{return name;});
  require(pack(again)==bytes,"internal marker leaked to public trace");++count;
 }
 require(thrown==count,"name exception coverage");
 std::cout<<"{\"cases\":"<<count<<",\"high_names\":"<<highNames<<",\"default_names\":"<<defaultNames<<",\"resolver_exceptions\":"<<thrown<<",\"passed\":true}\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

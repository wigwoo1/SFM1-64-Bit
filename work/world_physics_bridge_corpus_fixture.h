#pragma once
#include "source_prepared_world_trace_candidate.h"
#include "source_prepared_world_physics_trace_candidate.h"
#include "physics_search_test_fixture.h"
#include <cstdlib>
#include <fstream>

// Test-only surface identities. All displacements share the same semantic
// name: original native10350ee0 is a shared **displacement** name, while the
// accepted corpus's per-mesh0x20000000tags identify which fixture mesh hit.
// Brush names distinguish the bound material IDs. No production name pointer
// or material-property resolver is inferred from these fixture values.
inline std::ofstream worldPhysicsCorpusSide;
inline uint64_t worldPhysicsCorpusCalls=0;
inline std::array<uint64_t,3> worldPhysicsCorpusNames{};
// Preserve the accepted main's implicit success return. The MSVC command-line
// globals are available when its first owned query reaches this Windows-only
// test fixture; no accepted oracle/main body has to be copied or rewritten.
inline void worldPhysicsCorpusFinish(){
  if(!worldPhysicsCorpusSide.is_open())return;
  worldPhysicsCorpusSide.flush();
  std::ofstream counts(std::string(__argv[__argc-1])+".physics-counts.json");
  counts<<"{\"calls\":"<<worldPhysicsCorpusCalls<<",\"default\":"<<worldPhysicsCorpusNames[0]
   <<",\"terrain\":"<<worldPhysicsCorpusNames[1]<<",\"brush\":"<<worldPhysicsCorpusNames[2]<<"}\n";
  counts.flush();
  if(!counts||!worldPhysicsCorpusSide)std::abort();
}
template<class SurfaceExcludes,class SurfaceDetails,class SurfaceIdentity>
SourceSweptHullWorldTraceCandidate worldPhysicsCorpusTrace(
 const SourceBspBrushOnlyTraceCandidate& world,
 const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,
 const SourcePreparedHullWorldTablesCandidate& tables,
 const std::vector<std::array<uint16_t,2>>& properties,
 const SourceServerHullRayCandidate& ray,uint32_t mask,
 const std::array<uint16_t,2>& defaultSurface,bool computePositions,bool worldAvailable,
 SurfaceExcludes surfaceExcludes,SurfaceDetails surfaceDetails,SurfaceIdentity surfaceIdentity){
 if(!worldPhysicsCorpusSide.is_open()){
  if(__argc<2)throw std::runtime_error("corpus output path missing");
  worldPhysicsCorpusSide.open(std::string(__argv[__argc-1])+".physics",std::ios::binary);
  if(!worldPhysicsCorpusSide)throw std::runtime_error("typed corpus side stream");
  if(std::atexit(worldPhysicsCorpusFinish)!=0)throw std::runtime_error("typed corpus finish registration");
 }
 const auto publicTrace=sourcePreparedWorldTraceCandidate(world,meshes,tables,properties,ray,mask,defaultSurface,
  computePositions,worldAvailable,surfaceExcludes,surfaceDetails,surfaceIdentity);
 unsigned names=0;
 const auto converted=sourcePreparedWorldPhysicsTraceCandidate(publicTrace,[&](const SourceCollisionSurfaceCandidate& surface)->uint64_t{
  ++names;
  if(surface.kind==SourceCollisionSurfaceKindCandidate::Default){++worldPhysicsCorpusNames[0];return 0;}
  if(surface.kind==SourceCollisionSurfaceKindCandidate::Displacement){++worldPhysicsCorpusNames[1];return 0x2222000000000000ull;}
  const auto identity=surfaceIdentity(surface);
  if(identity==UINT32_MAX||identity>=65535)throw std::runtime_error("invalid selected brush name fixture");
  ++worldPhysicsCorpusNames[2];return 0x1111000000000000ull+identity;
 });
 if(names!=1)throw std::runtime_error("name resolver must run once per converted trace");
 searchWriteTrace(worldPhysicsCorpusSide,converted);
 if(!worldPhysicsCorpusSide)throw std::runtime_error("typed corpus side output");
 ++worldPhysicsCorpusCalls;
 return publicTrace;
}

#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef near
#undef far
#include "source_engine_hull_trace_candidate.h"
#include "source_prepared_world_trace_candidate.h"
#include "source_prepared_world_physics_trace_candidate.h"
#include "source_server_trace_annotation_candidate.h"
#include "physics_search_test_fixture.h"
#include <cstdlib>
#include <fstream>
inline constexpr uint64_t worldEngineEntityId=0x3333000000000001ull;
inline std::ofstream worldEngineOwned,worldEngineNative,worldEngineRays;
inline uint64_t worldEngineOwnedCalls=0,worldEngineNativeCalls=0;
inline void worldEngineFinish(){
 worldEngineOwned.flush();worldEngineNative.flush();worldEngineRays.flush();
 std::ofstream count(std::string(__argv[__argc-1])+".engine-counts.json");
 count<<"{\"owned\":"<<worldEngineOwnedCalls<<",\"native\":"<<worldEngineNativeCalls<<"}\n";count.flush();
 if(!count||!worldEngineOwned||!worldEngineNative||!worldEngineRays)std::abort();
}
inline void worldEngineStreams(){
 if(worldEngineOwned.is_open())return;
 if(__argc<2)throw std::runtime_error("engine corpus output argument");const std::string base(__argv[__argc-1]);
 worldEngineOwned.open(base+".owned-physics",std::ios::binary);worldEngineNative.open(base+".native-physics",std::ios::binary);worldEngineRays.open(base+".rays",std::ios::binary);
 if(!worldEngineOwned||!worldEngineNative||!worldEngineRays||std::atexit(worldEngineFinish)!=0)throw std::runtime_error("engine corpus output streams");
}
inline std::array<uint8_t,80> worldEngineRayBytes(const SourceServerHullRayCandidate& ray){
 std::array<uint8_t,80> bytes{};std::memcpy(bytes.data(),ray.center.data(),12);std::memcpy(bytes.data()+16,ray.delta.data(),12);
 std::memcpy(bytes.data()+32,ray.offset.data(),12);std::memcpy(bytes.data()+48,ray.extents.data(),12);bytes[68]=uint8_t(ray.isRay);bytes[69]=uint8_t(ray.swept);return bytes;
}
struct WorldEngineAnnotationFixture {
 uintptr_t collideableEntity(uintptr_t){
#ifdef _WIN64
  return uintptr_t(worldEngineEntityId);
#else
  return 1;
#endif
 }
 uint32_t referenceHandle(uintptr_t){return 0;}
};
#ifndef _WIN64
inline uint32_t worldEngineProfile=0,worldEnginePartitionCalls=0;
inline std::array<uintptr_t,28> worldEngineTable{};inline std::array<uintptr_t,2> worldEngineObject{};
inline std::array<uintptr_t,15> worldEnginePartitionTable{};
inline std::array<uintptr_t,1> worldEngineCollideableTable{};
inline std::array<uintptr_t,3> worldEngineEntityTable{};
inline uintptr_t worldEngineCollideableVtable=0,worldEngineEntityVtable=0;
inline uint32_t worldEngineReference=0;
inline uintptr_t __fastcall worldEngineGetWorld(void*,void*){return uintptr_t(&worldEngineCollideableVtable);}
inline uintptr_t __fastcall worldEngineGetEntity(void*,void*){return uintptr_t(&worldEngineEntityVtable);}
inline const uint32_t* __fastcall worldEngineGetHandle(void*,void*){return &worldEngineReference;}
inline uint32_t __fastcall worldEnginePartitionMask(void*,void*){return 1;}
inline uint32_t __fastcall worldEngineTraceType(void*,void*){return 0;}
inline bool __fastcall worldEnginePredicate(void*,void*,void*,uint32_t){throw std::runtime_error("empty partition invoked predicate");}
inline void __fastcall worldEngineEnumerate(void*,void*,uint32_t mask,const void*,bool coarse,void*){
 if(mask!=1||coarse)throw std::runtime_error("engine world partition arguments");++worldEnginePartitionCalls;
}
inline void worldEngineNativeCall(const void* ray,void* output,uint32_t mask){
 *reinterpret_cast<uint32_t*>(0x10643ee8)|=1;*reinterpret_cast<uintptr_t*>(0x10643ee4)=uintptr_t(&worldEngineProfile);
 worldEnginePartitionTable[14]=uintptr_t(&worldEngineEnumerate);*reinterpret_cast<uintptr_t*>(0x106683c0)=uintptr_t(worldEnginePartitionTable.data());
 worldEngineTable[25]=uintptr_t(&worldEngineGetWorld);worldEngineTable[26]=0x1016d840;worldEngineTable[27]=uintptr_t(&worldEnginePartitionMask);
 worldEngineObject[0]=uintptr_t(worldEngineTable.data());
 worldEngineCollideableTable[0]=uintptr_t(&worldEngineGetEntity);worldEngineEntityTable[2]=uintptr_t(&worldEngineGetHandle);
 worldEngineCollideableVtable=uintptr_t(worldEngineCollideableTable.data());worldEngineEntityVtable=uintptr_t(worldEngineEntityTable.data());
 worldEnginePartitionTable[14]=uintptr_t(&worldEngineEnumerate);
 *reinterpret_cast<uintptr_t*>(0x106683c0)=uintptr_t(worldEnginePartitionTable.data());
 *reinterpret_cast<uint32_t*>(0x10643ee8)|=1;*reinterpret_cast<uintptr_t*>(0x10643ee4)=uintptr_t(&worldEngineProfile);
 const std::array<uintptr_t,2> filterTable{uintptr_t(&worldEnginePredicate),uintptr_t(&worldEngineTraceType)};uintptr_t filter=uintptr_t(filterTable.data());
 reinterpret_cast<void(__thiscall*)(void*,const void*,uint32_t,const void*,void*)>(0x1016ece0)(worldEngineObject.data(),ray,mask,&filter,output);
 if(worldEngineProfile!=worldEngineNativeCalls+1||worldEngineObject[1]!=worldEngineNativeCalls+1)throw std::runtime_error("real engine world profiling calls");
}
inline uint64_t worldEngineNormalizeEntity(uint32_t value){
 if(value==0)return 0;if(value!=uintptr_t(&worldEngineEntityVtable))throw std::runtime_error("engine world annotation identity");return worldEngineEntityId;
}
inline void worldEngineNativeRecord(const std::array<uint8_t,88>& row){
 auto read=[&](size_t offset,auto& value){std::memcpy(&value,row.data()+offset,sizeof(value));};
 SourcePhysicsSearchTraceCandidate trace;std::memcpy(trace.start.data(),row.data(),12);std::memcpy(trace.end.data(),row.data()+12,12);std::memcpy(trace.plane.normal.data(),row.data()+24,12);
 read(36,trace.plane.distance);std::memcpy(trace.planeTags.data(),row.data()+40,4);read(44,trace.fraction);read(48,trace.contents);read(52,trace.displacementFlags);trace.allSolid=row[54];trace.startSolid=row[55];read(56,trace.fractionLeftSolid);
 uint32_t name,metadata,entity;read(60,name);read(64,metadata);read(76,entity);
 if(name==UINT32_MAX)trace.surfaceId=0;else if(name>=0x20000000u&&name<0x20010000u)trace.surfaceId=0x2222000000000000ull;
 else if(name>=0x10000000u&&name<0x10010000u)trace.surfaceId=0x1111000000000000ull+(name-0x10000000u);else throw std::runtime_error("engine native surface identity");
 trace.surfaceMetadata=metadata;read(68,trace.hitGroup);read(72,trace.physicsBone);read(74,trace.worldSurface);trace.entityId=worldEngineNormalizeEntity(entity);read(80,trace.hitBox);
 searchWriteTrace(worldEngineNative,trace);++worldEngineNativeCalls;
}
#endif
template<class SurfaceExcludes,class SurfaceDetails,class SurfaceIdentity>
SourceSweptHullWorldTraceCandidate worldEngineOwnedTrace(const SourceBspBrushOnlyTraceCandidate& world,
 const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,const SourcePreparedHullWorldTablesCandidate& tables,
 const std::vector<std::array<uint16_t,2>>& properties,const SourceServerHullRayCandidate& ray,uint32_t mask,
 const std::array<uint16_t,2>& defaultSurface,bool compute,bool available,SurfaceExcludes excludes,SurfaceDetails details,SurfaceIdentity identities){
 if(!compute||ray.isRay||!ray.swept||world.root!=0)throw std::runtime_error("bounded real world engine corpus domain");worldEngineStreams();
 SourceSweptHullWorldTraceCandidate publicValue;
 struct Host{
  const SourceBspBrushOnlyTraceCandidate& world;const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes;
  const SourcePreparedHullWorldTablesCandidate& tables;const std::vector<std::array<uint16_t,2>>& properties;
  const std::array<uint16_t,2>& surface;bool available;SurfaceExcludes& excludes;SurfaceDetails& details;SurfaceIdentity& identities;SourceSweptHullWorldTraceCandidate& publicValue;
  SourcePhysicsSearchTraceCandidate traceWorld(const SourceServerHullRayCandidate& r,uint32_t m){
   publicValue=sourcePreparedWorldTraceCandidate(world,meshes,tables,properties,r,m,surface,true,available,excludes,details,identities);
   auto result=sourcePreparedWorldPhysicsTraceCandidate(publicValue,[&](const SourceCollisionSurfaceCandidate& s)->uint64_t{
    if(s.kind==SourceCollisionSurfaceKindCandidate::Default)return 0;if(s.kind==SourceCollisionSurfaceKindCandidate::Displacement)return 0x2222000000000000ull;
    const auto id=identities(s);if(id==UINT32_MAX||id>=65535)throw std::runtime_error("engine brush name binding");return 0x1111000000000000ull+id;
   });WorldEngineAnnotationFixture annotate;sourceServerTraceAnnotationCandidate(1,result,{0,0},annotate);
#ifndef _WIN64
   if(result.entityId==1)result.entityId=worldEngineEntityId;
#endif
   return result;
  }
  std::vector<uintptr_t> enumerateCandidates(const SourceServerHullRayCandidate&){return {};}
  void candidateOverflow(size_t){throw std::runtime_error("empty partition overflow");}
  uintptr_t resolveCollideable(uintptr_t){throw std::runtime_error("empty partition resolve");}
  uint32_t referenceHandle(uintptr_t){throw std::runtime_error("empty partition reference");}
  SourcePhysicsSearchTraceCandidate clipToCollideable(const SourceServerHullRayCandidate&,uint32_t,uintptr_t){throw std::runtime_error("empty partition clip");}
 }host{world,meshes,tables,properties,defaultSurface,available,excludes,details,identities,publicValue};
 const auto trace=sourceEngineHullTraceCandidate(ray,mask,0,host,[](uintptr_t,uint32_t)->bool{throw std::runtime_error("empty partition filter");});
 searchWriteTrace(worldEngineOwned,trace);const auto bytes=worldEngineRayBytes(ray);supportPut(worldEngineRays,bytes);++worldEngineOwnedCalls;
 publicValue.startPosition=trace.start;publicValue.endPosition=trace.end;publicValue.state.trace.fraction=trace.fraction;publicValue.state.trace.leftSolid=trace.fractionLeftSolid;
 return publicValue;
}

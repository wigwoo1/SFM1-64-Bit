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
inline constexpr uint64_t worldMixedEntityId=0x3333000000000001ull;
inline std::ofstream worldMixedOwned,worldMixedNative,worldMixedRays;
inline uint64_t worldMixedOwnedCalls=0,worldMixedNativeCalls=0;
inline uint32_t worldMixedActiveOwnedMask=0;
inline unsigned worldMixedFixtureProfile=UINT32_MAX;
inline std::vector<std::array<uint32_t,2>> worldMixedOwnedEvents,worldMixedNativeEvents;
inline std::array<uint8_t,80> worldMixedOwnedEnumerated{},worldMixedNativeEnumerated{};
inline std::ofstream worldMixedOwnedEnumeration,worldMixedNativeEnumeration,worldMixedOwnedEventStream,worldMixedNativeEventStream;

inline void worldMixedFinish(){
 worldMixedOwned.flush();worldMixedNative.flush();worldMixedRays.flush();worldMixedOwnedEnumeration.flush();worldMixedNativeEnumeration.flush();worldMixedOwnedEventStream.flush();worldMixedNativeEventStream.flush();
 std::ofstream count(std::string(__argv[__argc-1])+".engine-counts.json");
 count<<"{\"owned\":"<<worldMixedOwnedCalls<<",\"native\":"<<worldMixedNativeCalls<<"}\n";count.flush();
 if(!count||!worldMixedOwned||!worldMixedNative||!worldMixedRays||!worldMixedOwnedEnumeration||!worldMixedNativeEnumeration||!worldMixedOwnedEventStream||!worldMixedNativeEventStream)std::abort();
}
inline void worldMixedStreams(){
 if(worldMixedOwned.is_open())return;
 if(__argc<2)throw std::runtime_error("engine corpus output argument");const std::string base(__argv[__argc-1]);
 worldMixedOwned.open(base+".owned-physics",std::ios::binary);worldMixedNative.open(base+".native-physics",std::ios::binary);worldMixedRays.open(base+".rays",std::ios::binary);
 worldMixedOwnedEnumeration.open(base+".owned-enumerated-rays",std::ios::binary);worldMixedNativeEnumeration.open(base+".native-enumerated-rays",std::ios::binary);worldMixedOwnedEventStream.open(base+".owned-events",std::ios::binary);worldMixedNativeEventStream.open(base+".native-events",std::ios::binary);
 if(!worldMixedOwned||!worldMixedNative||!worldMixedRays||!worldMixedOwnedEnumeration||!worldMixedNativeEnumeration||!worldMixedOwnedEventStream||!worldMixedNativeEventStream||std::atexit(worldMixedFinish)!=0)throw std::runtime_error("engine corpus output streams");
}
inline std::array<uint8_t,80> worldMixedRayBytes(const SourceServerHullRayCandidate& ray){
 std::array<uint8_t,80> bytes{};std::memcpy(bytes.data(),ray.center.data(),12);std::memcpy(bytes.data()+16,ray.delta.data(),12);
 std::memcpy(bytes.data()+32,ray.offset.data(),12);std::memcpy(bytes.data()+48,ray.extents.data(),12);bytes[68]=uint8_t(ray.isRay);bytes[69]=uint8_t(ray.swept);return bytes;
}
struct WorldMixedAnnotationFixture {
 uintptr_t collideableEntity(uintptr_t){
#ifdef _WIN64
  return uintptr_t(worldMixedEntityId);
#else
  return 1;
#endif
 }
 uint32_t referenceHandle(uintptr_t){return 0;}
};

inline unsigned worldMixedReadProfile(){
 if(worldMixedFixtureProfile!=UINT32_MAX)return worldMixedFixtureProfile;
 char value[3]{};const auto count=GetEnvironmentVariableA("SFM64_MIXED_ENTITY_PROFILE",value,3);
 if(count!=1||value[0]<'0'||value[0]>'2')throw std::runtime_error("mixed entity profile must be0/1/2");
 return worldMixedFixtureProfile=unsigned(value[0]-'0');
}
inline unsigned worldMixedCandidateCount(){return worldMixedReadProfile()==0?2:worldMixedReadProfile()==1?3:4;}
inline bool worldMixedStatic(unsigned id){return worldMixedReadProfile()==0?id==1:id==2;}
inline bool worldMixedAllowed(unsigned id){return worldMixedReadProfile()==0?id!=1:worldMixedReadProfile()==1?id==3:true;}
inline uint32_t worldMixedHandle(unsigned id){return (worldMixedStatic(id)?0x40000000u:0x40010000u)+id;}
inline uintptr_t worldMixedIdentity(unsigned id){
#ifdef _WIN64
 return uintptr_t(0x4444000000000000ull+id);
#else
 return uintptr_t(0x6000u+id);
#endif
}
inline unsigned worldMixedLabel(uintptr_t entity){for(unsigned id=1;id<=worldMixedCandidateCount();++id)if(entity==worldMixedIdentity(id))return id;throw std::runtime_error("owned mixed candidate identity");}
inline SourcePhysicsSearchTraceCandidate worldMixedCandidate(unsigned id){
 if(!id||id>worldMixedCandidateCount())throw std::runtime_error("mixed candidate index");
 const unsigned profile=worldMixedReadProfile();SourcePhysicsSearchTraceCandidate trace;
 trace.start={101.f+id,201.f+id,301.f+id};trace.end={401.f+id,501.f+id,601.f+id};
 trace.plane.normal={.125f*id,-.25f*id,.5f};trace.plane.distance=700.f+id;
 trace.planeTags={uint8_t(1+id),uint8_t(2+id),uint8_t(0x30+id),uint8_t(0x40+id)};
 trace.fraction=profile==0?(id==1?.5f:1.f):profile==1?(id==1?.125f:id==2?1.f:.25f):(id==1?.75f:id==2?.5f:0.f);
 trace.contents=0x80000000u|id;trace.displacementFlags=uint16_t(0x100+id);
 trace.allSolid=uint8_t(profile==2&&id==3);trace.startSolid=uint8_t(profile==2&&(id==1||id==3));
 trace.fractionLeftSolid=profile==2?(id==1?.5f:id==2?.25f:id==3?1.f:0.f):0.f;
 trace.surfaceId=0x1111000000000000ull+10+id;trace.surfaceMetadata=(0x500u+id)|((0x600u+id)<<16);
 trace.hitGroup=0x7000u+id;trace.physicsBone=uint16_t(0x80+id);trace.worldSurface=uint16_t(0x20+id);
 trace.entityId=0x4444000000000000ull+id;trace.hitBox=0x9000u+id;return trace;
}
inline void worldMixedWriteEvents(std::ostream& out,const std::vector<std::array<uint32_t,2>>& events){supportPut(out,uint32_t(events.size()));for(const auto& event:events)supportPut(out,event);}

#ifndef _WIN64
inline uint32_t worldMixedProfile=0,worldMixedPartitionCalls=0;
inline std::array<uintptr_t,28> worldMixedTable{};inline std::array<uintptr_t,2> worldMixedObject{};
inline std::array<uintptr_t,15> worldMixedPartitionTable{};
inline std::array<uintptr_t,1> worldMixedCollideableTable{};
inline std::array<uintptr_t,3> worldMixedEntityTable{};
inline uintptr_t worldMixedCollideableVtable=0,worldMixedEntityVtable=0;
inline uint32_t worldMixedReference=0;
inline uintptr_t __fastcall worldMixedGetWorld(void*,void*){return uintptr_t(&worldMixedCollideableVtable);}
inline uintptr_t __fastcall worldMixedGetEntity(void*,void*){return uintptr_t(&worldMixedEntityVtable);}
inline const uint32_t* __fastcall worldMixedGetHandle(void*,void*){return &worldMixedReference;}
inline uint32_t __fastcall worldMixedPartitionMask(void*,void*){return 1;}
inline uint32_t __fastcall worldMixedTraceType(void*,void*){return 0;}

inline uint32_t worldMixedActiveMask=0;
inline std::array<std::array<uintptr_t,2>,4> worldMixedEntities{};inline std::array<uintptr_t,3> worldMixedCandidateTable{};
inline unsigned worldMixedNativeLabel(const void* entity){for(unsigned id=1;id<=worldMixedCandidateCount();++id)if(entity==worldMixedEntities[id-1].data())return id;throw std::runtime_error("native mixed entity identity");}
inline const uint32_t* __fastcall worldMixedGetCandidateHandle(void* self,void*){const auto id=worldMixedNativeLabel(self);worldMixedNativeEvents.push_back({2,id});return reinterpret_cast<const uint32_t*>(&worldMixedEntities[id-1][1]);}
inline bool __fastcall worldMixedPredicate(void*,void*,void* entity,uint32_t mask){if(mask!=worldMixedActiveMask)throw std::runtime_error("mixed predicate mask");const auto id=worldMixedNativeLabel(entity);worldMixedNativeEvents.push_back({4,id});return worldMixedAllowed(id);}
inline void __fastcall worldMixedResolve(void*,void*,void* entity,void** collideable,void** other){const auto id=worldMixedNativeLabel(entity);worldMixedNativeEvents.push_back({1,id});*collideable=entity;*other=nullptr;}
inline void __fastcall worldMixedClip(void*,void*,const uint8_t* ray,uint32_t mask,void* entity,uint8_t* output){
 if(mask!=worldMixedActiveMask||std::memcmp(ray,worldMixedNativeEnumerated.data(),80))throw std::runtime_error("mixed native clip ray/mask");
 const auto id=worldMixedNativeLabel(entity);worldMixedNativeEvents.push_back({3,id});const auto trace=worldMixedCandidate(id);
 auto put=[&](size_t offset,const auto& value){std::memcpy(output+offset,&value,sizeof(value));};
 std::memcpy(output,trace.start.data(),12);std::memcpy(output+12,trace.end.data(),12);std::memcpy(output+24,trace.plane.normal.data(),12);put(36,trace.plane.distance);std::memcpy(output+40,trace.planeTags.data(),4);
 put(44,trace.fraction);put(48,trace.contents);put(52,trace.displacementFlags);put(54,trace.allSolid);put(55,trace.startSolid);put(56,trace.fractionLeftSolid);
 const uint32_t name=0x10000000u+10+id,identity=0x6000u+id;put(60,name);put(64,trace.surfaceMetadata);put(68,trace.hitGroup);put(72,trace.physicsBone);put(74,trace.worldSurface);put(76,identity);put(80,trace.hitBox);
}

inline void __fastcall worldMixedEnumerate(void*,void*,uint32_t mask,const uint8_t* ray,bool coarse,void* collector){
 if(mask!=1||coarse)throw std::runtime_error("engine world partition arguments");++worldMixedPartitionCalls;std::memcpy(worldMixedNativeEnumerated.data(),ray,80);
 const auto* table=*reinterpret_cast<const uintptr_t*const*>(collector);using Collect=bool(__thiscall*)(void*,void*);
 for(unsigned id=1;id<=worldMixedCandidateCount();++id)(void)reinterpret_cast<Collect>(table[0])(collector,worldMixedEntities[id-1].data());
}
inline void worldMixedNativeCall(const void* ray,void* output,uint32_t mask){
 worldMixedReadProfile();worldMixedNativeEvents.clear();worldMixedNativeEnumerated.fill(0xff);worldMixedActiveMask=mask;worldMixedCandidateTable[2]=uintptr_t(&worldMixedGetCandidateHandle);for(unsigned id=1;id<=worldMixedCandidateCount();++id)worldMixedEntities[id-1]={uintptr_t(worldMixedCandidateTable.data()),worldMixedHandle(id)};
 worldMixedTable[4]=uintptr_t(&worldMixedClip);worldMixedTable[24]=uintptr_t(&worldMixedResolve);
 *reinterpret_cast<uint32_t*>(0x10643ee8)|=1;*reinterpret_cast<uintptr_t*>(0x10643ee4)=uintptr_t(&worldMixedProfile);
 worldMixedPartitionTable[14]=uintptr_t(&worldMixedEnumerate);*reinterpret_cast<uintptr_t*>(0x106683c0)=uintptr_t(worldMixedPartitionTable.data());
 worldMixedTable[25]=uintptr_t(&worldMixedGetWorld);worldMixedTable[26]=0x1016d840;worldMixedTable[27]=uintptr_t(&worldMixedPartitionMask);
 worldMixedObject[0]=uintptr_t(worldMixedTable.data());
 worldMixedCollideableTable[0]=uintptr_t(&worldMixedGetEntity);worldMixedEntityTable[2]=uintptr_t(&worldMixedGetHandle);
 worldMixedCollideableVtable=uintptr_t(worldMixedCollideableTable.data());worldMixedEntityVtable=uintptr_t(worldMixedEntityTable.data());
 worldMixedPartitionTable[14]=uintptr_t(&worldMixedEnumerate);
 *reinterpret_cast<uintptr_t*>(0x106683c0)=uintptr_t(worldMixedPartitionTable.data());
 *reinterpret_cast<uint32_t*>(0x10643ee8)|=1;*reinterpret_cast<uintptr_t*>(0x10643ee4)=uintptr_t(&worldMixedProfile);
 const std::array<uintptr_t,2> filterTable{uintptr_t(&worldMixedPredicate),uintptr_t(&worldMixedTraceType)};uintptr_t filter=uintptr_t(filterTable.data());
 reinterpret_cast<void(__thiscall*)(void*,const void*,uint32_t,const void*,void*)>(0x1016ece0)(worldMixedObject.data(),ray,mask,&filter,output);
 if(worldMixedProfile!=worldMixedNativeCalls+1||worldMixedObject[1]!=worldMixedNativeCalls+1)throw std::runtime_error("real engine world profiling calls");
 supportPut(worldMixedNativeEnumeration,worldMixedNativeEnumerated);worldMixedWriteEvents(worldMixedNativeEventStream,worldMixedNativeEvents);
}
inline uint64_t worldMixedNormalizeEntity(uint32_t value){
 if(value==0)return 0;if(value>=0x6001u&&value<=0x6000u+worldMixedCandidateCount())return 0x4444000000000000ull+(value-0x6000u);if(value!=uintptr_t(&worldMixedEntityVtable))throw std::runtime_error("engine world annotation identity");return worldMixedEntityId;
}
inline void worldMixedNativeRecord(const std::array<uint8_t,88>& row){
 auto read=[&](size_t offset,auto& value){std::memcpy(&value,row.data()+offset,sizeof(value));};
 SourcePhysicsSearchTraceCandidate trace;std::memcpy(trace.start.data(),row.data(),12);std::memcpy(trace.end.data(),row.data()+12,12);std::memcpy(trace.plane.normal.data(),row.data()+24,12);
 read(36,trace.plane.distance);std::memcpy(trace.planeTags.data(),row.data()+40,4);read(44,trace.fraction);read(48,trace.contents);read(52,trace.displacementFlags);trace.allSolid=row[54];trace.startSolid=row[55];read(56,trace.fractionLeftSolid);
 uint32_t name,metadata,entity;read(60,name);read(64,metadata);read(76,entity);
 if(name==UINT32_MAX)trace.surfaceId=0;else if(name>=0x20000000u&&name<0x20010000u)trace.surfaceId=0x2222000000000000ull;
 else if(name>=0x10000000u&&name<0x10010000u)trace.surfaceId=0x1111000000000000ull+(name-0x10000000u);else throw std::runtime_error("engine native surface identity");
 trace.surfaceMetadata=metadata;read(68,trace.hitGroup);read(72,trace.physicsBone);read(74,trace.worldSurface);trace.entityId=worldMixedNormalizeEntity(entity);read(80,trace.hitBox);
 searchWriteTrace(worldMixedNative,trace);++worldMixedNativeCalls;
}
#endif
template<class SurfaceExcludes,class SurfaceDetails,class SurfaceIdentity>
SourceSweptHullWorldTraceCandidate worldMixedOwnedTrace(const SourceBspBrushOnlyTraceCandidate& world,
 const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,const SourcePreparedHullWorldTablesCandidate& tables,
 const std::vector<std::array<uint16_t,2>>& properties,const SourceServerHullRayCandidate& ray,uint32_t mask,
 const std::array<uint16_t,2>& defaultSurface,bool compute,bool available,SurfaceExcludes excludes,SurfaceDetails details,SurfaceIdentity identities){
 if(!compute||ray.isRay||!ray.swept||world.root!=0)throw std::runtime_error("bounded real world engine corpus domain");worldMixedReadProfile();worldMixedStreams();worldMixedOwnedEvents.clear();worldMixedOwnedEnumerated.fill(0xff);worldMixedActiveOwnedMask=mask;
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
   });WorldMixedAnnotationFixture annotate;sourceServerTraceAnnotationCandidate(1,result,{0,0},annotate);
#ifndef _WIN64
   if(result.entityId==1)result.entityId=worldMixedEntityId;
#endif
   return result;
  }
  std::vector<uintptr_t> enumerateCandidates(const SourceServerHullRayCandidate& r){worldMixedOwnedEnumerated=worldMixedRayBytes(r);std::vector<uintptr_t> out;for(unsigned id=1;id<=worldMixedCandidateCount();++id)out.push_back(worldMixedIdentity(id));return out;}
  void candidateOverflow(size_t){throw std::runtime_error("empty partition overflow");}
  uintptr_t resolveCollideable(uintptr_t e){const auto id=worldMixedLabel(e);worldMixedOwnedEvents.push_back({1,id});return e;}
  uint32_t referenceHandle(uintptr_t e){const auto id=worldMixedLabel(e);worldMixedOwnedEvents.push_back({2,id});return worldMixedHandle(id);}
  SourcePhysicsSearchTraceCandidate clipToCollideable(const SourceServerHullRayCandidate& r,uint32_t m,uintptr_t e){if(m!=worldMixedActiveOwnedMask||worldMixedRayBytes(r)!=worldMixedOwnedEnumerated)throw std::runtime_error("mixed owned clip ray/mask");const auto id=worldMixedLabel(e);worldMixedOwnedEvents.push_back({3,id});return worldMixedCandidate(id);}
 }host{world,meshes,tables,properties,defaultSurface,available,excludes,details,identities,publicValue};
 const auto trace=sourceEngineHullTraceCandidate(ray,mask,0,host,[](uintptr_t e,uint32_t m)->bool{if(m!=worldMixedActiveOwnedMask)throw std::runtime_error("mixed owned predicate mask");const auto id=worldMixedLabel(e);worldMixedOwnedEvents.push_back({4,id});return worldMixedAllowed(id);});
 searchWriteTrace(worldMixedOwned,trace);supportPut(worldMixedOwnedEnumeration,worldMixedOwnedEnumerated);worldMixedWriteEvents(worldMixedOwnedEventStream,worldMixedOwnedEvents);const auto bytes=worldMixedRayBytes(ray);supportPut(worldMixedRays,bytes);++worldMixedOwnedCalls;
 publicValue.startPosition=trace.start;publicValue.endPosition=trace.end;publicValue.state.trace.fraction=trace.fraction;publicValue.state.trace.leftSolid=trace.fractionLeftSolid;
 publicValue.state.trace.contents=trace.contents;publicValue.state.trace.allSolid=trace.allSolid!=0;publicValue.state.trace.startSolid=trace.startSolid!=0;
 for(unsigned axis=0;axis<3;++axis)publicValue.state.trace.plane[axis]=trace.plane.normal[axis];publicValue.state.trace.plane[3]=trace.plane.distance;publicValue.state.trace.planeTags=trace.planeTags;
 publicValue.state.displacementFlags=trace.displacementFlags;publicValue.state.surfaceProperty=uint16_t(trace.surfaceMetadata);publicValue.state.surfaceFlags=uint16_t(trace.surfaceMetadata>>16);publicValue.worldSurfaceIndex=trace.worldSurface;
 if(trace.surfaceId>=0x1111000000000000ull&&trace.surfaceId<0x1111000000010000ull)publicValue.state.trace.surface={SourceCollisionSurfaceKindCandidate::RuntimeSurface,uint32_t(trace.surfaceId-0x1111000000000000ull)};
 return publicValue;
}

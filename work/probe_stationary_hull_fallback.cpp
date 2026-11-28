#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef near
#undef far
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>
#include <memory>
#include "source_swept_hull_world_trace_candidate.h"
#include "source_stationary_displacement_hull_normal_candidate.h"
#include "source_stationary_hull_fallback_candidate.h"
#include "source_bsp_displacement_collision_candidate.h"
#include "source_bsp_trace_surface_flags_candidate.h"
#include "source_displacement_grid_triangles_candidate.h"

using Vec=std::array<float,3>;using Row=std::array<uint32_t,16>;
void require(bool v,const char* s){if(!v)throw std::runtime_error(s);}
uint32_t bits(float f){uint32_t u;std::memcpy(&u,&f,4);return u;}
template<class T>T& at(uint8_t* p,size_t n){return *reinterpret_cast<T*>(p+n);}
template<class T>T at(const uint8_t* p,size_t n){T value;std::memcpy(&value,p+n,sizeof(value));return value;}
std::vector<uint8_t> read(const char* p){std::ifstream f(p,std::ios::binary);require(bool(f),"BSP read");return {(std::istreambuf_iterator<char>(f)),{}};}
std::vector<uint32_t> identities(){std::vector<uint32_t> ids(32768);for(uint32_t i=0;i<ids.size();++i)ids[i]=i;return ids;}
std::vector<uint32_t> materialMapping;std::vector<uint16_t> materialFlags;std::unique_ptr<SourceBspTraceSurfaceFlagsCandidate> boundFlags;
bool excludedBrush(const SourceBspBrushOnlyTraceCandidate& world,uint32_t index,unsigned){const auto& r=world.records.at(index);if(r.optimized){for(unsigned a=0;a<6;++a){uint16_t id;std::memcpy(&id,r.box.data()+32+2*a,2);if(id!=65535&&(materialFlags.at(id)&0x80))return true;}}else{const auto& brush=world.data.brushes.at(index);for(uint32_t a=0;a<brush.count;++a){const auto texture=world.data.sides.at(brush.first+a).texture;if(texture>=0&&(boundFlags->flags({SourceCollisionSurfaceKindCandidate::TextureInfo,uint32_t(texture)})&0x80))return true;}}return false;}
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
template<class T>T& address(uintptr_t p){return *reinterpret_cast<T*>(p);}
struct Entry{Vec plane;uint32_t id;};struct Bucket{Entry* entries;uint32_t allocated,grow,count,unused;};
std::vector<std::array<Entry,8192>> entryStorage;std::vector<Bucket> nativeBuckets;
std::array<Vec,8192> nativePlanes{};alignas(16)std::array<uint8_t,0xf4> mesh{};
unsigned inserts=0,appends=0,acquires=0,releases=0;bool sweeping=false;
void allocationBoundary(unsigned slot,CONTEXT& c){using O=PrivateX86BoundaryObserverFixture;const auto m=reinterpret_cast<uintptr_t>(mesh.data());
 if(slot==0){require(!sweeping,"allocation during prepared sweep");auto* b=reinterpret_cast<Bucket*>(c.Ecx);require(b>=nativeBuckets.data()&&b<nativeBuckets.data()+nativeBuckets.size(),"bucket insert owner");const auto id=address<uint32_t>(c.Esp+4);require(id==b->count&&id<8192,"bucket insert index");b->entries[id]=*reinterpret_cast<const Entry*>(address<uintptr_t>(c.Esp+8));++b->count;++inserts;O::returnCdecl(c,id);c.Esp+=8;}
 else if(slot==1){require(!sweeping&&c.Ecx==m+0xc4,"plane append owner/phase");const auto id=address<uint32_t>(c.Esp+4);require(id==at<uint32_t>(mesh.data(),0xd0)&&id<8192,"plane append index");nativePlanes[id]=*reinterpret_cast<const Vec*>(address<uintptr_t>(c.Esp+8));at<uint32_t>(mesh.data(),0xd0)=id+1;++appends;O::returnCdecl(c,id);c.Esp+=8;}
 else if(slot==2){require(sweeping&&c.Ecx==m&&at<uint32_t>(mesh.data(),0xbc)>0,"prepared cache acquire owner");++acquires;require(acquires==1,"single acquire per query");O::returnCdecl(c);}
 else if(slot==3){require(sweeping&&c.Ecx==0x10643680&&address<uint32_t>(c.Esp+4)==0x1234,"prepared cache release");++releases;require(releases==acquires,"paired release");O::returnCdecl(c);c.Esp+=4;}
 else throw std::runtime_error("unexpected boundary");
}
void prepare(unsigned power,unsigned buckets,bool masked,const std::vector<Vec>& vertices,
 const std::vector<SourceDisplacementCollisionTriangleCandidate>& triangles,
 const SourceDisplacementCollisionTreeCandidate& tree,const SourceDisplacementHullCacheCandidate& cache,
 std::array<std::array<uint16_t,9>,512>& indices){
 entryStorage.clear();entryStorage.resize(buckets);nativeBuckets.clear();for(unsigned i=0;i<buckets;++i)nativeBuckets.push_back({entryStorage[i].data(),8192,uint32_t(-1),0,0});mesh.fill(0);sweeping=false;inserts=appends=0;
 const uintptr_t owner=0x106436dc;address<uintptr_t>(owner)=reinterpret_cast<uintptr_t>(nativeBuckets.data());address<uint32_t>(owner+0xc)=buckets;address<uint8_t>(owner+0x16)=masked;address<uint32_t>(owner+0x18)=buckets-1;
 at<uint32_t>(mesh.data(),0x24)=0x1234;at<uint32_t>(mesh.data(),0x28)=power;
 at<uintptr_t>(mesh.data(),0x70)=reinterpret_cast<uintptr_t>(vertices.data());at<uintptr_t>(mesh.data(),0x80)=reinterpret_cast<uintptr_t>(triangles.data());
 at<uintptr_t>(mesh.data(),0x90)=reinterpret_cast<uintptr_t>(tree.nodes.data());at<uint32_t>(mesh.data(),0x98)=uint32_t(tree.nodes.size());at<uintptr_t>(mesh.data(),0xa0)=reinterpret_cast<uintptr_t>(tree.leafPairs.data());
 at<uintptr_t>(mesh.data(),0xb0)=reinterpret_cast<uintptr_t>(indices.data());at<uint32_t>(mesh.data(),0xb4)=512;at<uint32_t>(mesh.data(),0xb8)=uint32_t(-1);
 at<uintptr_t>(mesh.data(),0xc4)=reinterpret_cast<uintptr_t>(nativePlanes.data());at<uint32_t>(mesh.data(),0xc8)=8192;
 using Build=void(__thiscall*)(void*);reinterpret_cast<Build>(0x1015c630)(mesh.data());
 require(at<uint32_t>(mesh.data(),0xbc)==triangles.size()&&at<uint32_t>(mesh.data(),0xd0)==cache.planes.size(),"native cache counts");require(std::memcmp(indices.data(),cache.indices.data(),18*triangles.size())==0,"native cache indices");require(cache.planes.empty()||std::memcmp(nativePlanes.data(),cache.planes.data(),12*cache.planes.size())==0,"native cache planes");require(inserts==appends&&appends==cache.planes.size(),"native cache services");for(const auto& b:nativeBuckets)require(b.count==0,"native cache bucket cleanup");
}
struct NativeNode{const SourceLineWalkPlaneCandidate* plane;std::array<int32_t,2> children;};
struct NativeSide{const SourceBspCollisionPlaneCandidate* plane;uint16_t texture,bevel;};
struct alignas(16)NativeBox{std::array<uint8_t,48> bytes;};
struct NativeLeaf{uint32_t contents=0,unused=0;uint16_t first=0,count=0,dispFirst=0,dispCount=0;};
struct NativeWorld {
 const SourceBspBrushOnlyTraceCandidate& source;
 std::vector<NativeNode> nodes;std::vector<NativeSide> sides;std::vector<NativeBox> boxes;
 std::vector<std::array<uint32_t,2>> brushes;std::vector<NativeLeaf> leaves;std::vector<uint16_t> refs;
 std::vector<std::array<uint32_t,2>> surfaces;
 std::array<uint8_t,0x230> fields{};
 explicit NativeWorld(const SourceBspBrushOnlyTraceCandidate& w):source(w),surfaces(materialFlags.size()){
  static_assert(sizeof(SourceLineWalkPlaneCandidate)==20&&sizeof(NativeNode)==12&&sizeof(NativeLeaf)==16&&sizeof(NativeSide)==8,"native record widths");
  for(const auto& n:w.nodes)nodes.push_back({&w.planes.at(n.plane),n.children});
  for(const auto& s:w.data.sides)sides.push_back({&w.data.planes.at(s.plane),uint16_t(boundFlags->forTexture(s.texture)),uint16_t(s.bevel)});
  for(size_t i=0;i<w.records.size();++i){boxes.push_back({w.records[i].box});const auto& b=w.data.brushes.at(i);require(b.first<65536&&b.count<65535&&i<65536,"runtime span");brushes.push_back({b.contents,w.records[i].optimized?(uint32_t(i)<<16)|65535:(b.first<<16)|b.count});}
  for(const auto& row:w.data.leafBrushes){require(refs.size()<65536&&row.size()<65536,"leaf span");leaves.push_back({0,0,uint16_t(refs.size()),uint16_t(row.size()),0,0});for(auto i:row){require(i<65536,"brush reference");refs.push_back(uint16_t(i));}}
  const auto p=reinterpret_cast<uintptr_t>(fields.data());address<uintptr_t>(p)=reinterpret_cast<uintptr_t>(nodes.data());address<uintptr_t>(p+0x68)=reinterpret_cast<uintptr_t>(sides.data());address<uintptr_t>(p+0x70)=reinterpret_cast<uintptr_t>(boxes.data());address<uintptr_t>(p+0x88)=reinterpret_cast<uintptr_t>(leaves.data());address<uintptr_t>(p+0x98)=reinterpret_cast<uintptr_t>(refs.data());address<uintptr_t>(p+0xa8)=reinterpret_cast<uintptr_t>(brushes.data());address<uintptr_t>(p+0x218)=reinterpret_cast<uintptr_t>(surfaces.data());
 }
};
#endif

std::array<uint16_t,2> details(const SourceCollisionSurfaceCandidate& surface,unsigned){const auto id=boundFlags->identity(surface);if(id==SourceBspTraceSurfaceFlagsCandidate::defaultId)return {0x3344,0};require(id<materialFlags.size(),"bound brush material identity");return {uint16_t(1000+id),materialFlags[id]};}
using Record=std::array<uint8_t,88>;
Record pack(const SourceSweptHullWorldTraceCandidate& value){Record r{};auto* t=r.data();const auto& s=value.state;std::memcpy(t,value.startPosition.data(),12);std::memcpy(t+12,value.endPosition.data(),12);std::memcpy(t+0x18,s.trace.plane.data(),16);std::memcpy(t+0x28,s.trace.planeTags.data(),4);at<float>(t,0x2c)=s.trace.fraction;at<uint32_t>(t,0x30)=s.trace.contents;at<uint16_t>(t,0x34)=s.displacementFlags;t[0x36]=uint8_t(s.trace.allSolid);t[0x37]=uint8_t(s.trace.startSolid);at<float>(t,0x38)=s.trace.leftSolid;const auto kind=s.trace.surface.kind;at<uint32_t>(t,0x3c)=kind==SourceCollisionSurfaceKindCandidate::Default?0xffffffffu:kind==SourceCollisionSurfaceKindCandidate::Displacement?0x20000000u+s.trace.surface.index:0x10000000u+boundFlags->identity(s.trace.surface);at<uint16_t>(t,0x40)=s.surfaceProperty;at<uint16_t>(t,0x42)=s.surfaceFlags;at<uint16_t>(t,0x4a)=value.worldSurfaceIndex;at<uint32_t>(t,0x54)=s.displacementHit;return r;}
Record owned(const SourceBspBrushOnlyTraceCandidate& world,const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,const SourcePreparedHullWorldTablesCandidate& tables,const std::vector<std::array<uint16_t,2>>& properties,const SourceServerHullRayCandidate& ray,uint32_t mask,bool compute,bool available){return pack(sourceSweptHullWorldTraceCandidate(world,meshes,tables,properties,ray,mask,{0x3344,0},compute,available,[&](uint32_t index){return excludedBrush(world,index,0);},[&](const SourceCollisionSurfaceCandidate& surface){return details(surface,0);},[&](const SourceCollisionSurfaceCandidate& surface){return boundFlags->identity(surface);}));}
#ifndef _WIN64
std::vector<std::array<uint8_t,244>> nativeObjects;
std::vector<std::vector<Vec>> storedPlanes;std::vector<std::vector<std::array<uint16_t,9>>> storedIndices;
std::vector<std::array<uint8_t,48>> nativeCulls;
bool queryPhase=false;uintptr_t activeWork=0;int openCache=-1;uint32_t currentCandidate=UINT32_MAX,selectedIndex=UINT32_MAX;float nativeBeforeFraction=1,initialFraction=1;bool candidateHadBrush=false,caseHadDisp=false,caseBrushThenDisp=false;unsigned nativeDispThenBrush=0,nativeBrushThenDisp=0;
const SourceServerHullRayCandidate* activeRay=nullptr;uint32_t activeMask=0;unsigned poolAcquires=0,poolReleases=0,setupObservations=0;
void slotZero(CONTEXT& c,uint32_t entry){auto* observer=PrivateX86BoundaryObserverFixture::active;require(observer!=nullptr,"active wrapper observer");observer->entries[0]=entry;c.Dr0=entry;c.ContextFlags|=CONTEXT_DEBUG_REGISTERS;}
void boundary(unsigned slot,CONTEXT& c){if(!queryPhase){allocationBoundary(slot,c);return;}using O=PrivateX86BoundaryObserverFixture;
 if(slot==0){const auto base=reinterpret_cast<uintptr_t>(nativeObjects.data());require(c.Ecx==activeWork&&c.Edx>=base&&(c.Edx-base)%244==0&&(c.Edx-base)/244<nativeObjects.size(),"fallback displacement identity");currentCandidate=uint32_t((c.Edx-base)/244);nativeBeforeFraction=at<float>(reinterpret_cast<uint8_t*>(activeWork),0x80);c.EFlags|=0x10000u;}
 else if(slot==1){require(c.Esi==activeWork&&currentCandidate<nativeObjects.size(),"fallback displacement return");if(at<float>(reinterpret_cast<uint8_t*>(activeWork),0x80)<nativeBeforeFraction)selectedIndex=currentCandidate;currentCandidate=UINT32_MAX;c.EFlags|=0x10000u;}
 else if(slot==2){const auto base=reinterpret_cast<uintptr_t>(nativeObjects.data());require(openCache==-1&&c.Ecx>=base&&(c.Ecx-base)%244==0&&(c.Ecx-base)/244<nativeObjects.size(),"fallback cache acquire");openCache=int((c.Ecx-base)/244);++acquires;O::returnCdecl(c);}
 else if(slot==3){require(openCache>=0&&c.Ecx==0x10643680&&address<uint32_t>(c.Esp+4)==0x1234u+uint32_t(openCache),"fallback cache release");openCache=-1;++releases;O::returnCdecl(c);c.Esp+=4;}
 else throw std::runtime_error("unexpected fallback boundary");
}
void prepareObjects(const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,const std::vector<SourceDisplacementHullCacheCandidate>& caches,const std::vector<std::array<uint16_t,2>>& properties,const std::vector<SourceLeafDisplacementCullCandidate>& culls,unsigned buckets,bool masked){
 nativeObjects.resize(meshes.size());storedPlanes.resize(meshes.size());storedIndices.resize(meshes.size());nativeCulls.resize(meshes.size());
 for(size_t i=0;i<meshes.size();++i){const auto& input=meshes.at(i);std::array<std::array<uint16_t,9>,512> indices{};prepare(input.power,buckets,masked,input.vertices,input.triangles,input.tree,caches.at(i),indices);storedPlanes[i].assign(nativePlanes.begin(),nativePlanes.begin()+caches[i].planes.size());storedIndices[i].assign(indices.begin(),indices.begin()+input.triangles.size());nativeObjects[i]=mesh;auto* object=nativeObjects[i].data();at<uintptr_t>(object,0xb0)=reinterpret_cast<uintptr_t>(storedIndices[i].data());at<uintptr_t>(object,0xc4)=reinterpret_cast<uintptr_t>(storedPlanes[i].data());at<uint32_t>(object,0x24)=0x1234u+uint32_t(i);at<uint32_t>(object,0x20)=input.contents;at<uint32_t>(object,0x2c)=input.runtimeFlags;std::memcpy(object+4,input.tree.expandedBounds.data(),12);std::memcpy(object+0x14,input.tree.expandedBounds.data()+3,12);std::memcpy(object+0x60,input.baseNormal.data(),12);std::memcpy(object+0x6c,properties[i].data(),4);auto* c=nativeCulls[i].data();nativeCulls[i].fill(0);std::memcpy(c,culls[i].mins.data(),12);std::memcpy(c+16,culls[i].maxs.data(),12);at<uint32_t>(c,32)=culls[i].visitIndex;at<uint32_t>(c,36)=culls[i].contents;}
 address<uintptr_t>(0x10637820)=reinterpret_cast<uintptr_t>(nativeObjects.data());address<uintptr_t>(0x10637824)=reinterpret_cast<uintptr_t>(nativeCulls.data());
}

#endif

using Work=std::array<uint8_t,0x230>;
SourcePreparedWorldHullStateCandidate initialState(unsigned mode){SourcePreparedWorldHullStateCandidate s;s.trace.fraction=mode?.25f:1.f;s.trace.leftSolid=mode?.75f:0.f;s.trace.allSolid=mode!=0;s.trace.startSolid=mode!=0;s.trace.contents=0x12345678;s.trace.plane={9,8,7,6};s.trace.planeTags={5,4,3,2};s.trace.surface={SourceCollisionSurfaceKindCandidate::RuntimeSurface,77};s.displacementFlags=0xbeef;s.displacementHit=mode?0xffffffffu:0;s.surfaceProperty=0xabcd;s.surfaceFlags=0xef01;s.worldSurfaceIndexSource=SourceCollisionSurfaceCandidate{SourceCollisionSurfaceKindCandidate::RuntimeSurface,mode?77u:23u};return s;}
void putState(Work& work,const SourcePreparedWorldHullStateCandidate& state){SourceSweptHullWorldTraceCandidate trace;trace.state=state;trace.startPosition={11,12,13};trace.endPosition={14,15,16};if(state.worldSurfaceIndexSource){const auto id=boundFlags->identity(*state.worldSurfaceIndexSource);trace.worldSurfaceIndex=id==UINT32_MAX?65535:uint16_t(id);}const auto record=pack(trace);std::memcpy(work.data()+0x54,record.data(),84);at<uint32_t>(work.data(),0x114)=state.displacementHit;at<uint32_t>(work.data(),0x98)=0x11112222;at<uint16_t>(work.data(),0x9c)=0x3344;at<uint32_t>(work.data(),0xa0)=0x55556666;at<uint32_t>(work.data(),0xa4)=0x77778888;}
Work initialWork(const Vec& extents,uint32_t mask,unsigned mode,bool wrap){Work work;work.fill(0xa7);const Vec start{91,92,93},end{-91,-92,-93};std::memcpy(work.data(),start.data(),12);std::memcpy(work.data()+12,end.data(),12);Vec minimum;for(size_t a=0;a<3;++a)minimum[a]=-extents[a];std::memcpy(work.data()+0x18,minimum.data(),12);std::memcpy(work.data()+0x24,extents.data(),12);std::memcpy(work.data()+0x30,extents.data(),12);at<uint32_t>(work.data(),0xfc)=mask;work[0x100]=work[0x101]=0;at<uint32_t>(work.data(),0x104)=0;at<uint32_t>(work.data(),0x11c)=0;at<uint32_t>(work.data(),0x120)=17;at<uint32_t>(work.data(),0x124)=wrap?UINT32_MAX:0;for(size_t offset:{size_t(0x128),size_t(0x13c),size_t(0x150),size_t(0x164)})at<uint32_t>(work.data(),offset)=0;at<uint32_t>(work.data(),0x148)=1866;at<uint32_t>(work.data(),0x170)=43;putState(work,initialState(mode));return work;}
Work ownedFallback(const SourceBspBrushOnlyTraceCandidate& world,const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,const SourcePreparedHullWorldTablesCandidate& tables,const std::vector<std::array<uint16_t,2>>& properties,const Vec& point,const Vec& normal,const Vec& extents,uint32_t mask,unsigned mode,bool wrap){
 auto work=initialWork(extents,mask,mode,wrap);auto state=initialState(mode);const auto ray=sourceStationaryHullFallbackCandidate(point,normal,{0x3344,0},state,[&](const auto& prepared,auto& result){sourcePreparedWorldHullCandidate(world,meshes,tables.caches,tables.leafDisplacements,tables.culls,properties,prepared.start,prepared.delta,extents,mask,result,[&](uint32_t index){return excludedBrush(world,index,0);},[&](const SourceCollisionSurfaceCandidate& surface){return details(surface,0);},false);});
 std::memcpy(work.data(),ray.start.data(),12);std::memcpy(work.data()+0x3c,ray.delta.data(),12);std::memcpy(work.data()+0x48,ray.inverse.data(),12);std::memcpy(work.data()+0x108,ray.normal.data(),12);at<uint32_t>(work.data(),0x124)=1;putState(work,state);return work;
}
#ifndef _WIN64
Work nativeFallback(NativeWorld& world,const std::vector<std::vector<uint32_t>>& refs,const Vec& point,const Vec& normal,const Vec& extents,uint32_t mask,unsigned mode,bool wrap){
 for(uint32_t i=0;i<world.surfaces.size();++i)world.surfaces[i]={0x10000000u+i,uint32_t(1000+i)|(uint32_t(materialFlags[i])<<16)};address<uint32_t>(0x10402fe8)=0xffffffffu;address<uint32_t>(0x10402fec)=0x3344;require(address<uint32_t>(0x106c1228)==0,"private memset feature state");
 std::vector<uint16_t> dispRefs;for(size_t leaf=0;leaf<refs.size();++leaf){require(dispRefs.size()<65536&&refs[leaf].size()<65536,"fallback displacement leaf span");world.leaves[leaf].dispFirst=uint16_t(dispRefs.size());world.leaves[leaf].dispCount=uint16_t(refs[leaf].size());for(auto id:refs[leaf])dispRefs.push_back(uint16_t(id));}at<uintptr_t>(world.fields.data(),0xb0)=reinterpret_cast<uintptr_t>(dispRefs.data());
 auto work=initialWork(extents,mask,mode,wrap);std::vector<uint32_t> parentBrush(world.brushes.size(),0xcafebabe),parentDisp(nativeObjects.size(),0xcafebabe),brushVisits(world.brushes.size(),wrap?17u:0u),dispVisits(nativeObjects.size(),wrap?17u:0u);at<uintptr_t>(work.data(),0x104)=reinterpret_cast<uintptr_t>(world.fields.data());at<uintptr_t>(work.data(),0x128)=reinterpret_cast<uintptr_t>(parentBrush.data());at<uintptr_t>(work.data(),0x13c)=reinterpret_cast<uintptr_t>(brushVisits.data());at<uintptr_t>(work.data(),0x150)=reinterpret_cast<uintptr_t>(parentDisp.data());at<uintptr_t>(work.data(),0x164)=reinterpret_cast<uintptr_t>(dispVisits.data());
 activeWork=reinterpret_cast<uintptr_t>(work.data());openCache=-1;currentCandidate=selectedIndex=UINT32_MAX;acquires=releases=0;using Fallback=void(__cdecl*)(void*,const float*,const float*,uint32_t);reinterpret_cast<Fallback>(0x1014ce00)(work.data(),point.data(),normal.data(),mode?0x89123456u:0u);require(openCache==-1&&acquires==releases,"fallback cache balance");
 require(at<uint32_t>(work.data(),0x11c)==0&&at<uint32_t>(work.data(),0x120)==17&&at<uint32_t>(work.data(),0x124)==1,"native nested visit transaction");for(auto mark:parentBrush)require(mark==0xcafebabe,"parent brush visit overwritten");for(auto mark:parentDisp)require(mark==0xcafebabe,"parent displacement visit overwritten");for(auto mark:brushVisits)require(mark<=1,"child brush generation/reset");for(auto mark:dispVisits)require(mark==0,"nonswept child displacement marks");
 const auto name=at<uint32_t>(work.data(),0x90);if(name==0x10350ee0u){require(selectedIndex<nativeObjects.size(),"fallback winning displacement identity");at<uint32_t>(work.data(),0x90)=0x20000000u+selectedIndex;}else require(name==0xffffffffu||(name>=0x10000000u&&name<0x10000000u+materialFlags.size()),"fallback surface namespace");
 for(size_t offset:{size_t(0x104),size_t(0x128),size_t(0x13c),size_t(0x150),size_t(0x164)})at<uint32_t>(work.data(),offset)=0;return work;
}
#endif

unsigned normalTests(std::ofstream& out){
 std::vector<SourceDisplacementRuntimeMeshCandidate> meshes(4);const std::array<Vec,4> normals{Vec{-0.f,0,1},Vec{1,0,0},Vec{0,-1,0},Vec{.6f,.8f,0}};for(size_t i=0;i<4;++i){auto& m=meshes[i];m.tree.expandedBounds={float(i)*8-4,-2,-1,float(i)*8+4,2,1};m.baseNormal=normals[i];m.contents=i==3?0x80u:1u<<i;m.runtimeFlags=1u<<i;}
 const std::vector<std::vector<uint32_t>> lists{{},{0},{1,0},{3,2,1,0},{0,1,0,2},{2,2},{3,0,3}};
 const std::array<Vec,5> minimum{Vec{},Vec{-2,-3,-4},Vec{-24,-24,-41},Vec{-5,1,-7},Vec{100,200,300}},maximum{Vec{},Vec{2,3,4},Vec{24,24,41},Vec{-2,9,-3},Vec{104,206,308}};
#ifndef _WIN64
 std::vector<std::array<uint8_t,244>> objects(4);for(size_t i=0;i<4;++i){std::memcpy(objects[i].data()+4,meshes[i].tree.expandedBounds.data(),12);std::memcpy(objects[i].data()+0x14,meshes[i].tree.expandedBounds.data()+3,12);std::memcpy(objects[i].data()+0x60,meshes[i].baseNormal.data(),12);at<uint32_t>(objects[i].data(),0x20)=meshes[i].contents;at<uint32_t>(objects[i].data(),0x2c)=meshes[i].runtimeFlags;}address<uintptr_t>(0x10637820)=reinterpret_cast<uintptr_t>(objects.data());const auto beforeObjects=objects;
#endif
 unsigned cases=0;
 for(unsigned q=0;q<32;++q)for(unsigned variant=0;variant<5;++variant)for(bool isRay:{false,true})for(const auto& refs:lists)for(uint32_t mask:{0u,1u,2u,0x85u,0xffffffffu}){
  const auto& b=meshes[q%4].tree.expandedBounds;Vec point{(b[0]+b[3])*.5f,0,0};const float ex=(maximum[variant][0]-minimum[variant][0])*.5f,ey=(maximum[variant][1]-minimum[variant][1])*.5f;
  switch(q/4){case 0:break;case 1:point[0]=b[3]+ex;break;case 2:point[0]=std::nextafter(b[3]+ex,std::numeric_limits<float>::infinity());break;case 3:point[1]=b[1]-ey;break;case 4:point[1]=std::nextafter(b[1]-ey,-std::numeric_limits<float>::infinity());break;case 5:point={1000,1000,1000};break;case 6:point={-4,-2,-1};break;case 7:point={28,2,1};break;}
  Vec normal{91,-0.f,93};uint32_t contents=0xdeadbeef;sourceStationaryDisplacementHullNormalCandidate(meshes,refs,point,minimum[variant],maximum[variant],isRay,mask,normal,contents);std::array<uint8_t,16> expected{};std::memcpy(expected.data(),normal.data(),12);at<uint32_t>(expected.data(),12)=contents;auto actual=expected;
#ifndef _WIN64
  std::array<uint8_t,0x220> work;work.fill(0xa7);std::memcpy(work.data(),point.data(),12);std::memcpy(work.data()+0x18,minimum[variant].data(),12);std::memcpy(work.data()+0x24,maximum[variant].data(),12);work[0x100]=uint8_t(isRay);const auto beforeWork=work;std::vector<uint16_t> nativeRefs;for(auto id:refs)nativeRefs.push_back(uint16_t(id));std::array<uint8_t,48> result;result.fill(0xa5);const Vec initialNormal{91,-0.f,93};std::memcpy(result.data()+16,initialNormal.data(),12);at<uint32_t>(result.data(),28)=0xdeadbeef;using Normal=void(__cdecl*)(void*,const uint16_t*,int,float*,uint32_t,uint32_t*);reinterpret_cast<Normal>(0x1014cd40)(work.data(),nativeRefs.data(),int(nativeRefs.size()),reinterpret_cast<float*>(result.data()+16),mask,reinterpret_cast<uint32_t*>(result.data()+28));require(work==beforeWork&&objects==beforeObjects,"normal selection mutated inputs");for(size_t i=0;i<16;++i)require(result[i]==0xa5&&result[32+i]==0xa5,"normal output guard");std::memcpy(actual.data(),result.data()+16,16);if(actual!=expected){std::cerr<<"normal case "<<cases<<" q "<<q<<" variant "<<variant<<" isRay "<<isRay<<" mask "<<mask<<'\n';throw std::runtime_error("hull normal mismatch");}
#endif
  out.write(reinterpret_cast<const char*>(actual.data()),actual.size());++cases;
 }
 return cases;
}

int main(int argc,char** argv){try{
 require(argc==5,"expected engine or -, BSP, private permutation and disposable output");const auto bsp=read(argv[2]);auto word=[&](size_t offset){require(offset<=bsp.size()&&bsp.size()-offset>=4,"BSP word");uint32_t value;std::memcpy(&value,bsp.data()+offset,4);return value;};const auto texinfo=word(8+6*16),texinfoBytes=word(8+6*16+4),texdataBytes=word(8+2*16+4);require(texinfoBytes%72==0&&texdataBytes%32==0&&texinfo<=bsp.size()&&texinfoBytes<=bsp.size()-texinfo,"material spans");materialFlags.resize(texdataBytes/32);for(uint32_t i=0;i<texinfoBytes/72;++i){const auto id=word(texinfo+72*i+68);require(id<materialFlags.size(),"material identity");materialMapping.push_back(id);materialFlags[id]|=uint16_t(word(texinfo+72*i+64));}boundFlags=std::make_unique<SourceBspTraceSurfaceFlagsCandidate>(bsp,materialMapping);SourceBspBrushOnlyTraceCandidate world(bsp,materialMapping);require(world.root==0&&world.data.brushes.size()==1866,"fallback world root/inventory");
 std::array<uint32_t,256> raw{};std::ifstream fixture(argv[3],std::ios::binary);fixture.read(reinterpret_cast<char*>(raw.data()),sizeof(raw));require(fixture.gcount()==sizeof(raw)&&fixture.peek()==std::char_traits<char>::eof(),"permutation fixture");SourcePlaneHashPermutationCandidate permutation{};for(size_t i=0;i<raw.size();++i){require(raw[i]<256,"permutation value");permutation[i]=uint8_t(raw[i]);}
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1],0x643eec);image.seal();require(std::memcmp(raw.data(),reinterpret_cast<void*>(0x104129b0),1024)==0,"permutation identity");PrivateX86BoundaryObserverFixture observer{{0x1015b150,0x10001610,0x1015c760,0x10261300},boundary};
#endif
 std::ofstream out(argv[4],std::ios::binary);require(bool(out),"output");std::array<uint32_t,4> header{0x53484e31,0,0,0x230};out.write(reinterpret_cast<const char*>(header.data()),16);const auto normalCases=normalTests(out);unsigned cases=0,hits=0,solid=0,wrapped=0,zeroDirection=0,zeroMask=0,retainedContents=0,totalAcquires=0;
 for(unsigned hdr=0;hdr<2;++hdr){const SourceBspDisplacementCollisionCandidate imported(bsp,int32_t(hdr));const auto& meshes=imported.meshes;require(meshes.size()==43,"benchmark meshes");auto tables=sourcePreparedHullWorldTablesCandidate(world,meshes,permutation,64,true);std::vector<std::array<uint16_t,2>> properties;for(size_t i=0;i<meshes.size();++i)properties.push_back({uint16_t(100+i),uint16_t(500+i)});
#ifndef _WIN64
  queryPhase=false;observer.entries={0x1015b150,0x10001610,0x1015c760,0x10261300};observer.configure(true);prepareObjects(meshes,tables.caches,properties,tables.culls,64,true);observer.configure(false);queryPhase=true;observer.entries={0x1014d100,0x10148831,0x1015c760,0x10261300};observer.configure(true);NativeWorld nativeWorld(world);
#endif
  for(size_t index=0;index<meshes.size();++index){const auto& m=meshes[index];const auto& b=m.tree.expandedBounds;for(unsigned q=0;q<2;++q){Vec point{(b[0]+b[3])*.5f,(b[1]+b[4])*.5f,(b[2]+b[5])*.5f};for(size_t a=0;a<3;++a)point[a]+=(q?-.25f:64.f)*m.baseNormal[a];for(unsigned hull=0;hull<2;++hull)for(unsigned direction=0;direction<4;++direction)for(unsigned mode=0;mode<2;++mode)for(uint32_t mask:{0u,0x2081u,0xffffffffu})for(bool wrap:{false,true}){const Vec extents=hull?Vec{24,24,41}:Vec{2,3,5};Vec normal=m.baseNormal;if(direction==1)for(auto& v:normal)v=-v;if(direction==2)normal={0,-0.f,0};if(direction==3)normal={.6f,.8f,.25f};const auto expected=ownedFallback(world,meshes,tables,properties,point,normal,extents,mask,mode,wrap);auto actual=expected;
#ifndef _WIN64
   actual=nativeFallback(nativeWorld,tables.leafDisplacements,point,normal,extents,mask,mode,wrap);if(actual!=expected){std::cerr<<"fallback case "<<cases<<" hdr "<<hdr<<" mesh "<<index<<" q "<<q<<" hull "<<hull<<" direction "<<direction<<" mode "<<mode<<" mask "<<mask<<" wrap "<<wrap<<'\n';for(size_t i=0;i<actual.size();++i)if(actual[i]!=expected[i])std::cerr<<"byte "<<i<<" native "<<unsigned(actual[i])<<" owned "<<unsigned(expected[i])<<'\n';throw std::runtime_error("full stationary hull fallback mismatch");}totalAcquires+=acquires;
#endif
   out.write(reinterpret_cast<const char*>(actual.data()),actual.size());require(bool(out),"fallback record write");++cases;hits+=at<float>(actual.data(),0x80)<1;solid+=actual[0x8b]!=0;wrapped+=wrap;zeroDirection+=direction==2;zeroMask+=mask==0;retainedContents+=at<uint32_t>(actual.data(),0x84)==0x12345678;
  }}}
#ifndef _WIN64
  observer.configure(false);for(size_t i=0;i<meshes.size();++i){require(storedIndices[i]==tables.caches[i].indices,"fallback cache indices changed");require(std::memcmp(storedPlanes[i].data(),tables.caches[i].planes.data(),storedPlanes[i].size()*12)==0,"fallback cache planes changed");}
#endif
 }
 header[1]=normalCases;header[2]=cases;out.seekp(0);out.write(reinterpret_cast<const char*>(header.data()),16);out.flush();require(bool(out),"final record header");
#ifndef _WIN64
 image.verify();
#endif
 require(hits&&solid&&wrapped&&zeroDirection&&zeroMask&&retainedContents,"fallback coverage");std::cout<<"{\"normal_cases\":"<<normalCases<<",\"fallback_cases\":"<<cases<<",\"hits\":"<<hits<<",\"start_solid\":"<<solid<<",\"wrapped_generations\":"<<wrapped<<",\"zero_direction\":"<<zeroDirection<<",\"zero_mask\":"<<zeroMask<<",\"retained_contents\":"<<retainedContents<<",\"native_cache_acquires\":"<<totalAcquires<<",\"passed\":true}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

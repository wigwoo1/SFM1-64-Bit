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
#include "source_stationary_displacement_hull_policy_candidate.h"
#include "source_stationary_hull_leaf_candidate.h"
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
 std::array<uint8_t,0x230> fields{};size_t baseRefCount=0;
 explicit NativeWorld(const SourceBspBrushOnlyTraceCandidate& w):source(w),surfaces(materialFlags.size()){
  static_assert(sizeof(SourceLineWalkPlaneCandidate)==20&&sizeof(NativeNode)==12&&sizeof(NativeLeaf)==16&&sizeof(NativeSide)==8,"native record widths");
  for(const auto& n:w.nodes)nodes.push_back({&w.planes.at(n.plane),n.children});
  for(const auto& s:w.data.sides)sides.push_back({&w.data.planes.at(s.plane),uint16_t(boundFlags->forTexture(s.texture)),uint16_t(s.bevel)});
  for(size_t i=0;i<w.records.size();++i){boxes.push_back({w.records[i].box});const auto& b=w.data.brushes.at(i);require(b.first<65536&&b.count<65535&&i<65536,"runtime span");brushes.push_back({b.contents,w.records[i].optimized?(uint32_t(i)<<16)|65535:(b.first<<16)|b.count});}
  for(const auto& row:w.data.leafBrushes){require(refs.size()<65536&&row.size()<65536,"leaf span");leaves.push_back({0,0,uint16_t(refs.size()),uint16_t(row.size()),0,0});for(auto i:row){require(i<65536,"brush reference");refs.push_back(uint16_t(i));}}
  baseRefCount=refs.size();const auto p=reinterpret_cast<uintptr_t>(fields.data());address<uintptr_t>(p)=reinterpret_cast<uintptr_t>(nodes.data());address<uintptr_t>(p+0x68)=reinterpret_cast<uintptr_t>(sides.data());address<uintptr_t>(p+0x70)=reinterpret_cast<uintptr_t>(boxes.data());address<uintptr_t>(p+0x88)=reinterpret_cast<uintptr_t>(leaves.data());address<uintptr_t>(p+0x98)=reinterpret_cast<uintptr_t>(refs.data());address<uintptr_t>(p+0xa8)=reinterpret_cast<uintptr_t>(brushes.data());address<uintptr_t>(p+0x218)=reinterpret_cast<uintptr_t>(surfaces.data());
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
std::vector<uint32_t> nativeBrushOrder;bool nativeLeafDispatched=false;uintptr_t activeBrushBase=0;uint32_t* activeParentBrush=nullptr;size_t activeBrushCount=0;
std::vector<uint32_t> nativeCullOrder,nativeOverlapOrder;uint32_t nativeDirectMesh=UINT32_MAX;bool nativeUsedFallback=false;uint32_t* activeParentDisp=nullptr;
void slotAddress(CONTEXT& c,unsigned slot,uint32_t entry){auto* observer=PrivateX86BoundaryObserverFixture::active;require(observer&&slot<2,"policy observation slot");observer->entries[slot]=entry;if(slot==0)c.Dr0=entry;else c.Dr1=entry;c.ContextFlags|=CONTEXT_DEBUG_REGISTERS;}
void boundary(unsigned slot,CONTEXT& c){if(!queryPhase){allocationBoundary(slot,c);return;}using O=PrivateX86BoundaryObserverFixture;
 if(slot==0&&c.Eip==0x10146c30){const auto pointer=address<uintptr_t>(c.Esp+4);require(c.Ecx==activeWork&&pointer>=activeBrushBase&&(pointer-activeBrushBase)%8==0&&(pointer-activeBrushBase)/8<activeBrushCount,"stationary leaf brush identity");const auto index=uint32_t((pointer-activeBrushBase)/8);require(activeParentBrush[index]==17&&(address<uint32_t>(pointer)&address<uint32_t>(activeWork+0xfc)),"leaf mark-before-brush and mask");nativeBrushOrder.push_back(index);require(address<uint32_t>(c.Esp)==0x10147fbb,"stationary brush caller");c.EFlags|=0x10000u;}
 else if(slot==1&&c.Eip==0x1014d320){nativeLeafDispatched=true;require(address<uint32_t>(c.Esp)==0x10148020,"stationary displacement caller");slotAddress(c,0,0x1014e5d0);slotAddress(c,1,0x1014ce00);c.EFlags|=0x10000u;}
 else if(slot==0&&c.Eip==0x1014e5d0){const auto base=reinterpret_cast<uintptr_t>(nativeCulls.data()),pointer=address<uintptr_t>(c.Esp+12);require(pointer>=base&&(pointer-base)%48==0&&(pointer-base)/48<nativeCulls.size(),"policy cull identity");const uint32_t id=uint32_t((pointer-base)/48),visit=address<uint32_t>(pointer+32);require(activeParentDisp&&visit<nativeCulls.size()&&activeParentDisp[visit]==17&&(address<uint32_t>(pointer+36)&address<uint32_t>(activeWork+0xfc)),"contents-before-visit and mark-before-cull");nativeCullOrder.push_back(id);require(address<uint32_t>(c.Esp)==0x1014d44f,"cull return boundary");slotAddress(c,0,0x1014d44f);c.EFlags|=0x10000u;}
 else if(slot==0&&c.Eip==0x1014d44f){slotAddress(c,0,0x1014e5d0);slotAddress(c,1,(c.Eax&255u)?0x1015a7d0:0x1014ce00);c.EFlags|=0x10000u;}
 else if(slot==1&&c.Eip==0x1015a7d0){const auto base=reinterpret_cast<uintptr_t>(nativeObjects.data());require(c.Ecx>=base&&(c.Ecx-base)%244==0&&(c.Ecx-base)/244<nativeObjects.size(),"policy overlap identity");const auto id=uint32_t((c.Ecx-base)/244);require(!nativeCullOrder.empty()&&id==nativeCullOrder.back(),"overlap follows admitted cull");nativeOverlapOrder.push_back(id);slotAddress(c,1,0x1014d471);c.EFlags|=0x10000u;}
 else if(slot==1&&c.Eip==0x1014d471){if(c.Eax&255u)nativeDirectMesh=nativeOverlapOrder.back();slotAddress(c,1,0x1014ce00);c.EFlags|=0x10000u;}
 else if(slot==1&&c.Eip==0x1014ce00){nativeUsedFallback=true;slotAddress(c,0,0x1014d100);slotAddress(c,1,0x10148831);c.EFlags|=0x10000u;}
 else if(slot==0&&c.Eip==0x1014d100){const auto base=reinterpret_cast<uintptr_t>(nativeObjects.data());require(c.Ecx==activeWork&&c.Edx>=base&&(c.Edx-base)%244==0&&(c.Edx-base)/244<nativeObjects.size(),"policy fallback displacement identity");currentCandidate=uint32_t((c.Edx-base)/244);nativeBeforeFraction=at<float>(reinterpret_cast<uint8_t*>(activeWork),0x80);c.EFlags|=0x10000u;}
 else if(slot==1&&c.Eip==0x10148831){require(c.Esi==activeWork&&currentCandidate<nativeObjects.size(),"policy fallback displacement return");if(at<float>(reinterpret_cast<uint8_t*>(activeWork),0x80)<nativeBeforeFraction)selectedIndex=currentCandidate;currentCandidate=UINT32_MAX;c.EFlags|=0x10000u;}
 else if(slot==2){const auto base=reinterpret_cast<uintptr_t>(nativeObjects.data());require(openCache==-1&&c.Ecx>=base&&(c.Ecx-base)%244==0&&(c.Ecx-base)/244<nativeObjects.size(),"policy cache acquire");openCache=int((c.Ecx-base)/244);++acquires;O::returnCdecl(c);}
 else if(slot==3){require(openCache>=0&&c.Ecx==0x10643680&&address<uint32_t>(c.Esp+4)==0x1234u+uint32_t(openCache),"policy cache release");openCache=-1;++releases;O::returnCdecl(c);c.Esp+=4;}
 else throw std::runtime_error("unexpected policy boundary");
}
void prepareObjects(const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,const std::vector<SourceDisplacementHullCacheCandidate>& caches,const std::vector<std::array<uint16_t,2>>& properties,const std::vector<SourceLeafDisplacementCullCandidate>& culls,unsigned buckets,bool masked){
 nativeObjects.resize(meshes.size());storedPlanes.resize(meshes.size());storedIndices.resize(meshes.size());nativeCulls.resize(meshes.size());
 for(size_t i=0;i<meshes.size();++i){const auto& input=meshes.at(i);std::array<std::array<uint16_t,9>,512> indices{};prepare(input.power,buckets,masked,input.vertices,input.triangles,input.tree,caches.at(i),indices);storedPlanes[i].assign(nativePlanes.begin(),nativePlanes.begin()+caches[i].planes.size());storedIndices[i].assign(indices.begin(),indices.begin()+input.triangles.size());nativeObjects[i]=mesh;auto* object=nativeObjects[i].data();at<uintptr_t>(object,0xb0)=reinterpret_cast<uintptr_t>(storedIndices[i].data());at<uintptr_t>(object,0xc4)=reinterpret_cast<uintptr_t>(storedPlanes[i].data());at<uint32_t>(object,0x24)=0x1234u+uint32_t(i);at<uint32_t>(object,0x20)=input.contents;at<uint32_t>(object,0x2c)=input.runtimeFlags;std::memcpy(object+4,input.tree.expandedBounds.data(),12);std::memcpy(object+0x14,input.tree.expandedBounds.data()+3,12);std::memcpy(object+0x60,input.baseNormal.data(),12);std::memcpy(object+0x6c,properties[i].data(),4);auto* c=nativeCulls[i].data();nativeCulls[i].fill(0);std::memcpy(c,culls[i].mins.data(),12);std::memcpy(c+16,culls[i].maxs.data(),12);at<uint32_t>(c,32)=culls[i].visitIndex;at<uint32_t>(c,36)=culls[i].contents;}
 address<uintptr_t>(0x10637820)=reinterpret_cast<uintptr_t>(nativeObjects.data());address<uintptr_t>(0x10637824)=reinterpret_cast<uintptr_t>(nativeCulls.data());
}

#endif

using Work=std::array<uint8_t,0x230>;
SourcePreparedWorldHullStateCandidate initialState(unsigned mode){SourcePreparedWorldHullStateCandidate s;s.trace.fraction=mode%3==0?1.f:mode%3==1?.25f:0.f;s.trace.leftSolid=mode&2?.75f:0.f;s.trace.allSolid=(mode&8)!=0;s.trace.startSolid=(mode&4)!=0;s.trace.contents=0x12345678;s.trace.plane={9,8,7,6};s.trace.planeTags={5,4,3,2};s.trace.surface={SourceCollisionSurfaceKindCandidate::RuntimeSurface,77};s.displacementFlags=0xbeef;s.displacementHit=mode?0xffffffffu:0;s.surfaceProperty=0xabcd;s.surfaceFlags=0xef01;s.worldSurfaceIndexSource=SourceCollisionSurfaceCandidate{SourceCollisionSurfaceKindCandidate::RuntimeSurface,mode?77u:23u};return s;}
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


struct PolicyResult {Work work;std::vector<uint32_t> visits,culls,overlaps;uint32_t direct=UINT32_MAX;bool fallback=false;};
using Metadata=std::array<uint32_t,6>;
uint32_t orderHash(const std::vector<uint32_t>& ids){uint32_t result=2166136261u;for(auto id:ids)result=(result^id)*16777619u;return result;}
Metadata metadata(const PolicyResult& result){return {result.direct,uint32_t(result.fallback),uint32_t(result.culls.size()),uint32_t(result.overlaps.size()),orderHash(result.culls),orderHash(result.overlaps)};}
std::vector<uint32_t> initialVisits(const std::vector<SourceLeafDisplacementCullCandidate>& culls,uint32_t index,uint32_t next,unsigned mode){std::vector<uint32_t> visited(culls.size());if(mode==1)visited.at(culls.at(index).visitIndex)=17;if(mode==2)visited.at(culls.at(next).visitIndex)=17;return visited;}
PolicyResult ownedPolicy(const SourceBspBrushOnlyTraceCandidate& world,const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,const SourcePreparedHullWorldTablesCandidate& tables,const std::vector<SourceLeafDisplacementCullCandidate>& culls,const std::vector<std::array<uint16_t,2>>& properties,const std::vector<uint32_t>& ordered,const Vec& point,const Vec& extents,uint32_t mask,unsigned mode,unsigned visitMode,uint32_t index,uint32_t next,bool checkConvenience){
 PolicyResult result;result.work=initialWork(extents,mask,mode,visitMode==2);at<uint32_t>(result.work.data(),0x148)=uint32_t(world.data.brushes.size());at<uint32_t>(result.work.data(),0x170)=uint32_t(culls.size());std::memcpy(result.work.data(),point.data(),12);result.visits=initialVisits(culls,index,next,visitMode);auto state=initialState(mode);Vec minimum;for(size_t a=0;a<3;++a)minimum[a]=-extents[a];
 auto traceHull=[&](const auto& ray,auto& output){sourcePreparedWorldHullCandidate(world,meshes,tables.caches,tables.leafDisplacements,culls,properties,ray.start,ray.delta,extents,mask,output,[&](uint32_t brush){return excludedBrush(world,brush,0);},[&](const SourceCollisionSurfaceCandidate& surface){return details(surface,0);},false);};
 const auto outcome=sourceStationaryDisplacementHullPolicyCandidate(meshes,culls,ordered,result.visits,17,point,minimum,extents,mask,{0x3344,0},state,[&](uint32_t id,const auto& low,const auto& high){result.culls.push_back(id);return sourceStationaryDisplacementCullBoxCandidate(culls.at(id),low,high);},[&](uint32_t id,const auto& low,const auto& high){result.overlaps.push_back(id);return sourceDisplacementBoxOverlapCandidate(meshes.at(id),low,high);},traceHull);
 if(checkConvenience){auto alternate=initialState(mode);auto visits=initialVisits(culls,index,next,visitMode);const auto check=sourceStationaryDisplacementHullCandidate(meshes,culls,ordered,visits,17,point,minimum,extents,mask,{0x3344,0},alternate,traceHull);Work a{},b{};putState(a,state);putState(b,alternate);require(a==b&&visits==result.visits&&check.directMesh==outcome.directMesh&&bool(check.fallbackRay)==bool(outcome.fallbackRay),"public policy geometry binding");}
 result.direct=outcome.directMesh;result.fallback=bool(outcome.fallbackRay);if(outcome.fallbackRay){const auto& ray=*outcome.fallbackRay;std::memcpy(result.work.data(),ray.start.data(),12);std::memcpy(result.work.data()+0x3c,ray.delta.data(),12);std::memcpy(result.work.data()+0x48,ray.inverse.data(),12);std::memcpy(result.work.data()+0x108,ray.normal.data(),12);at<uint32_t>(result.work.data(),0x124)=1;}putState(result.work,state);return result;
}
#ifndef _WIN64
PolicyResult nativePolicy(NativeWorld& world,const std::vector<std::vector<uint32_t>>& refs,const std::vector<SourceLeafDisplacementCullCandidate>& culls,const std::vector<uint32_t>& ordered,const Vec& point,const Vec& extents,uint32_t mask,unsigned mode,unsigned visitMode,uint32_t index,uint32_t next){
 for(uint32_t i=0;i<world.surfaces.size();++i)world.surfaces[i]={0x10000000u+i,uint32_t(1000+i)|(uint32_t(materialFlags[i])<<16)};address<uint32_t>(0x10402fe8)=0xffffffffu;address<uint32_t>(0x10402fec)=0x3344;require(address<uint32_t>(0x106c1228)==0,"private memset feature state");
 for(size_t i=0;i<culls.size();++i){auto* c=nativeCulls[i].data();nativeCulls[i].fill(0);std::memcpy(c,culls[i].mins.data(),12);std::memcpy(c+16,culls[i].maxs.data(),12);at<uint32_t>(c,32)=culls[i].visitIndex;at<uint32_t>(c,36)=culls[i].contents;}
 const auto beforeCulls=nativeCulls;std::vector<uint16_t> dispRefs;for(size_t leaf=0;leaf<refs.size();++leaf){require(dispRefs.size()<65536&&refs[leaf].size()<65536,"policy world leaf span");world.leaves[leaf].dispFirst=uint16_t(dispRefs.size());world.leaves[leaf].dispCount=uint16_t(refs[leaf].size());for(auto id:refs[leaf])dispRefs.push_back(uint16_t(id));}at<uintptr_t>(world.fields.data(),0xb0)=reinterpret_cast<uintptr_t>(dispRefs.data());
 PolicyResult result;result.work=initialWork(extents,mask,mode,visitMode==2);at<uint32_t>(result.work.data(),0x148)=uint32_t(world.brushes.size());at<uint32_t>(result.work.data(),0x170)=uint32_t(culls.size());std::memcpy(result.work.data(),point.data(),12);result.visits=initialVisits(culls,index,next,visitMode);std::vector<uint32_t> parentBrush(world.brushes.size(),0xcafebabe),childBrush(world.brushes.size(),visitMode==2?17u:0u),childDisp(nativeObjects.size(),visitMode==2?17u:0u);auto* w=result.work.data();at<uintptr_t>(w,0x104)=reinterpret_cast<uintptr_t>(world.fields.data());at<uintptr_t>(w,0x128)=reinterpret_cast<uintptr_t>(parentBrush.data());at<uintptr_t>(w,0x13c)=reinterpret_cast<uintptr_t>(childBrush.data());at<uintptr_t>(w,0x150)=reinterpret_cast<uintptr_t>(result.visits.data());at<uintptr_t>(w,0x164)=reinterpret_cast<uintptr_t>(childDisp.data());std::vector<uint16_t> nativeRefs;for(auto id:ordered)nativeRefs.push_back(uint16_t(id));require(!nativeRefs.empty(),"original leaf requires nonempty displacement list");
 activeWork=reinterpret_cast<uintptr_t>(w);activeParentDisp=result.visits.data();openCache=-1;currentCandidate=selectedIndex=UINT32_MAX;acquires=releases=0;nativeCullOrder.clear();nativeOverlapOrder.clear();nativeDirectMesh=UINT32_MAX;nativeUsedFallback=false;auto* observer=PrivateX86BoundaryObserverFixture::active;require(observer!=nullptr,"active policy observer");observer->entries={0x1014e5d0,0x1014ce00,0x1015c760,0x10261300};observer->configure(true);
 using Dispatch=void(__cdecl*)(void*,const uint16_t*,int,const float*,const float*,const float*,uint32_t,void*);reinterpret_cast<Dispatch>(0x1014d320)(w,nativeRefs.data(),int(nativeRefs.size()),reinterpret_cast<float*>(w),reinterpret_cast<float*>(w+0x18),reinterpret_cast<float*>(w+0x24),mask,w+0x54);observer->configure(false);
 result.direct=nativeDirectMesh;result.fallback=nativeUsedFallback;result.culls=nativeCullOrder;result.overlaps=nativeOverlapOrder;require((result.direct!=UINT32_MAX)!=result.fallback,"exactly one stationary policy path");require(openCache==-1&&acquires==releases&&nativeCulls==beforeCulls,"policy cache/cull integrity");require(at<uint32_t>(w,0x11c)==0&&at<uint32_t>(w,0x120)==17&&at<uint32_t>(w,0x124)==(result.fallback?1u:visitMode==2?UINT32_MAX:0u),"policy nested generation/depth");for(auto mark:parentBrush)require(mark==0xcafebabe,"outer brush visits changed");for(auto mark:result.visits)require(mark==0||mark==17,"outer displacement generation");for(auto mark:childBrush)require(result.fallback?mark<=1:mark==(visitMode==2?17u:0u),"child brush storage lifecycle");for(auto mark:childDisp)require(mark==(result.fallback?0u:visitMode==2?17u:0u),"child nonswept displacement lifecycle");
 const auto name=at<uint32_t>(w,0x90);if(name==0x10350ee0u){require(selectedIndex<nativeObjects.size(),"policy fallback winning displacement identity");at<uint32_t>(w,0x90)=0x20000000u+selectedIndex;}else require(name==0xffffffffu||(name>=0x10000000u&&name<0x10000000u+materialFlags.size()),"policy semantic surface namespace");for(size_t offset:{size_t(0x104),size_t(0x128),size_t(0x13c),size_t(0x150),size_t(0x164)})at<uint32_t>(w,offset)=0;return result;
}
#endif

// Authored two-leaf world: a rear brush contains the start, while a front
// displacement is reached after the brush exit. The distant second mesh selects
// the fallback +X normal without overlapping the stationary query bounds.
std::vector<SourceDisplacementRuntimeMeshCandidate> controlledMeshes(){
 std::vector<SourceDisplacementRuntimeMeshCandidate> meshes;
 for(unsigned id=0;id<2;++id){SourceDisplacementRuntimeMeshCandidate m;m.power=1;m.contents=id?2u:1u;
  for(unsigned row=0;row<3;++row)for(unsigned column=0;column<3;++column){const float x=8.f*float(column)-8.f,y=8.f*float(row)-8.f;m.vertices.push_back(id?Vec{50,x,y}:Vec{4,y,x});}
  std::vector<std::array<uint16_t,3>> packed;
  for(const auto& grid:sourceDisplacementGridTrianglesCandidate(1,std::vector<uint16_t>(8))){SourceDisplacementCollisionTriangleCandidate triangle;triangle.packedVertices=grid.vertices;m.triangles.push_back(sourceDisplacementCollisionTriangleCandidate(m.vertices,triangle));packed.push_back(grid.vertices);}
  m.baseNormal=m.triangles.front().normal;m.tree=sourceDisplacementCollisionTreeCandidate(1,m.vertices,packed);meshes.push_back(std::move(m));
 }
 require(meshes[0].baseNormal[0]==-1.f&&meshes[1].baseNormal[0]==1.f,"controlled mesh winding");return meshes;
}
void controlledWorld(SourceBspBrushOnlyTraceCandidate& world){
 world.data.planes.clear();world.data.sides.clear();
 const Vec low{-4,-8,-8},high{-1,8,8};
 for(uint16_t axis=0;axis<3;++axis)for(uint16_t side=0;side<2;++side){Vec normal{};normal[axis]=side?-1.f:1.f;const float distance=side?-low[axis]:high[axis];world.data.planes.push_back(sourceBspCollisionPlaneCandidate(normal,distance,side?3:axis));world.data.sides.push_back({uint16_t(2*axis+side),int16_t(2*axis+side),-1,0});}
 world.data.brushes={{0,6,1}};world.data.leafBrushes={{},{0}};world.data.leafContents={0,0};world.data.worldBrushes={0};world.data.worldNodes=1;world.data.displacementRecords=2;
 world.records=sourceBspCollisionRecordsCandidate(world.data,materialMapping);world.planes={{{1,0,0},0,0}};world.nodes={{0,{-1,-2}}};world.root=0;world.modelRoots={0};
}


struct LeafResult {PolicyResult displacement;std::vector<uint32_t> brushVisits,brushes;bool dispatched=false;};
std::vector<uint32_t> initialBrushVisits(size_t count,const std::vector<uint32_t>& ordered,unsigned mode){std::vector<uint32_t> visits(count);if(!ordered.empty()&&mode)visits.at(mode==1?ordered.front():ordered.back())=17;return visits;}
void storeFallbackRay(Work& work,const SourceStationaryHullFallbackRayCandidate& ray){std::memcpy(work.data(),ray.start.data(),12);std::memcpy(work.data()+0x3c,ray.delta.data(),12);std::memcpy(work.data()+0x48,ray.inverse.data(),12);std::memcpy(work.data()+0x108,ray.normal.data(),12);at<uint32_t>(work.data(),0x124)=1;}
LeafResult ownedLeaf(const SourceBspBrushOnlyTraceCandidate& world,const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,const SourcePreparedHullWorldTablesCandidate& tables,const std::vector<std::array<uint16_t,2>>& properties,const std::vector<uint32_t>& brushes,const std::vector<uint32_t>& displacements,const Vec& point,const Vec& extents,uint32_t mask,unsigned mode,unsigned visitMode,uint32_t index,uint32_t next,bool checkConvenience){
 LeafResult result;auto& d=result.displacement;d.work=initialWork(extents,mask,mode,visitMode==2);at<uint32_t>(d.work.data(),0x148)=uint32_t(world.data.brushes.size());at<uint32_t>(d.work.data(),0x170)=uint32_t(meshes.size());std::memcpy(d.work.data(),point.data(),12);const auto beforeWork=d.work;d.visits=initialVisits(tables.culls,index,next,visitMode);result.brushVisits=initialBrushVisits(world.data.brushes.size(),brushes,visitMode);auto state=initialState(mode);Vec minimum;for(size_t a=0;a<3;++a)minimum[a]=-extents[a];std::vector<uint32_t> contents;for(const auto& brush:world.data.brushes)contents.push_back(brush.contents);
 auto traceHull=[&](const auto& ray,auto& output){sourcePreparedWorldHullCandidate(world,meshes,tables.caches,tables.leafDisplacements,tables.culls,properties,ray.start,ray.delta,extents,mask,output,[&](uint32_t brush){return excludedBrush(world,brush,0);},[&](const SourceCollisionSurfaceCandidate& surface){return details(surface,0);},false);};
 sourceStationaryHullLeafPolicyCandidate(brushes,contents,result.brushVisits,17,mask,!displacements.empty(),state,
  [&](uint32_t brush,auto& output){result.brushes.push_back(brush);sourceStationaryBrushHullCandidate(world.data,world.records,brush,point,minimum,extents,output.trace);},
  [&](auto& output){result.dispatched=true;const auto outcome=sourceStationaryDisplacementHullPolicyCandidate(meshes,tables.culls,displacements,d.visits,17,point,minimum,extents,mask,{0x3344,0},output,[&](uint32_t id,const auto& low,const auto& high){d.culls.push_back(id);return sourceStationaryDisplacementCullBoxCandidate(tables.culls.at(id),low,high);},[&](uint32_t id,const auto& low,const auto& high){d.overlaps.push_back(id);return sourceDisplacementBoxOverlapCandidate(meshes.at(id),low,high);},traceHull);d.direct=outcome.directMesh;d.fallback=bool(outcome.fallbackRay);if(outcome.fallbackRay)storeFallbackRay(d.work,*outcome.fallbackRay);});
 putState(d.work,state);
 if(checkConvenience){auto alternate=initialState(mode);auto brushMarks=initialBrushVisits(world.data.brushes.size(),brushes,visitMode),dispMarks=initialVisits(tables.culls,index,next,visitMode);auto alternateWork=beforeWork;sourceStationaryHullLeafCandidate(world,meshes,tables.culls,brushes,displacements,brushMarks,dispMarks,17,point,minimum,extents,mask,{0x3344,0},alternate,[&](const auto& ray,auto& output){storeFallbackRay(alternateWork,ray);traceHull(ray,output);});putState(alternateWork,alternate);require(alternateWork==d.work&&brushMarks==result.brushVisits&&dispMarks==d.visits,"stationary leaf complete geometry binding");}
 return result;
}
#ifndef _WIN64
LeafResult nativeLeaf(NativeWorld& world,const std::vector<std::vector<uint32_t>>& refs,const std::vector<SourceLeafDisplacementCullCandidate>& culls,const std::vector<uint32_t>& brushes,const std::vector<uint32_t>& displacements,const Vec& point,const Vec& extents,uint32_t mask,unsigned mode,unsigned visitMode,uint32_t index,uint32_t next){
 for(uint32_t i=0;i<world.surfaces.size();++i)world.surfaces[i]={0x10000000u+i,uint32_t(1000+i)|(uint32_t(materialFlags[i])<<16)};address<uint32_t>(0x10402fe8)=0xffffffffu;address<uint32_t>(0x10402fec)=0x3344;require(address<uint32_t>(0x106c1228)==0,"leaf private memset feature");
 const auto beforeCulls=nativeCulls;const uint32_t leafIndex=uint32_t(refs.size());if(world.leaves.size()==leafIndex)world.leaves.push_back({});require(world.leaves.size()==size_t(leafIndex)+1,"extra leaf record coverage");world.refs.resize(world.baseRefCount);require(world.refs.size()+brushes.size()<65536,"leaf brush span");auto& leaf=world.leaves.back();leaf.first=uint16_t(world.refs.size());leaf.count=uint16_t(brushes.size());for(auto id:brushes)world.refs.push_back(uint16_t(id));at<uintptr_t>(world.fields.data(),0x88)=reinterpret_cast<uintptr_t>(world.leaves.data());at<uintptr_t>(world.fields.data(),0x98)=reinterpret_cast<uintptr_t>(world.refs.data());
 std::vector<uint16_t> dispRefs;for(size_t i=0;i<refs.size();++i){world.leaves[i].dispFirst=uint16_t(dispRefs.size());world.leaves[i].dispCount=uint16_t(refs[i].size());for(auto id:refs[i])dispRefs.push_back(uint16_t(id));}require(dispRefs.size()+displacements.size()<65536,"leaf displacement span");leaf.dispFirst=uint16_t(dispRefs.size());leaf.dispCount=uint16_t(displacements.size());for(auto id:displacements)dispRefs.push_back(uint16_t(id));at<uintptr_t>(world.fields.data(),0xb0)=reinterpret_cast<uintptr_t>(dispRefs.data());
 LeafResult result;auto& d=result.displacement;d.work=initialWork(extents,mask,mode,visitMode==2);auto* w=d.work.data();at<uint32_t>(w,0x148)=uint32_t(world.brushes.size());at<uint32_t>(w,0x170)=uint32_t(culls.size());std::memcpy(w,point.data(),12);d.visits=initialVisits(culls,index,next,visitMode);result.brushVisits=initialBrushVisits(world.brushes.size(),brushes,visitMode);std::vector<uint32_t> childBrush(world.brushes.size(),visitMode==2?17u:0u),childDisp(culls.size(),visitMode==2?17u:0u);
 at<uintptr_t>(w,0x104)=reinterpret_cast<uintptr_t>(world.fields.data());at<uintptr_t>(w,0x128)=reinterpret_cast<uintptr_t>(result.brushVisits.data());at<uintptr_t>(w,0x13c)=reinterpret_cast<uintptr_t>(childBrush.data());at<uintptr_t>(w,0x150)=reinterpret_cast<uintptr_t>(d.visits.data());at<uintptr_t>(w,0x164)=reinterpret_cast<uintptr_t>(childDisp.data());activeWork=reinterpret_cast<uintptr_t>(w);activeParentDisp=d.visits.data();activeParentBrush=result.brushVisits.data();activeBrushBase=reinterpret_cast<uintptr_t>(world.brushes.data());activeBrushCount=world.brushes.size();
 nativeBrushOrder.clear();nativeLeafDispatched=false;nativeCullOrder.clear();nativeOverlapOrder.clear();nativeDirectMesh=UINT32_MAX;nativeUsedFallback=false;openCache=-1;currentCandidate=selectedIndex=UINT32_MAX;acquires=releases=0;auto* observer=PrivateX86BoundaryObserverFixture::active;observer->entries={0x10146c30,0x1014d320,0x1015c760,0x10261300};observer->configure(true);const uintptr_t target=0x10147f40;
 __asm { mov ecx,w
         mov eax,leafIndex
         call target }
 observer->configure(false);d.direct=nativeDirectMesh;d.fallback=nativeUsedFallback;d.culls=nativeCullOrder;d.overlaps=nativeOverlapOrder;result.brushes=nativeBrushOrder;result.dispatched=nativeLeafDispatched;
 require(openCache==-1&&acquires==releases&&nativeCulls==beforeCulls,"stationary leaf cache integrity");require(at<uint32_t>(w,0x11c)==0&&at<uint32_t>(w,0x120)==17&&at<uint32_t>(w,0x124)==(d.fallback?1u:visitMode==2?UINT32_MAX:0u),"leaf depth/generation");for(auto mark:result.brushVisits)require(mark==0||mark==17,"leaf brush visits");for(auto mark:d.visits)require(mark==0||mark==17,"leaf displacement visits");for(auto mark:childBrush)require(d.fallback?mark<=1:mark==(visitMode==2?17u:0u),"leaf child brush visits");for(auto mark:childDisp)require(mark==(d.fallback?0u:visitMode==2?17u:0u),"leaf child displacement visits");
 const auto name=at<uint32_t>(w,0x90);if(name==0x10350ee0u){require(selectedIndex<nativeObjects.size(),"leaf fallback terrain identity");at<uint32_t>(w,0x90)=0x20000000u+selectedIndex;}else require(name==0xffffffffu||(name>=0x10000000u&&name<0x10000000u+materialFlags.size()),"leaf surface namespace");for(size_t offset:{size_t(0x104),size_t(0x128),size_t(0x13c),size_t(0x150),size_t(0x164)})at<uint32_t>(w,offset)=0;return result;
}
#endif
using LeafMetadata=std::array<uint32_t,9>;
LeafMetadata leafMetadata(const LeafResult& r){const auto m=metadata(r.displacement);return {uint32_t(r.brushes.size()),orderHash(r.brushes),uint32_t(r.dispatched),m[0],m[1],m[2],m[3],m[4],m[5]};}
void writeVisits(std::ofstream& out,const std::vector<uint32_t>& visits){std::vector<uint8_t> bits((visits.size()+7)/8);for(size_t i=0;i<visits.size();++i){require(visits[i]==0||visits[i]==17,"visit bit packing");if(visits[i])bits[i/8]|=uint8_t(1u<<(i%8));}out.write(reinterpret_cast<const char*>(bits.data()),bits.size());}
uint32_t pointLeaf(const SourceBspBrushOnlyTraceCandidate& world,const Vec& point){int32_t node=world.root;while(node>=0){const auto& n=world.nodes.at(size_t(node));const auto& p=world.planes.at(n.plane);const float xy=point[0]*p.normal[0]+point[1]*p.normal[1],distance=xy+point[2]*p.normal[2]-p.distance;node=n.children[distance>=0?0:1];}return uint32_t(-1-int64_t(node));}

int main(int argc,char** argv){try{
 require(argc==5,"expected engine or -, BSP, private permutation and disposable output");const auto bsp=read(argv[2]);auto word=[&](size_t offset){uint32_t value;require(offset<=bsp.size()&&bsp.size()-offset>=4,"BSP word");std::memcpy(&value,bsp.data()+offset,4);return value;};const auto texinfo=word(8+6*16),texinfoBytes=word(8+6*16+4),texdataBytes=word(8+2*16+4);materialFlags.resize(texdataBytes/32);for(uint32_t i=0;i<texinfoBytes/72;++i){const auto id=word(texinfo+72*i+68);materialMapping.push_back(id);materialFlags.at(id)|=uint16_t(word(texinfo+72*i+64));}boundFlags=std::make_unique<SourceBspTraceSurfaceFlagsCandidate>(bsp,materialMapping);SourceBspBrushOnlyTraceCandidate world(bsp,materialMapping);require(world.data.brushes.size()==1866,"leaf brush inventory");
 std::array<uint32_t,256> raw{};std::ifstream fixture(argv[3],std::ios::binary);fixture.read(reinterpret_cast<char*>(raw.data()),sizeof(raw));require(fixture.gcount()==sizeof(raw)&&fixture.peek()==std::char_traits<char>::eof(),"permutation fixture");SourcePlaneHashPermutationCandidate permutation{};for(size_t i=0;i<raw.size();++i){require(raw[i]<256,"permutation range");permutation[i]=uint8_t(raw[i]);}
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1],0x643eec);image.seal();require(std::memcmp(raw.data(),reinterpret_cast<void*>(0x104129b0),1024)==0,"permutation identity");PrivateX86BoundaryObserverFixture observer{{0x1015b150,0x10001610,0x1015c760,0x10261300},boundary};
#endif
 std::ofstream out(argv[4],std::ios::binary);require(bool(out),"leaf output");std::array<uint32_t,8> header{0x534c4632,0,0,560,1866,43,3,2};out.write(reinterpret_cast<const char*>(header.data()),sizeof(header));unsigned cases=0,brushCalls=0,dispatched=0,direct=0,fallbacks=0,solid=0,fallbackSolid=0,zeroMasks=0,premarked=0,emptyBrushDispatch=0,initialZeroDispatch=0,surfaceBypass=0,convenience=0,nativeAcquires=0;
 auto record=[&](const auto& meshes,const auto& tables,const auto& properties,const auto& brushes,const auto& displacements,const Vec& point,const Vec& extents,uint32_t mask,unsigned mode,unsigned visit,uint32_t index,uint32_t next,
#ifndef _WIN64
 NativeWorld& nativeWorld,
#endif
 bool controlled){const bool check=cases%31==0;const auto expected=ownedLeaf(world,meshes,tables,properties,brushes,displacements,point,extents,mask,mode,visit,index,next,check);auto actual=expected;convenience+=check;
#ifndef _WIN64
 actual=nativeLeaf(nativeWorld,tables.leafDisplacements,tables.culls,brushes,displacements,point,extents,mask,mode,visit,index,next);if(actual.displacement.work!=expected.displacement.work||actual.displacement.visits!=expected.displacement.visits||actual.brushVisits!=expected.brushVisits||actual.brushes!=expected.brushes||actual.displacement.culls!=expected.displacement.culls||actual.displacement.overlaps!=expected.displacement.overlaps||leafMetadata(actual)!=leafMetadata(expected)){std::cerr<<"leaf case "<<cases<<" controlled "<<controlled<<" point "<<point[0]<<' '<<point[1]<<' '<<point[2]<<" hull "<<extents[0]<<" mask "<<mask<<" mode "<<mode<<" visit "<<visit<<'\n';for(size_t i=0;i<560;++i)if(actual.displacement.work[i]!=expected.displacement.work[i])std::cerr<<"work byte "<<i<<" native "<<unsigned(actual.displacement.work[i])<<" owned "<<unsigned(expected.displacement.work[i])<<'\n';throw std::runtime_error("complete stationary leaf mismatch");}nativeAcquires+=acquires;
#endif
 const auto& d=actual.displacement;const std::array<uint32_t,2> inputCounts{uint32_t(brushes.size()),uint32_t(displacements.size())};out.write(reinterpret_cast<const char*>(inputCounts.data()),sizeof(inputCounts));for(const auto* ids:{&brushes,&displacements})out.write(reinterpret_cast<const char*>(ids->data()),std::streamsize(ids->size()*4));out.write(reinterpret_cast<const char*>(d.work.data()),560);writeVisits(out,actual.brushVisits);writeVisits(out,d.visits);const auto m=leafMetadata(actual);out.write(reinterpret_cast<const char*>(m.data()),sizeof(m));require(bool(out),"leaf record write");++cases;brushCalls+=unsigned(actual.brushes.size());dispatched+=actual.dispatched;direct+=d.direct!=UINT32_MAX;fallbacks+=d.fallback;solid+=d.work[0x8b]!=0;fallbackSolid+=d.fallback&&d.work[0x8b]!=0;zeroMasks+=mask==0;premarked+=visit!=0;emptyBrushDispatch+=brushes.empty()&&actual.dispatched;initialZeroDispatch+=initialState(mode).trace.fraction==0.f&&actual.dispatched;surfaceBypass+=controlled&&mask==0x2080&&at<uint32_t>(d.work.data(),0x84)==0x80&&std::find(actual.brushes.begin(),actual.brushes.end(),1u)!=actual.brushes.end();};
 for(unsigned hdr=0;hdr<2;++hdr){const SourceBspDisplacementCollisionCandidate imported(bsp,int32_t(hdr));const auto& meshes=imported.meshes;const auto tables=sourcePreparedHullWorldTablesCandidate(world,meshes,permutation,64,true);std::vector<std::array<uint16_t,2>> properties;for(size_t i=0;i<meshes.size();++i)properties.push_back({uint16_t(100+i),uint16_t(500+i)});
#ifndef _WIN64
 queryPhase=false;observer.entries={0x1015b150,0x10001610,0x1015c760,0x10261300};observer.configure(true);prepareObjects(meshes,tables.caches,properties,tables.culls,64,true);observer.configure(false);queryPhase=true;NativeWorld nativeWorld(world);
#endif
 for(uint32_t index=0;index<meshes.size();++index){const auto next=(index+1)%uint32_t(meshes.size());const auto& selectedMesh=meshes[index];const auto& b=selectedMesh.tree.expandedBounds;const auto& triangle=selectedMesh.triangles[index%selectedMesh.triangles.size()];Vec centroid{};for(size_t a=0;a<3;++a)centroid[a]=(selectedMesh.vertices[triangle.packedVertices[0]&511u][a]+selectedMesh.vertices[triangle.packedVertices[1]&511u][a]+selectedMesh.vertices[triangle.packedVertices[2]&511u][a])/3.f;
 for(unsigned q=0;q<3;++q){Vec point{(b[0]+b[3])*.5f,(b[1]+b[4])*.5f,(b[2]+b[5])*.5f};for(size_t a=0;a<3;++a)point[a]+=(q?-.25f:64.f)*selectedMesh.baseNormal[a];if(q==2)point=centroid;const auto leaf=pointLeaf(world,point);
 for(unsigned hull=0;hull<2;++hull)for(unsigned lists=0;lists<4;++lists)for(uint32_t mask:{0u,1u,0x2080u,0xffffffffu})for(unsigned mode:{0u,1u,2u,4u,9u})for(unsigned visit=0;visit<3;++visit){auto brushes=world.data.leafBrushes.at(leaf),displacements=tables.leafDisplacements.at(leaf);if(lists==1){brushes.clear();displacements={index,index,next};}if(lists==2){if(!brushes.empty())brushes.push_back(brushes.front());displacements={next,index};}if(lists==3){brushes={index%1866,(index+17)%1866,index%1866};displacements.clear();}record(meshes,tables,properties,brushes,displacements,point,hull?Vec{24,24,41}:Vec{2,3,5},mask,mode,visit,index,next,
#ifndef _WIN64
 nativeWorld,
#endif
 false);}
 }}
#ifndef _WIN64
 for(size_t i=0;i<meshes.size();++i){require(storedIndices[i]==tables.caches[i].indices,"leaf cache indices unchanged");require(std::memcmp(storedPlanes[i].data(),tables.caches[i].planes.data(),storedPlanes[i].size()*12)==0,"leaf cache planes unchanged");}
#endif
 }
 const unsigned benchmarkCases=cases;controlledWorld(world);world.data.brushes.push_back({0,6,0x80});world.data.brushes.push_back({0,6,0});world.data.leafBrushes={{},{0,1,2}};world.data.worldBrushes={0,1,2};int16_t excludedTexture=-1;for(size_t i=0;i<materialMapping.size();++i)if(materialFlags.at(materialMapping[i])&0x80){excludedTexture=int16_t(i);break;}require(excludedTexture>=0,"actual surface-exclusion flag fixture");for(auto& side:world.data.sides)side.texture=excludedTexture;world.records=sourceBspCollisionRecordsCandidate(world.data,materialMapping);require(excludedBrush(world,1,0),"controlled swept surface exclusion would apply");auto meshes=controlledMeshes();const auto tables=sourcePreparedHullWorldTablesCandidate(world,meshes,permutation,64,true);const std::vector<std::array<uint16_t,2>> properties{{100,500},{101,501}};
#ifndef _WIN64
 queryPhase=false;observer.entries={0x1015b150,0x10001610,0x1015c760,0x10261300};observer.configure(true);prepareObjects(meshes,tables.caches,properties,tables.culls,64,true);observer.configure(false);queryPhase=true;NativeWorld nativeWorld(world);
#endif
 const std::vector<std::vector<uint32_t>> brushLists{{},{0},{1},{2,0,1,0},{1,0,1},{2,2}},dispLists{{},{1,0},{0,1,0}};
 for(float x:{-3.f,-2.f,-6.f,4.f,0.f})for(unsigned hull=0;hull<2;++hull)for(const auto& brushes:brushLists)for(const auto& displacements:dispLists)for(uint32_t mask:{0u,1u,0x2080u,0xffffffffu})for(unsigned mode:{0u,1u,2u,4u,9u})for(unsigned visit=0;visit<3;++visit)for(unsigned disabled=0;disabled<2;++disabled){meshes[0].runtimeFlags=disabled?4u:0u;
#ifndef _WIN64
 at<uint32_t>(nativeObjects[0].data(),0x2c)=meshes[0].runtimeFlags;
#endif
 record(meshes,tables,properties,brushes,displacements,Vec{x,0,0},hull?Vec{2,3,5}:Vec{1,1,1},mask,mode,visit,0,1,
#ifndef _WIN64
 nativeWorld,
#endif
 true);}
#ifndef _WIN64
 for(size_t i=0;i<meshes.size();++i){require(storedIndices[i]==tables.caches[i].indices,"controlled leaf cache indices unchanged");require(std::memcmp(storedPlanes[i].data(),tables.caches[i].planes.data(),storedPlanes[i].size()*12)==0,"controlled leaf cache planes unchanged");}image.verify();
#endif
 header[1]=benchmarkCases;header[2]=cases-benchmarkCases;out.seekp(0);out.write(reinterpret_cast<const char*>(header.data()),sizeof(header));out.flush();require(bool(out),"leaf output finish");require(brushCalls&&dispatched&&direct&&fallbacks&&fallbackSolid&&emptyBrushDispatch&&initialZeroDispatch&&surfaceBypass,"stationary leaf coverage");std::cout<<"{\"cases\":"<<cases<<",\"benchmark_cases\":"<<benchmarkCases<<",\"controlled_cases\":"<<cases-benchmarkCases<<",\"brush_calls\":"<<brushCalls<<",\"displacement_dispatches\":"<<dispatched<<",\"direct_overlaps\":"<<direct<<",\"fallbacks\":"<<fallbacks<<",\"start_solid\":"<<solid<<",\"fallback_solid\":"<<fallbackSolid<<",\"zero_masks\":"<<zeroMasks<<",\"premarked_queries\":"<<premarked<<",\"empty_brush_dispatch\":"<<emptyBrushDispatch<<",\"initial_zero_dispatch\":"<<initialZeroDispatch<<",\"surface_exclusion_bypasses\":"<<surfaceBypass<<",\"public_binding_checks\":"<<convenience<<",\"native_cache_acquires\":"<<nativeAcquires<<",\"passed\":true}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

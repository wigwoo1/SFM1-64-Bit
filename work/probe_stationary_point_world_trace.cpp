#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef near
#undef far
#include <array>
#include <algorithm>
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
#include "source_prepared_point_world_candidate.h"
#include "source_stationary_hull_world_trace_candidate.h"
#include "source_bsp_displacement_collision_candidate.h"
#include "source_bsp_trace_surface_flags_candidate.h"
#include "source_displacement_grid_triangles_candidate.h"

using Vec=std::array<float,3>;
void require(bool v,const char* s){if(!v)throw std::runtime_error(s);}
uint32_t bits(float f){uint32_t u;std::memcpy(&u,&f,4);return u;}
template<class T>T& at(uint8_t* p,size_t n){return *reinterpret_cast<T*>(p+n);}
template<class T>T at(const uint8_t* p,size_t n){T value;std::memcpy(&value,p+n,sizeof(value));return value;}
std::vector<uint8_t> read(const char* p){std::ifstream f(p,std::ios::binary);require(bool(f),"BSP read");return {(std::istreambuf_iterator<char>(f)),{}};}
std::vector<uint32_t> materialMapping;std::vector<uint16_t> materialFlags;std::unique_ptr<SourceBspTraceSurfaceFlagsCandidate> boundFlags;
bool excludedBrush(const SourceBspBrushOnlyTraceCandidate& world,uint32_t index,unsigned){const auto& r=world.records.at(index);if(r.optimized){for(unsigned a=0;a<6;++a){uint16_t id;std::memcpy(&id,r.box.data()+32+2*a,2);if(id!=65535&&(materialFlags.at(id)&0x80))return true;}}else{const auto& brush=world.data.brushes.at(index);for(uint32_t a=0;a<brush.count;++a){const auto texture=world.data.sides.at(brush.first+a).texture;if(texture>=0&&(boundFlags->flags({SourceCollisionSurfaceKindCandidate::TextureInfo,uint32_t(texture)})&0x80))return true;}}return false;}
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
template<class T>T& address(uintptr_t p){return *reinterpret_cast<T*>(p);}
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
  for(const auto& row:w.data.leafBrushes){require(refs.size()<65536&&row.size()<65536,"leaf span");leaves.push_back({w.data.leafContents.at(leaves.size()),0,uint16_t(refs.size()),uint16_t(row.size()),0,0});for(auto i:row){require(i<65536,"brush reference");refs.push_back(uint16_t(i));}}
  const auto p=reinterpret_cast<uintptr_t>(fields.data());address<uintptr_t>(p)=reinterpret_cast<uintptr_t>(nodes.data());address<uintptr_t>(p+0x68)=reinterpret_cast<uintptr_t>(sides.data());address<uintptr_t>(p+0x70)=reinterpret_cast<uintptr_t>(boxes.data());address<uintptr_t>(p+0x88)=reinterpret_cast<uintptr_t>(leaves.data());address<uintptr_t>(p+0x98)=reinterpret_cast<uintptr_t>(refs.data());address<uintptr_t>(p+0xa8)=reinterpret_cast<uintptr_t>(brushes.data());address<uintptr_t>(p+0x218)=reinterpret_cast<uintptr_t>(surfaces.data());
 }
};
#endif
std::array<uint16_t,2> details(const SourceCollisionSurfaceCandidate& surface,unsigned){const auto id=boundFlags->identity(surface);if(id==SourceBspTraceSurfaceFlagsCandidate::defaultId)return {0x3344,0};require(id<materialFlags.size(),"bound brush material identity");return {uint16_t(1000+id),materialFlags[id]};}
using Record=std::array<uint8_t,88>;
Record pack(const SourceSweptHullWorldTraceCandidate& value){Record r{};auto* t=r.data();const auto& s=value.state;std::memcpy(t,value.startPosition.data(),12);std::memcpy(t+12,value.endPosition.data(),12);std::memcpy(t+0x18,s.trace.plane.data(),16);std::memcpy(t+0x28,s.trace.planeTags.data(),4);at<float>(t,0x2c)=s.trace.fraction;at<uint32_t>(t,0x30)=s.trace.contents;at<uint16_t>(t,0x34)=s.displacementFlags;t[0x36]=uint8_t(s.trace.allSolid);t[0x37]=uint8_t(s.trace.startSolid);at<float>(t,0x38)=s.trace.leftSolid;const auto kind=s.trace.surface.kind;at<uint32_t>(t,0x3c)=kind==SourceCollisionSurfaceKindCandidate::Default?0xffffffffu:kind==SourceCollisionSurfaceKindCandidate::Displacement?0x20000000u+s.trace.surface.index:0x10000000u+boundFlags->identity(s.trace.surface);at<uint16_t>(t,0x40)=s.surfaceProperty;at<uint16_t>(t,0x42)=s.surfaceFlags;at<uint16_t>(t,0x4a)=value.worldSurfaceIndex;at<uint32_t>(t,0x54)=s.displacementHit;return r;}
using Work=std::array<uint8_t,0x230>;
SourcePreparedWorldHullStateCandidate initialState(unsigned mode){SourcePreparedWorldHullStateCandidate s;s.trace.fraction=mode%3==0?1.f:mode%3==1?.25f:0.f;s.trace.leftSolid=mode&2?.75f:0.f;s.trace.allSolid=(mode&8)!=0;s.trace.startSolid=(mode&4)!=0;s.trace.contents=0x12345678;s.trace.plane={9,8,7,6};s.trace.planeTags={5,4,3,2};s.trace.surface={SourceCollisionSurfaceKindCandidate::RuntimeSurface,77};s.displacementFlags=0xbeef;s.displacementHit=mode?0xffffffffu:0;s.surfaceProperty=0xabcd;s.surfaceFlags=0xef01;s.worldSurfaceIndexSource=SourceCollisionSurfaceCandidate{SourceCollisionSurfaceKindCandidate::RuntimeSurface,mode?77u:23u};return s;}
void putState(Work& work,const SourcePreparedWorldHullStateCandidate& state){SourceSweptHullWorldTraceCandidate trace;trace.state=state;trace.startPosition={11,12,13};trace.endPosition={14,15,16};if(state.worldSurfaceIndexSource){const auto id=boundFlags->identity(*state.worldSurfaceIndexSource);trace.worldSurfaceIndex=id==UINT32_MAX?65535:uint16_t(id);}const auto record=pack(trace);std::memcpy(work.data()+0x54,record.data(),84);at<uint32_t>(work.data(),0x114)=state.displacementHit;at<uint32_t>(work.data(),0x98)=0x11112222;at<uint16_t>(work.data(),0x9c)=0x3344;at<uint32_t>(work.data(),0xa0)=0x55556666;at<uint32_t>(work.data(),0xa4)=0x77778888;}
Work initialWork(const Vec& extents,uint32_t mask,unsigned mode,bool wrap){Work work;work.fill(0xa7);const Vec start{91,92,93},end{-91,-92,-93};std::memcpy(work.data(),start.data(),12);std::memcpy(work.data()+12,end.data(),12);Vec minimum;for(size_t a=0;a<3;++a)minimum[a]=-extents[a];std::memcpy(work.data()+0x18,minimum.data(),12);std::memcpy(work.data()+0x24,extents.data(),12);std::memcpy(work.data()+0x30,extents.data(),12);at<uint32_t>(work.data(),0xfc)=mask;work[0x100]=work[0x101]=0;at<uint32_t>(work.data(),0x104)=0;at<uint32_t>(work.data(),0x11c)=0;at<uint32_t>(work.data(),0x120)=17;at<uint32_t>(work.data(),0x124)=wrap?UINT32_MAX:0;for(size_t offset:{size_t(0x128),size_t(0x13c),size_t(0x150),size_t(0x164)})at<uint32_t>(work.data(),offset)=0;at<uint32_t>(work.data(),0x148)=1866;at<uint32_t>(work.data(),0x170)=43;putState(work,initialState(mode));return work;}
struct PointTables {std::vector<std::vector<uint32_t>> refs;std::vector<SourceLeafDisplacementCullCandidate> culls;std::vector<std::array<uint16_t,2>> properties;};
PointTables pointTables(const SourceBspBrushOnlyTraceCandidate& world,const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes){PointTables tables;tables.refs.resize(world.data.leafBrushes.size());for(size_t i=0;i<meshes.size();++i){const auto& b=meshes[i].tree.expandedBounds;tables.culls.push_back({{b[0],b[1],b[2]},{b[3],b[4],b[5]},meshes[i].contents,uint32_t(i)});tables.properties.push_back({uint16_t(100+i),uint16_t(500+i)});for(auto leaf:sourceDisplacementLeafAssignmentCandidate(world,{b[0],b[1],b[2]},{b[3],b[4],b[5]}))tables.refs.at(leaf).push_back(uint32_t(i));}return tables;}

#ifndef _WIN64
std::vector<std::array<uint8_t,244>> pointObjects;std::vector<std::array<uint8_t,48>> pointCulls;
// Original point tree uses object+98 for its logical internal-node count.
void pointObjectsPrepare(const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,const PointTables& tables){pointObjects.resize(meshes.size());pointCulls.resize(meshes.size());for(size_t i=0;i<meshes.size();++i){const auto& m=meshes[i];auto* object=pointObjects[i].data();pointObjects[i].fill(0);at<uint32_t>(object,0x20)=m.contents;at<uint32_t>(object,0x28)=m.power;at<uint32_t>(object,0x2c)=m.runtimeFlags;std::memcpy(object+4,m.tree.expandedBounds.data(),12);std::memcpy(object+0x14,m.tree.expandedBounds.data()+3,12);std::memcpy(object+0x60,m.baseNormal.data(),12);std::memcpy(object+0x6c,tables.properties[i].data(),4);at<uintptr_t>(object,0x70)=reinterpret_cast<uintptr_t>(m.vertices.data());at<uintptr_t>(object,0x80)=reinterpret_cast<uintptr_t>(m.triangles.data());at<uintptr_t>(object,0x90)=reinterpret_cast<uintptr_t>(m.tree.nodes.data());at<uintptr_t>(object,0xa0)=reinterpret_cast<uintptr_t>(m.tree.leafPairs.data());for(auto row:{std::array<uint32_t,2>{0x70,uint32_t(m.vertices.size())},std::array<uint32_t,2>{0x80,uint32_t(m.triangles.size())},std::array<uint32_t,2>{0x90,uint32_t(m.tree.nodes.size())},std::array<uint32_t,2>{0xa0,uint32_t(m.tree.leafPairs.size())}}){at<uint32_t>(object,row[0]+4)=row[1];at<uint32_t>(object,row[0]+8)=row[1];at<uint32_t>(object,row[0]+12)=row[1];}auto* c=pointCulls[i].data();pointCulls[i].fill(0);std::memcpy(c,tables.culls[i].mins.data(),12);std::memcpy(c+16,tables.culls[i].maxs.data(),12);at<uint32_t>(c,32)=tables.culls[i].visitIndex;at<uint32_t>(c,36)=tables.culls[i].contents;}address<uintptr_t>(0x10637820)=reinterpret_cast<uintptr_t>(pointObjects.data());address<uintptr_t>(0x10637824)=reinterpret_cast<uintptr_t>(pointCulls.data());}

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


Record stationaryOwned(const SourceBspBrushOnlyTraceCandidate& world,const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,const SourcePreparedHullWorldTablesCandidate& tables,const std::vector<std::array<uint16_t,2>>& properties,const SourceServerHullRayCandidate& ray,uint32_t mask,bool compute,bool available){return pack(sourceStationaryHullWorldTraceCandidate(world,meshes,tables,properties,ray,mask,{0x3344,0},compute,available,[&](uint32_t index){return excludedBrush(world,index,0);},[&](const SourceCollisionSurfaceCandidate& surface){return details(surface,0);},[&](const SourceCollisionSurfaceCandidate& surface){return boundFlags->identity(surface);}));}

#ifndef _WIN64

uintptr_t activeWork=0,activeInputRay=0;const SourceServerHullRayCandidate* activeRay=nullptr;uint32_t activeMask=0;unsigned poolAcquires=0,poolReleases=0,setupObservations=0,fallbackCallsThisQuery=0,pointObservations=0,finishObservations=0;
void slot(unsigned index,CONTEXT& c,uint32_t entry){auto* observer=PrivateX86BoundaryObserverFixture::active;observer->entries[index]=entry;if(index==0)c.Dr0=entry;else if(index==2)c.Dr2=entry;else throw std::runtime_error("public point observer slot");c.ContextFlags|=CONTEXT_DEBUG_REGISTERS;}
void boundary(unsigned index,CONTEXT& c){using O=PrivateX86BoundaryObserverFixture;auto* work=reinterpret_cast<uint8_t*>(activeWork);
 if(index==0&&c.Eip==0x10149230){require(++poolAcquires==1&&address<uint32_t>(c.Esp)==0x10149ebb,"public point work acquire");O::returnCdecl(c,uint32_t(activeWork));slot(0,c,0x10148030);}
 else if(index==0&&c.Eip==0x10148030){require(c.Esi==activeWork&&activeRay&&address<uint32_t>(c.Esp)==0x1014a09b&&address<uintptr_t>(c.Esp+4)==activeInputRay&&address<int32_t>(c.Esp+8)==0,"public stationary point geometry route");const auto& ray=*activeRay;for(size_t a=0;a<3;++a){const float end=a==0?ray.center[a]+ray.delta[a]:ray.delta[a]+ray.center[a],negative=ray.extents[a]*-1.f,inverse=ray.delta[a]==0.f?std::numeric_limits<float>::max():1.f/ray.delta[a];require(std::memcmp(work+4*a,&ray.center[a],4)==0&&std::memcmp(work+12+4*a,&end,4)==0&&std::memcmp(work+0x18+4*a,&negative,4)==0&&std::memcmp(work+0x24+4*a,&ray.extents[a],4)==0&&std::memcmp(work+0x30+4*a,&ray.extents[a],4)==0&&std::memcmp(work+0x3c+4*a,&ray.delta[a],4)==0&&std::memcmp(work+0x48+4*a,&inverse,4)==0,"public point setup field bits");}require(at<uint32_t>(work,0xfc)==activeMask&&work[0x100]==1&&!work[0x101]&&at<uint32_t>(work,0x114)==0,"public point flags/marker reset");for(size_t a=0;a<3;++a)require(at<uint32_t>(work,0x108+4*a)==0,"public point normal reset");SourceSweptHullWorldTraceCandidate reset;reset.state.surfaceProperty=0x3344;const auto trace=pack(reset);require(std::memcmp(work+0x54,trace.data(),84)==0,"public point complete trace reset");++setupObservations;slot(0,c,0x1014ce00);c.EFlags|=0x10000u;}
 else if(index==0&&c.Eip==0x1014ce00){require(address<uintptr_t>(c.Esp+4)==activeWork,"public point fallback owner");++fallbackCallsThisQuery;c.EFlags|=0x10000u;}
 else if(index==1){require(c.Eip==0x1000e020&&c.Ecx==0x10637798&&address<uintptr_t>(c.Esp+4)==activeWork-4&&address<int32_t>(activeWork+0x11c)==-1&&++poolReleases==1,"public point work release");O::returnCdecl(c);c.Esp+=4;slot(0,c,0x10149230);}
 else if(index==2&&c.Eip==0x10148950){require(c.Ecx==activeWork&&c.Edx==0&&address<uintptr_t>(c.Esp+12)==activeWork&&address<uintptr_t>(c.Esp+16)==activeWork+12&&work[0x100]==1&&!work[0x101]&&at<uint32_t>(work,0x11c)==1&&at<uint32_t>(work,0x124)==fallbackCallsThisQuery,"public nested point geometry/generation");++pointObservations;slot(2,c,0x1014cf70);c.EFlags|=0x10000u;}
 else if(index==2&&c.Eip==0x1014cf70){require(at<uint32_t>(work,0x11c)==1,"point depth before original finish");slot(2,c,0x10148950);c.EFlags|=0x10000u;}
 else if(index==3){require(c.Eip==0x10145e20&&c.Ecx==activeWork+0x54&&c.Edx==activeInputRay&&address<uint32_t>(c.Esp)==0x1014a0d4,"original public coordinate finisher");for(size_t a=0;a<6;++a)require(at<uint32_t>(work,0x54+4*a)==0,"collision altered position output before finishing");++finishObservations;c.EFlags|=0x10000u;}
 else throw std::runtime_error("unexpected public stationary point boundary");
}

std::array<uint8_t,80> stationaryRayBytes(const SourceServerHullRayCandidate& ray){std::array<uint8_t,80> bytes{};std::memcpy(bytes.data(),ray.center.data(),12);std::memcpy(bytes.data()+0x10,ray.delta.data(),12);std::memcpy(bytes.data()+0x20,ray.offset.data(),12);std::memcpy(bytes.data()+0x30,ray.extents.data(),12);bytes[0x44]=uint8_t(ray.isRay);bytes[0x45]=uint8_t(ray.swept);return bytes;}
Record stationaryNative(NativeWorld& world,const std::vector<std::vector<uint32_t>>& refs,const SourceServerHullRayCandidate& ray,uint32_t mask,bool compute,bool available,bool wrap){
 for(uint32_t i=0;i<world.surfaces.size();++i)world.surfaces[i]={0x10000000u+i,uint32_t(1000+i)|(uint32_t(materialFlags[i])<<16)};address<uint32_t>(0x10402fe8)=0xffffffffu;address<uint32_t>(0x10402fec)=0x3344;require(address<uint32_t>(0x106c1228)==0,"stationary wrapper memset feature");for(size_t i=0;i<world.leaves.size();++i)world.leaves[i].contents=world.source.data.leafContents.at(i);
 std::vector<uint16_t> dispRefs;for(size_t leaf=0;leaf<refs.size();++leaf){require(dispRefs.size()<65536&&refs[leaf].size()<65536,"public stationary leaf span");world.leaves[leaf].dispFirst=uint16_t(dispRefs.size());world.leaves[leaf].dispCount=uint16_t(refs[leaf].size());for(auto id:refs[leaf])dispRefs.push_back(uint16_t(id));}at<uintptr_t>(world.fields.data(),0xb0)=reinterpret_cast<uintptr_t>(dispRefs.data());at<uint32_t>(world.fields.data(),0x7c)=available?uint32_t(world.source.data.planes.size()):0;
 alignas(16)std::array<uint8_t,0x250> storage;storage.fill(0xa7);auto* work=storage.data()+16;std::vector<uint32_t> brushVisits(world.brushes.size()),dispVisits(pointObjects.size()),childBrush(world.brushes.size(),wrap?17u:0u),childDisp(pointObjects.size(),wrap?17u:0u);at<uintptr_t>(work,0x104)=reinterpret_cast<uintptr_t>(world.fields.data());at<uint32_t>(work,0x114)=0;at<uint32_t>(work,0x11c)=0;at<uint32_t>(work,0x120)=1;at<uint32_t>(work,0x124)=wrap?UINT32_MAX:0;at<uintptr_t>(work,0x128)=reinterpret_cast<uintptr_t>(brushVisits.data());at<uintptr_t>(work,0x13c)=reinterpret_cast<uintptr_t>(childBrush.data());at<uint32_t>(work,0x148)=uint32_t(world.brushes.size());at<uintptr_t>(work,0x150)=reinterpret_cast<uintptr_t>(dispVisits.data());at<uintptr_t>(work,0x164)=reinterpret_cast<uintptr_t>(childDisp.data());at<uint32_t>(work,0x170)=uint32_t(pointObjects.size());
 alignas(16)auto input=stationaryRayBytes(ray);const auto before=input;std::array<uint8_t,116> output;output.fill(0xa5);activeWork=reinterpret_cast<uintptr_t>(work);activeInputRay=reinterpret_cast<uintptr_t>(input.data());activeRay=&ray;activeMask=mask;poolAcquires=poolReleases=0;fallbackCallsThisQuery=0;const auto priorSetups=setupObservations,priorPoints=pointObservations,priorFinishes=finishObservations;
 using Wrapper=void(__cdecl*)(const void*,int32_t,uint32_t,bool,void*);reinterpret_cast<Wrapper>(0x10149eb0)(input.data(),world.source.root,mask,compute,output.data()+16);require(poolAcquires==1&&poolReleases==1,"public stationary service balance");require(setupObservations-priorSetups==unsigned(available),"available-world setup gate");require(pointObservations-priorPoints==fallbackCallsThisQuery&&finishObservations-priorFinishes==unsigned(available&&compute),"public point nested/coordinate observation counts");require(input==before,"public wrapper changed Ray_t");for(size_t i=0;i<16;++i)require(output[i]==0xa5&&output[100+i]==0xa5&&storage[i]==0xa7&&storage[0x240+i]==0xa7,"public stationary output/work guards");require(at<uint32_t>(work,0x120)==1&&at<int32_t>(work,0x11c)==-1&&at<uint32_t>(work,0x124)==(fallbackCallsThisQuery?fallbackCallsThisQuery:wrap?UINT32_MAX:0u),"public stationary generation/depth finish");for(auto mark:brushVisits)require(mark<=1,"public parent brush marks");for(auto mark:dispVisits)require(mark<=1,"public parent displacement marks");for(auto mark:childBrush)require(fallbackCallsThisQuery?mark<=fallbackCallsThisQuery:mark==(wrap?17u:0u),"public child brush generations");for(auto mark:childDisp)require(mark==(fallbackCallsThisQuery?0u:wrap?17u:0u),"public child displacement marks");
 Record result{};std::memcpy(result.data(),output.data()+16,84);const auto name=at<uint32_t>(result.data(),0x3c);if(name==0x10350ee0u){const auto property=at<uint16_t>(result.data(),0x40);const uint32_t id=property>=500?uint32_t(property-500):uint32_t(property-100);require(id<pointObjects.size(),"public terrain fixture identity");at<uint32_t>(result.data(),0x3c)=0x20000000u+id;}else require(name==0xffffffffu||(name>=0x10000000u&&name<0x10000000u+materialFlags.size()),"public stationary surface namespace");at<uint32_t>(result.data(),84)=at<uint32_t>(work,0x114);return result;
}

#endif
void prepareControlledWorld(SourceBspBrushOnlyTraceCandidate& world,bool comb){controlledWorld(world);world.data.brushes.push_back({0,6,0x80});world.data.brushes.push_back({0,6,0});int16_t texture=-1;for(size_t i=0;i<materialMapping.size();++i)if(materialFlags.at(materialMapping[i])&0x80){texture=int16_t(i);break;}require(texture>=0,"point stationary surface fixture");for(auto& side:world.data.sides)side.texture=texture;world.records=sourceBspCollisionRecordsCandidate(world.data,materialMapping);world.data.worldBrushes={0,1,2};world.data.leafBrushes={{},{0,1,2,1}};if(!comb)return;constexpr size_t count=1030;world.nodes.clear();world.data.leafBrushes.assign(count,{});world.data.leafBrushes[0]={0,1,2,1};world.data.leafContents.assign(count,0);world.data.worldNodes=count-1;for(size_t i=0;i<count-1;++i)world.nodes.push_back({0,{i+2==count?-int32_t(count):int32_t(i+1),-int32_t(i+1)}});}
void leafContents(SourceBspBrushOnlyTraceCandidate& world,unsigned mode){for(size_t i=0;i<world.data.leafContents.size();++i)world.data.leafContents[i]=mode==0?0u:mode==1?1u:mode==2?(i+1==world.data.leafContents.size()?0u:1u):(i==0?0x81u:0x80u);}

SourceServerHullRayCandidate stationaryQuery(const Vec& point,unsigned variant,unsigned movement){
 Vec low{},high{},start=point;
 if(variant==1){low={-.000732421875f,-.000244140625f,0};high={.000732421875f,.000244140625f,0};}
 if(variant==2){high={.00048828125f,.0009765625f,.000244140625f};for(size_t a=0;a<3;++a)start[a]=point[a]-(low[a]+high[a])*.5f;}
 if(variant==3){low=high=point;start={};}
 auto end=start;if(movement==1)end[1]+=1e-30f;if(movement==2)end[0]-=1e-23f;if(movement==3)end[2]+=1e-40f;const auto ray=sourceServerHullRayCandidate(start,end,low,high);require(!ray.swept&&ray.isRay,"stationary constructed point flags");return ray;
}
int main(int argc,char** argv){try{
 require(argc==4,"expected engine or -, BSP and disposable output");const auto bsp=read(argv[2]);auto word=[&](size_t offset){uint32_t value;require(offset<=bsp.size()&&bsp.size()-offset>=4,"BSP word");std::memcpy(&value,bsp.data()+offset,4);return value;};const auto texinfo=word(8+6*16),texinfoBytes=word(8+6*16+4),texdataBytes=word(8+2*16+4);materialFlags.resize(texdataBytes/32);for(uint32_t i=0;i<texinfoBytes/72;++i){const auto id=word(texinfo+72*i+68);materialMapping.push_back(id);materialFlags.at(id)|=uint16_t(word(texinfo+72*i+64));}boundFlags=std::make_unique<SourceBspTraceSurfaceFlagsCandidate>(bsp,materialMapping);SourceBspBrushOnlyTraceCandidate world(bsp,materialMapping);require(world.root==0&&world.data.brushes.size()==1866,"public stationary inventory");
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1],0x643eec);image.seal();PrivateX86BoundaryObserverFixture observer{{0x10149230,0x1000e020,0x10148950,0x10145e20},boundary};
#endif
 std::ofstream out(argv[3],std::ios::binary);require(bool(out),"public stationary output");std::array<uint32_t,8> header{0x53505431,0,0,0,0,160,0,0};out.write(reinterpret_cast<const char*>(header.data()),sizeof(header));unsigned cases=0,hits=0,startSolid=0,allSolid=0,empty=0,positionsOff=0,underflow=0,offsets=0,infiniteInverse=0,wrapped=0,zeroMasks=0,negativeZeroPositions=0,nativeSetups=0,nativeFallbackTotal=0;
 auto record=[&](const auto& meshes,const auto& tables,const auto& properties,const SourceServerHullRayCandidate& ray,uint32_t mask,bool compute,bool available,bool wrap,
#ifndef _WIN64
 NativeWorld& nativeWorld,
#endif
 unsigned phase){require(tables.caches.empty(),"point trace must not require hull cache/permutation");const auto expected=stationaryOwned(world,meshes,tables,properties,ray,mask,compute,available);auto actual=expected;
#ifndef _WIN64
 const auto prior=setupObservations;actual=stationaryNative(nativeWorld,tables.leafDisplacements,ray,mask,compute,available,wrap);nativeSetups+=setupObservations-prior;nativeFallbackTotal+=fallbackCallsThisQuery;if(actual!=expected){std::cerr<<"public stationary case "<<cases<<" phase "<<phase<<" center "<<ray.center[0]<<' '<<ray.center[1]<<' '<<ray.center[2]<<" mask "<<mask<<" compute "<<compute<<" available "<<available<<" wrap "<<wrap<<'\n';for(size_t i=0;i<actual.size();++i)if(actual[i]!=expected[i])std::cerr<<"byte "<<i<<" native "<<unsigned(actual[i])<<" owned "<<unsigned(expected[i])<<'\n';throw std::runtime_error("full public stationary trace mismatch");}
#endif
 std::array<uint32_t,18> input{};for(size_t a=0;a<3;++a){input[a]=bits(ray.center[a]);input[3+a]=bits(ray.delta[a]);input[6+a]=bits(ray.offset[a]);input[9+a]=bits(ray.extents[a]);}input[12]=uint32_t(ray.isRay)|(uint32_t(ray.swept)<<1);input[13]=mask;input[14]=compute;input[15]=available;input[16]=wrap;input[17]=phase;out.write(reinterpret_cast<const char*>(input.data()),sizeof(input));out.write(reinterpret_cast<const char*>(actual.data()),actual.size());require(bool(out),"public stationary record");++cases;hits+=at<float>(actual.data(),0x2c)<1;startSolid+=actual[0x37]!=0;allSolid+=actual[0x36]!=0;empty+=!available;positionsOff+=!compute;underflow+=!ray.swept&&ray.delta!=Vec{};offsets+=ray.offset!=Vec{};bool infinite=false;for(float value:ray.delta)if(value!=0.f&&!std::isfinite(1.f/value))infinite=true;infiniteInverse+=infinite;wrapped+=wrap;zeroMasks+=mask==0;bool negativeZero=false;for(size_t a=0;a<6;++a)negativeZero|=at<uint32_t>(actual.data(),4*a)==0x80000000u;negativeZeroPositions+=negativeZero;
 };
 for(unsigned hdr=0;hdr<2;++hdr){const SourceBspDisplacementCollisionCandidate imported(bsp,int32_t(hdr));const auto& meshes=imported.meshes;auto prepared=pointTables(world,meshes);SourcePreparedHullWorldTablesCandidate tables;tables.leafDisplacements=prepared.refs;tables.culls=prepared.culls;std::vector<std::array<uint16_t,2>> properties;for(size_t i=0;i<meshes.size();++i)properties.push_back({uint16_t(100+i),uint16_t(500+i)});
#ifndef _WIN64
 pointObjectsPrepare(meshes,prepared);observer.entries={0x10149230,0x1000e020,0x10148950,0x10145e20};observer.configure(true);NativeWorld nativeWorld(world);
#endif
 for(size_t index=0;index<meshes.size();++index){const auto& m=meshes[index];const auto& b=m.tree.expandedBounds;const auto& t=m.triangles[index%m.triangles.size()];Vec centroid{};for(size_t a=0;a<3;++a)centroid[a]=(m.vertices[t.packedVertices[0]&511u][a]+m.vertices[t.packedVertices[1]&511u][a]+m.vertices[t.packedVertices[2]&511u][a])/3.f;
 for(unsigned q=0;q<3;++q){Vec point{(b[0]+b[3])*.5f,(b[1]+b[4])*.5f,(b[2]+b[5])*.5f};for(size_t a=0;a<3;++a)point[a]+=(q?-.25f:64.f)*m.baseNormal[a];if(q==2)point=centroid;
 for(unsigned variant=0;variant<4;++variant)for(unsigned movement=0;movement<4;++movement)for(uint32_t mask:{0u,1u,0x2080u,0xffffffffu})for(bool compute:{false,true})for(bool available:{true,false})for(bool wrap:{false,true}){if(!available&&q!=0)continue;record(meshes,tables,properties,stationaryQuery(point,variant,movement),mask,compute,available,wrap,
#ifndef _WIN64
 nativeWorld,
#endif
 0);}
 }}
#ifndef _WIN64
 observer.configure(false);
#endif
 }
 header[2]=cases;
 for(unsigned topology=0;topology<2;++topology){prepareControlledWorld(world,false);if(topology)world.planes[0]={{.6f,.8f,0},0,3};const auto meshes=controlledMeshes();auto prepared=pointTables(world,meshes);std::reverse(prepared.refs[0].begin(),prepared.refs[0].end());SourcePreparedHullWorldTablesCandidate tables;tables.leafDisplacements=prepared.refs;tables.culls=prepared.culls;const std::vector<std::array<uint16_t,2>> properties{{100,500},{101,501}};
#ifndef _WIN64
 pointObjectsPrepare(meshes,prepared);observer.entries={0x10149230,0x1000e020,0x10148950,0x10145e20};observer.configure(true);NativeWorld nativeWorld(world);
#endif
 for(unsigned solidity=0;solidity<4;++solidity){leafContents(world,solidity);for(float x:{-20.f,-6.f,-3.f,-2.f,-1.f,0.f,1.f,2.f,3.f,4.f,50.f})for(unsigned variant=0;variant<4;++variant)for(unsigned movement=0;movement<4;++movement)for(uint32_t mask:{0u,1u,0x2080u,0xffffffffu})for(bool compute:{false,true})for(bool available:{true,false})for(bool wrap:{false,true})record(meshes,tables,properties,stationaryQuery(Vec{x,0,0},variant,movement),mask,compute,available,wrap,
#ifndef _WIN64
 nativeWorld,
#endif
 topology+1);}
 if(topology==1)for(unsigned flags=0;flags<4;++flags)for(bool compute:{false,true}){auto ray=stationaryQuery(Vec{},0,0);ray.isRay=(flags&1)!=0;ray.swept=(flags&2)!=0;record(meshes,tables,properties,ray,0xffffffffu,compute,false,false,
#ifndef _WIN64
 nativeWorld,
#endif
 3);++header[4];}
if(topology==1)for(unsigned variant=0;variant<4;++variant)for(uint32_t mask:{0u,0xffffffffu})for(bool compute:{false,true})for(bool available:{true,false})for(bool wrap:{false,true}){SourceServerHullRayCandidate ray;ray.isRay=true;ray.extents=variant&1?Vec{.000732421875f,.000244140625f,0}:Vec{};if(variant==0){ray.center={-0.f,-0.f,0};ray.offset={-0.f,0,-0.f};ray.delta={-0.f,0,-0.f};}if(variant==1){ray.center={100000000,-100000000,17};ray.offset={-100000000,100000000,-17};ray.delta={-1e-30f,0,1e-40f};}if(variant==2){ray.center={-2,3,-4};ray.offset={.5f,-.25f,1};ray.delta={-0.f,0,-0.f};}if(variant==3){ray.center={.03125f,-.03125f,2};ray.offset={-.0625f,.0625f,-2};ray.delta={0,1e-30f,0};}record(meshes,tables,properties,ray,mask,compute,available,wrap,
#ifndef _WIN64
 nativeWorld,
#endif
 4);++header[6];}
#ifndef _WIN64
 observer.configure(false);
#endif
 }
 header[1]=cases;header[3]=cases-header[2]-header[4]-header[6];out.seekp(0);out.write(reinterpret_cast<const char*>(header.data()),sizeof(header));out.flush();require(bool(out),"public stationary output finish");
#ifndef _WIN64
 image.verify();
#endif
 require(hits&&startSolid&&allSolid&&empty&&positionsOff&&underflow&&offsets&&infiniteInverse&&wrapped&&negativeZeroPositions,"public stationary coverage");std::cout<<"{\"cases\":"<<cases<<",\"benchmark_cases\":"<<header[2]<<",\"controlled_cases\":"<<header[3]<<",\"empty_flag_cases\":"<<header[4]<<",\"provided_ray_cases\":"<<header[6]<<",\"negative_zero_positions\":"<<negativeZeroPositions<<",\"hits\":"<<hits<<",\"start_solid\":"<<startSolid<<",\"all_solid\":"<<allSolid<<",\"empty_worlds\":"<<empty<<",\"positions_disabled\":"<<positionsOff<<",\"nonzero_stationary_delta\":"<<underflow<<",\"nonzero_offsets\":"<<offsets<<",\"overflowed_setup_inverse\":"<<infiniteInverse<<",\"wrapped_child_generation\":"<<wrapped<<",\"zero_masks\":"<<zeroMasks<<",\"native_setup_observations\":"<<nativeSetups<<",\"native_fallback_calls\":"<<nativeFallbackTotal;
#ifndef _WIN64
 std::cout<<",\"native_point_observations\":"<<pointObservations<<",\"native_finish_observations\":"<<finishObservations;
#endif
 std::cout<<",\"passed\":true}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

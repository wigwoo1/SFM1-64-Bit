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
#include "source_stationary_displacement_hull_policy_candidate.h"
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


struct PolicyResult {Work work{};std::vector<uint32_t> visits,childBrush,childDisp,culls,overlaps;Vec normal{};uint32_t normalContents=0,direct=UINT32_MAX;bool fallback=false;};
std::vector<uint32_t> initialVisits(const PointTables& tables,uint32_t index,unsigned mode){std::vector<uint32_t> visits(tables.culls.size());if(mode)visits[tables.culls[(index+(mode==2?1u:0u))%tables.culls.size()].visitIndex]=17;return visits;}
Work policyWork(const Vec& point,const Vec& extents,uint32_t mask,unsigned mode,unsigned visit,size_t brushes,size_t displacements){auto work=initialWork(extents,mask,mode,visit==2);std::memcpy(work.data(),point.data(),12);work[0x100]=1;at<uint32_t>(work.data(),0x148)=uint32_t(brushes);at<uint32_t>(work.data(),0x170)=uint32_t(displacements);return work;}
PolicyResult ownedPolicy(const SourceBspBrushOnlyTraceCandidate& world,const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,const PointTables& tables,const std::vector<uint32_t>& ordered,const Vec& point,const Vec& extents,uint32_t mask,unsigned mode,unsigned visit,uint32_t index,bool checkBinding){PolicyResult result;result.work=policyWork(point,extents,mask,mode,visit,world.data.brushes.size(),meshes.size());result.visits=initialVisits(tables,index,visit);result.childBrush.assign(world.data.brushes.size(),visit==2?17u:0u);result.childDisp.assign(meshes.size(),visit==2?17u:0u);auto state=initialState(mode);Vec minimum{};for(size_t a=0;a<3;++a)minimum[a]=-extents[a];
 auto tracePoint=[&](const auto& ray,auto& output){result.childBrush.assign(world.data.brushes.size(),0);result.childDisp.assign(meshes.size(),0);sourcePreparedPointWorldCandidate(world,meshes,tables.refs,tables.culls,tables.properties,world.root,ray.start,ray.delta,extents,mask,result.childBrush,result.childDisp,1,output,[&](uint32_t brush){return excludedBrush(world,brush,0);},[&](const SourceCollisionSurfaceCandidate& surface){return details(surface,0);},false);};
 const auto outcome=sourceStationaryDisplacementHullPolicyCandidate(meshes,tables.culls,ordered,result.visits,17,point,minimum,extents,mask,{0x3344,0},state,[&](uint32_t id,const auto& low,const auto& high){result.culls.push_back(id);return sourceStationaryDisplacementCullBoxCandidate(tables.culls.at(id),low,high);},[&](uint32_t id,const auto& low,const auto& high){result.overlaps.push_back(id);return sourceDisplacementBoxOverlapCandidate(meshes.at(id),low,high);},tracePoint,true);
 if(checkBinding){auto alternate=initialState(mode);auto visits=initialVisits(tables,index,visit);auto childBrush=result.childBrush,childDisp=result.childDisp;const auto check=sourceStationaryDisplacementHullCandidate(meshes,tables.culls,ordered,visits,17,point,minimum,extents,mask,{0x3344,0},alternate,tracePoint,true);Work a{},b{};putState(a,state);putState(b,alternate);require(a==b&&visits==result.visits&&check.directMesh==outcome.directMesh&&bool(check.fallbackRay)==bool(outcome.fallbackRay)&&childBrush==result.childBrush&&childDisp==result.childDisp,"point policy convenience binding");}
 result.direct=outcome.directMesh;result.fallback=bool(outcome.fallbackRay);if(outcome.fallbackRay){const auto& ray=*outcome.fallbackRay;result.normal=ray.normal;sourceStationaryDisplacementHullNormalCandidate(meshes,ordered,point,minimum,extents,true,mask,result.normal,result.normalContents);std::memcpy(result.work.data(),ray.start.data(),12);std::memcpy(result.work.data()+0x3c,ray.delta.data(),12);std::memcpy(result.work.data()+0x48,ray.inverse.data(),12);std::memcpy(result.work.data()+0x108,ray.normal.data(),12);at<uint32_t>(result.work.data(),0x124)=1;}putState(result.work,state);return result;
}
#ifndef _WIN64
uintptr_t activeWork=0,normalOutput=0,normalContentsOutput=0;uint32_t* activeVisits=nullptr;PolicyResult nativeObserved;unsigned routeObservations=0,pointObservations=0;bool activeWrap=false;Vec priorEnd{};
void setPolicySlot(CONTEXT& c,unsigned slot,uint32_t entry){auto* observer=PrivateX86BoundaryObserverFixture::active;observer->entries[slot]=entry;if(slot==0)c.Dr0=entry;else if(slot==1)c.Dr1=entry;else if(slot==2)c.Dr2=entry;else if(slot==3)c.Dr3=entry;else throw std::runtime_error("policy observer slot");c.ContextFlags|=CONTEXT_DEBUG_REGISTERS;}
void policyBoundary(unsigned slot,CONTEXT& c){auto* work=reinterpret_cast<uint8_t*>(activeWork);
 if(slot==0&&c.Eip==0x1014e5d0){require(address<uint32_t>(c.Esp)==0x1014d44f,"point policy bounds caller");const uintptr_t p=address<uintptr_t>(c.Esp+12),base=reinterpret_cast<uintptr_t>(pointCulls.data());require(p>=base&&(p-base)%48==0&&(p-base)/48<pointCulls.size()&&address<uintptr_t>(c.Esp+16)==p+16,"point policy bounds identity");const auto id=uint32_t((p-base)/48);require(activeVisits[address<uint32_t>(p+32)]==17&&(address<uint32_t>(p+36)&at<uint32_t>(work,0xfc)),"point policy mark before cull");nativeObserved.culls.push_back(id);setPolicySlot(c,0,0x1014d44f);}
 else if(slot==0&&c.Eip==0x1014d44f)setPolicySlot(c,0,0x1014e5d0);
 else if(slot==1&&c.Eip==0x1015a7d0){const auto base=reinterpret_cast<uintptr_t>(pointObjects.data());require(c.Ecx>=base&&(c.Ecx-base)%244==0&&(c.Ecx-base)/244<pointObjects.size()&&address<uint32_t>(c.Esp)==0x1014d471,"point policy overlap identity");nativeObserved.overlaps.push_back(uint32_t((c.Ecx-base)/244));setPolicySlot(c,1,0x1014d471);}
 else if(slot==1&&c.Eip==0x1014d471){if(c.Eax&255u)nativeObserved.direct=nativeObserved.overlaps.back();setPolicySlot(c,1,0x1015a7d0);}
 else if(slot==2&&c.Eip==0x1014cd40){require(address<uintptr_t>(c.Esp+4)==activeWork&&address<uint32_t>(c.Esp)==0x1014d498&&work[0x100]==1&&work[0x101]==0,"point normal caller/flags");normalOutput=address<uintptr_t>(c.Esp+16);normalContentsOutput=address<uintptr_t>(c.Esp+24);setPolicySlot(c,2,0x1014d498);}
 else if(slot==2&&c.Eip==0x1014d498){std::memcpy(nativeObserved.normal.data(),reinterpret_cast<void*>(normalOutput),12);nativeObserved.normalContents=address<uint32_t>(normalContentsOutput);setPolicySlot(c,2,0x1014cd40);}
 else if(slot==3&&c.Eip==0x1014ce00){require(address<uintptr_t>(c.Esp+4)==activeWork&&address<uint32_t>(c.Esp)==0x1014d4a7,"point fallback caller");nativeObserved.fallback=true;std::memcpy(priorEnd.data(),work+12,12);require(std::memcmp(nativeObserved.normal.data(),reinterpret_cast<void*>(address<uintptr_t>(c.Esp+12)),12)==0,"selected fallback normal");setPolicySlot(c,3,0x10149030);}
 else if(slot==3&&c.Eip==0x10149030){require(c.Ecx==activeWork&&c.Edx==0&&work[0x100]==1&&work[0x101]==0&&address<float>(c.Esp+4)==0.f&&address<float>(c.Esp+8)==1.f,"point fallback routing");require(at<uint32_t>(work,0x11c)==1&&at<uint32_t>(work,0x120)==17&&at<uint32_t>(work,0x124)==1,"point child generation/depth");++routeObservations;setPolicySlot(c,3,0x10148950);}
 else if(slot==3&&c.Eip==0x10148950){require(c.Ecx==activeWork&&c.Edx==0&&address<uintptr_t>(c.Esp+12)==activeWork&&address<uintptr_t>(c.Esp+16)==activeWork+12,"actual nested point geometry");++pointObservations;setPolicySlot(c,3,0x1014d4a7);}
 else if(slot==3&&c.Eip==0x1014d4a7){require(std::memcmp(priorEnd.data(),work+12,12)==0&&at<uint32_t>(work,0x11c)==0,"fallback end/depth restoration");setPolicySlot(c,3,0x1014ce00);}
 else throw std::runtime_error("unexpected stationary point boundary");
 c.EFlags|=0x10000u;
}
void bindRefs(NativeWorld& world,const PointTables& tables,std::vector<uint16_t>& refs){refs.clear();for(size_t i=0;i<tables.refs.size();++i){require(refs.size()+tables.refs[i].size()<65536,"point policy refs span");world.leaves[i].dispFirst=uint16_t(refs.size());world.leaves[i].dispCount=uint16_t(tables.refs[i].size());for(auto id:tables.refs[i])refs.push_back(uint16_t(id));}at<uintptr_t>(world.fields.data(),0xb0)=reinterpret_cast<uintptr_t>(refs.data());}
PolicyResult nativePolicy(NativeWorld& world,const PointTables& tables,const std::vector<uint32_t>& ordered,const Vec& point,const Vec& extents,uint32_t mask,unsigned mode,unsigned visit,uint32_t index){for(uint32_t i=0;i<world.surfaces.size();++i)world.surfaces[i]={0x10000000u+i,uint32_t(1000+i)|(uint32_t(materialFlags[i])<<16)};address<uint32_t>(0x10402fe8)=UINT32_MAX;address<uint32_t>(0x10402fec)=0x3344;for(size_t i=0;i<tables.culls.size();++i){auto* c=pointCulls[i].data();std::memcpy(c,tables.culls[i].mins.data(),12);std::memcpy(c+16,tables.culls[i].maxs.data(),12);at<uint32_t>(c,32)=tables.culls[i].visitIndex;at<uint32_t>(c,36)=tables.culls[i].contents;}
 PolicyResult result;result.work=policyWork(point,extents,mask,mode,visit,world.brushes.size(),pointObjects.size());result.visits=initialVisits(tables,index,visit);result.childBrush.assign(world.brushes.size(),visit==2?17u:0u);result.childDisp.assign(pointObjects.size(),visit==2?17u:0u);std::vector<uint32_t> parentBrush(world.brushes.size(),0xcafebabe);std::vector<uint16_t> refs;for(auto id:ordered)refs.push_back(uint16_t(id));auto* work=result.work.data();at<uintptr_t>(work,0x104)=reinterpret_cast<uintptr_t>(world.fields.data());at<uintptr_t>(work,0x128)=reinterpret_cast<uintptr_t>(parentBrush.data());at<uintptr_t>(work,0x13c)=reinterpret_cast<uintptr_t>(result.childBrush.data());at<uintptr_t>(work,0x150)=reinterpret_cast<uintptr_t>(result.visits.data());at<uintptr_t>(work,0x164)=reinterpret_cast<uintptr_t>(result.childDisp.data());activeWork=reinterpret_cast<uintptr_t>(work);activeVisits=result.visits.data();activeWrap=visit==2;nativeObserved={};const auto objects=pointObjects;const auto culls=pointCulls;auto* observer=PrivateX86BoundaryObserverFixture::active;observer->entries={0x1014e5d0,0x1015a7d0,0x1014cd40,0x1014ce00};observer->configure(true);
 using Policy=void(__cdecl*)(void*,const uint16_t*,int,const float*,const float*,const float*,uint32_t,void*);reinterpret_cast<Policy>(0x1014d320)(work,refs.data(),int(refs.size()),reinterpret_cast<float*>(work),reinterpret_cast<float*>(work+0x18),reinterpret_cast<float*>(work+0x24),mask,work+0x54);observer->configure(false);
 result.culls=nativeObserved.culls;result.overlaps=nativeObserved.overlaps;result.direct=nativeObserved.direct;result.fallback=nativeObserved.fallback;result.normal=nativeObserved.normal;result.normalContents=nativeObserved.normalContents;require((result.direct!=UINT32_MAX)!=result.fallback&&pointObjects==objects&&pointCulls==culls,"point policy route/geometry preservation");for(auto mark:parentBrush)require(mark==0xcafebabe,"point parent brush preservation");require(at<uint32_t>(work,0x11c)==0&&at<uint32_t>(work,0x120)==17&&at<uint32_t>(work,0x124)==(result.fallback?1u:visit==2?UINT32_MAX:0u),"point policy transaction finish");
 const auto name=at<uint32_t>(work,0x90);if(name==0x10350ee0u){const auto property=at<uint16_t>(work,0x94);const uint32_t id=property>=500?uint32_t(property-500):uint32_t(property-100);require(id<pointObjects.size(),"point policy surface ordinal");at<uint32_t>(work,0x90)=0x20000000u+id;}else require(name==UINT32_MAX||(name>=0x10000000u&&name<0x10000000u+materialFlags.size()),"point policy surface namespace");for(size_t at:{size_t(0x104),size_t(0x128),size_t(0x13c),size_t(0x150),size_t(0x164)})std::memset(work+at,0,4);return result;
}
#endif
void writeMarks(std::ofstream& out,const std::vector<uint32_t>& values,uint32_t generation){std::vector<uint8_t> packed((values.size()+7)/8);for(size_t i=0;i<values.size();++i){require(values[i]==0||values[i]==generation,"point policy packed visits");if(values[i])packed[i/8]|=uint8_t(1u<<(i%8));}out.write(reinterpret_cast<const char*>(packed.data()),packed.size());}
int main(int argc,char** argv){try{require(argc==4,"expected engine or -, BSP and disposable output");const auto bsp=read(argv[2]);auto word=[&](size_t offset){return at<uint32_t>(bsp.data(),offset);};const auto texinfo=word(8+6*16),texinfoBytes=word(8+6*16+4),texdataBytes=word(8+2*16+4);materialFlags.resize(texdataBytes/32);for(uint32_t i=0;i<texinfoBytes/72;++i){const auto id=word(texinfo+72*i+68);materialMapping.push_back(id);materialFlags.at(id)|=uint16_t(word(texinfo+72*i+64));}boundFlags=std::make_unique<SourceBspTraceSurfaceFlagsCandidate>(bsp,materialMapping);SourceBspBrushOnlyTraceCandidate world(bsp,materialMapping);
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1],0x643eec);image.seal();PrivateX86BoundaryObserverFixture observer{{0x1014e5d0,0x1015a7d0,0x1014cd40,0x1014ce00},policyBoundary};
#endif
 std::ofstream out(argv[3],std::ios::binary);require(bool(out),"point policy output");std::array<uint32_t,8> header{0x53505031,0,0,560,1866,43,1,2};out.write(reinterpret_cast<const char*>(header.data()),sizeof(header));unsigned cases=0,direct=0,fallback=0,fallbackSolid=0,cullTests=0,overlapTests=0,zero=0,tiny=0,wrapped=0,binding=0,distinct=0;
 auto record=[&](const auto& meshes,const auto& tables,const auto& refs,const Vec& point,const Vec& extents,uint32_t mask,unsigned mode,unsigned visit,uint32_t index,unsigned phase,unsigned q,unsigned list,unsigned config,unsigned disabled,
#ifndef _WIN64
 NativeWorld& nativeWorld
#else
 int
#endif
 ){const bool check=cases%31==0;auto expected=ownedPolicy(world,meshes,tables,refs,point,extents,mask,mode,visit,index,check);auto actual=expected;
#ifndef _WIN64
 actual=nativePolicy(nativeWorld,tables,refs,point,extents,mask,mode,visit,index);if(actual.work!=expected.work||actual.visits!=expected.visits||actual.childBrush!=expected.childBrush||actual.childDisp!=expected.childDisp||actual.culls!=expected.culls||actual.overlaps!=expected.overlaps||actual.direct!=expected.direct||actual.fallback!=expected.fallback||actual.normal!=expected.normal||actual.normalContents!=expected.normalContents){std::cerr<<"point policy case "<<cases<<" phase "<<phase<<" index "<<index<<" q "<<q<<" ext "<<extents[0]<<" mask "<<mask<<" mode "<<mode<<" visit "<<visit<<" list "<<list<<" config "<<config<<" disabled "<<disabled<<'\n';for(size_t i=0;i<560;++i)if(actual.work[i]!=expected.work[i])std::cerr<<"work byte "<<i<<" native "<<unsigned(actual.work[i])<<" owned "<<unsigned(expected.work[i])<<'\n';throw std::runtime_error("stationary point policy mismatch");}
#endif
 std::array<uint32_t,17> input{};for(size_t a=0;a<3;++a){input[a]=bits(point[a]);input[3+a]=bits(extents[a]);}input[6]=mask;input[7]=mode;input[8]=visit;input[9]=index;input[10]=phase;input[11]=q;input[12]=list;input[13]=config;input[14]=disabled;input[15]=uint32_t(actual.culls.size());input[16]=uint32_t(actual.overlaps.size());out.write(reinterpret_cast<const char*>(input.data()),sizeof(input));for(const auto* ids:{&actual.culls,&actual.overlaps})out.write(reinterpret_cast<const char*>(ids->data()),std::streamsize(ids->size()*4));std::array<uint32_t,6> metadata{actual.direct,uint32_t(actual.fallback),bits(actual.normal[0]),bits(actual.normal[1]),bits(actual.normal[2]),actual.normalContents};out.write(reinterpret_cast<const char*>(metadata.data()),sizeof(metadata));out.write(reinterpret_cast<const char*>(actual.work.data()),560);writeMarks(out,actual.visits,17);writeMarks(out,actual.childBrush,actual.fallback?1u:17u);writeMarks(out,actual.childDisp,actual.fallback?1u:17u);require(bool(out),"point policy record");++cases;direct+=actual.direct!=UINT32_MAX;fallback+=actual.fallback;fallbackSolid+=actual.fallback&&actual.work[0x8b];cullTests+=unsigned(actual.culls.size());overlapTests+=unsigned(actual.overlaps.size());zero+=extents==Vec{};tiny+=extents!=Vec{};wrapped+=visit==2;binding+=check;distinct+=actual.direct!=UINT32_MAX&&tables.culls[actual.direct].contents!=meshes[actual.direct].contents;
 };
 for(unsigned hdr=0;hdr<2;++hdr){const SourceBspDisplacementCollisionCandidate imported(bsp,int32_t(hdr));const auto& meshes=imported.meshes;auto tables=pointTables(world,meshes);
#ifndef _WIN64
 pointObjectsPrepare(meshes,tables);NativeWorld nativeWorld(world);std::vector<uint16_t> boundRefs;bindRefs(nativeWorld,tables,boundRefs);
#endif
 for(uint32_t index=0;index<meshes.size();++index){const auto next=(index+1)%uint32_t(meshes.size());const auto& mesh=meshes[index];const auto& b=mesh.tree.expandedBounds;const auto& triangle=mesh.triangles[index%mesh.triangles.size()];Vec centroid{};for(size_t a=0;a<3;++a)centroid[a]=(mesh.vertices[triangle.packedVertices[0]&511u][a]+mesh.vertices[triangle.packedVertices[1]&511u][a]+mesh.vertices[triangle.packedVertices[2]&511u][a])/3.f;
 for(unsigned q=0;q<7;++q){Vec point=centroid;if(q==1||q==5||q==6)for(size_t a=0;a<3;++a)point[a]+=(q==1?.0001220703125f:q==5?8.f:-8.f)*mesh.baseNormal[a];if(q==2)for(size_t a=0;a<3;++a)point[a]=(b[a]+b[a+3])*.5f;if(q==3)point={b[0]-.000244140625f,b[1],b[2]};if(q==4)point={b[3]+.000244140625f,b[4],b[5]};const unsigned list=q%2;const std::vector<uint32_t> refs=list?std::vector<uint32_t>{next,index,next}:std::vector<uint32_t>{index,next,index};
 for(unsigned hull=0;hull<3;++hull)for(uint32_t mask:{0u,1u,0x2080u,0xffffffffu})for(unsigned mode:{0u,1u})for(unsigned visit=0;visit<3;++visit)record(meshes,tables,refs,point,hull==0?Vec{}:hull==1?Vec{.000732421875f,.000244140625f,0}:Vec{.000244140625f,.000244140625f,.000244140625f},mask,mode,visit,index,0,q,list,0,0,
#ifndef _WIN64
 nativeWorld
#else
 0
#endif
 );
 }} }
 header[1]=cases;controlledWorld(world);auto meshes=controlledMeshes();auto tables=pointTables(world,meshes);
#ifndef _WIN64
 pointObjectsPrepare(meshes,tables);NativeWorld nativeWorld(world);std::vector<uint16_t> boundRefs;bindRefs(nativeWorld,tables,boundRefs);
#endif
 for(unsigned q=0;q<7;++q)for(unsigned hull=0;hull<3;++hull)for(unsigned list=0;list<3;++list)for(uint32_t mask:{0u,1u,2u,0x2081u,0xffffffffu})for(unsigned mode:{0u,1u})for(unsigned visit=0;visit<3;++visit)for(unsigned config=0;config<3;++config)for(unsigned disabled=0;disabled<3;++disabled){const Vec point{std::array<float,7>{-3,-2,-6,4,0,4.0001220703125f,3.9998779296875f}[q],0,0},extents=hull==0?Vec{}:hull==1?Vec{.000732421875f,.000244140625f,0}:Vec{.000244140625f,.000244140625f,.000244140625f};const std::vector<uint32_t> refs=list==0?std::vector<uint32_t>{1,0}:list==1?std::vector<uint32_t>{0,1}:std::vector<uint32_t>{1,1,0};auto configured=tables;if(config==1)for(auto& c:configured.culls)c.visitIndex=0;if(config==2)configured.culls[0].contents=2;meshes[0].runtimeFlags=disabled==0?0u:disabled==1?4u:8u;
#ifndef _WIN64
 at<uint32_t>(pointObjects[0].data(),0x2c)=meshes[0].runtimeFlags;
#endif
 record(meshes,configured,refs,point,extents,mask,mode,visit,0,1,q,list,config,disabled,
#ifndef _WIN64
 nativeWorld
#else
 0
#endif
 );
 }
 header[2]=cases-header[1];out.seekp(0);out.write(reinterpret_cast<const char*>(header.data()),sizeof(header));out.flush();require(bool(out),"point policy output finish");
#ifndef _WIN64
 image.verify();require(routeObservations==fallback&&pointObservations==fallback,"all fallbacks execute original point geometry");
#endif
 require(direct&&fallback&&fallbackSolid&&cullTests&&overlapTests&&zero&&tiny&&wrapped&&distinct,"stationary point policy coverage");std::cout<<"{\"cases\":"<<cases<<",\"benchmark_cases\":"<<header[1]<<",\"controlled_cases\":"<<header[2]<<",\"direct_overlaps\":"<<direct<<",\"fallbacks\":"<<fallback<<",\"fallback_solid\":"<<fallbackSolid<<",\"cull_tests\":"<<cullTests<<",\"mesh_overlap_tests\":"<<overlapTests<<",\"zero_bounds\":"<<zero<<",\"tiny_bounds\":"<<tiny<<",\"wrapped_generations\":"<<wrapped<<",\"binding_checks\":"<<binding<<",\"distinct_contents_hits\":"<<distinct;
#ifndef _WIN64
 std::cout<<",\"native_route_observations\":"<<routeObservations<<",\"native_point_observations\":"<<pointObservations;
#endif
 std::cout<<",\"passed\":true}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

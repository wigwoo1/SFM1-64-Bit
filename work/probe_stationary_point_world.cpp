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
#include "source_stationary_hull_world_candidate.h"
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



struct WorldResult {Work work{};SourceStationaryLeafSelectionCandidate selection;std::vector<uint32_t> visited,brushes,terrainLeaves,brushVisits,dispVisits,childBrush,childDisp;std::vector<std::array<uint32_t,4>> leafEnds;bool visitedNonSolid=false;unsigned fallbackCalls=0;};
std::vector<uint32_t> initialBrushMarks(size_t count,uint32_t index,unsigned mode){std::vector<uint32_t> v(count);if(mode==1&&count)v[index%count]=17;if(mode==2)for(size_t i=0;i<count;++i)if(i%3==index%3)v[i]=17;return v;}
std::vector<uint32_t> initialDispMarks(size_t count,uint32_t index,unsigned mode){std::vector<uint32_t> v(count);if(mode)v[(index+(mode==2?1u:0u))%count]=17;return v;}
WorldResult ownedWorld(const SourceBspBrushOnlyTraceCandidate& world,const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,const PointTables& tables,int32_t root,const Vec& point,const Vec& extents,uint32_t mask,unsigned mode,unsigned visit,uint32_t index,bool standalone,bool convenience=false){WorldResult result;result.work=initialWork(extents,mask,mode,visit==2);std::memcpy(result.work.data(),point.data(),12);result.work[0x100]=1;at<uint32_t>(result.work.data(),0x148)=uint32_t(world.data.brushes.size());at<uint32_t>(result.work.data(),0x170)=uint32_t(meshes.size());result.brushVisits=initialBrushMarks(world.data.brushes.size(),index,visit);result.dispVisits=initialDispMarks(meshes.size(),index,visit);result.childBrush.assign(world.data.brushes.size(),visit==2?17u:0u);result.childDisp.assign(meshes.size(),visit==2?17u:0u);uint32_t childGeneration=visit==2?UINT32_MAX:0;auto state=initialState(mode);Vec minimum{},expanded{};std::vector<uint32_t> contents;for(const auto& brush:world.data.brushes)contents.push_back(brush.contents);for(size_t a=0;a<3;++a){minimum[a]=-extents[a];expanded[a]=extents[a]+1.f;}
 auto tracePoint=[&](const auto& ray,auto& output){++result.fallbackCalls;if(++childGeneration==0){childGeneration=1;std::fill(result.childBrush.begin(),result.childBrush.end(),0);std::fill(result.childDisp.begin(),result.childDisp.end(),0);}sourcePreparedPointWorldCandidate(world,meshes,tables.refs,tables.culls,tables.properties,world.root,ray.start,ray.delta,extents,mask,result.childBrush,result.childDisp,childGeneration,output,[&](uint32_t brush){return excludedBrush(world,brush,0);},[&](const SourceCollisionSurfaceCandidate& surface){return details(surface,0);},false);std::memcpy(result.work.data()+0x3c,ray.delta.data(),12);std::memcpy(result.work.data()+0x48,ray.inverse.data(),12);std::memcpy(result.work.data()+0x108,ray.normal.data(),12);at<uint32_t>(result.work.data(),0x124)=childGeneration;};
 auto leaf=[&](uint32_t id,auto& output){result.visited.push_back(id);if(!standalone)result.visitedNonSolid|=!(world.data.leafContents.at(id)&1u);sourceStationaryHullLeafPolicyCandidate(world.data.leafBrushes.at(id),contents,result.brushVisits,17,mask,!tables.refs.at(id).empty(),output,[&](uint32_t brush,auto& target){result.brushes.push_back(brush);sourceStationaryBrushHullCandidate(world.data,world.records,brush,point,minimum,extents,target.trace);},[&](auto& target){result.terrainLeaves.push_back(id);sourceStationaryDisplacementHullCandidate(meshes,tables.culls,tables.refs.at(id),result.dispVisits,17,point,minimum,extents,mask,{0x3344,0},target,tracePoint,true);});result.leafEnds.push_back({bits(output.trace.fraction),bits(output.trace.leftSolid),uint32_t(output.trace.allSolid),uint32_t(output.trace.startSolid)});};
 if(standalone){result.selection.leaves={uint32_t(root)};if(convenience)sourceStationaryHullLeafCandidate(world,meshes,tables.culls,world.data.leafBrushes.at(root),tables.refs.at(root),result.brushVisits,result.dispVisits,17,point,minimum,extents,mask,{0x3344,0},state,tracePoint,true);else leaf(uint32_t(root),state);}
 else if(convenience){const auto outcome=sourceStationaryHullWorldCandidate(world,meshes,tables.refs,tables.culls,root,point,extents,result.brushVisits,result.dispVisits,17,mask,{0x3344,0},state,tracePoint,true);result.selection=outcome.selection;result.visitedNonSolid=outcome.visitedNonSolid;}
 else{result.selection=sourceStationaryLeafSelectionAtRootCandidate(world,root,point,expanded,1024);result.visitedNonSolid=sourceStationaryHullWorldPolicyCandidate(result.selection.leaves,world.data.leafContents,state,leaf);}
 putState(result.work,state);return result;
}
#ifndef _WIN64
uintptr_t activeWork=0,activeWorld=0,activeRay=0,brushBase=0,selectionRecord=0,leafReturn=0;int32_t activeRoot=0;bool standaloneQuery=false;WorldResult nativeObserved;Vec priorEnd{};unsigned totalPointEntries=0;
void setWorldSlot(CONTEXT& c,unsigned slot,uint32_t value){auto* o=PrivateX86BoundaryObserverFixture::active;o->entries[slot]=value;if(slot==0)c.Dr0=value;else if(slot==1)c.Dr1=value;else if(slot==2)c.Dr2=value;else if(slot==3)c.Dr3=value;else throw std::runtime_error("stationary point world slot");c.ContextFlags|=CONTEXT_DEBUG_REGISTERS;}
void worldBoundary(unsigned slot,CONTEXT& c){auto* work=reinterpret_cast<uint8_t*>(activeWork);
 if(slot==0&&c.Eip==0x10145810){selectionRecord=address<uintptr_t>(c.Esp+4);require(address<uintptr_t>(c.Esp+8)==activeRay&&address<int32_t>(c.Esp+16)==activeRoot&&address<uint32_t>(c.Esp)==0x101480d2,"point world selector inputs");const auto ext=address<uintptr_t>(c.Esp+12);for(size_t a=0;a<3;++a)require(address<float>(ext+4*a)==address<float>(activeRay+0x30+4*a)+1.f,"point stationary selection extents");require(address<int32_t>(selectionRecord)==-1&&address<uint32_t>(selectionRecord+4)==1024&&address<uintptr_t>(selectionRecord+12)==activeWorld,"point selector capacity/world");setWorldSlot(c,0,0x101480d2);}
 else if(slot==0&&c.Eip==0x101480d2){require(c.Eax<=1024,"point selected count");nativeObserved.selection.topNode=address<int32_t>(selectionRecord);const auto p=address<uintptr_t>(selectionRecord+8);nativeObserved.selection.leaves.assign(reinterpret_cast<uint32_t*>(p),reinterpret_cast<uint32_t*>(p)+c.Eax);setWorldSlot(c,0,0x10147f40);}
 else if(slot==0&&c.Eip==0x10147f40){require(c.Ecx==activeWork&&work[0x100]==1&&work[0x101]==0,"point stationary leaf flags/owner");leafReturn=address<uintptr_t>(c.Esp);if(!standaloneQuery)require(leafReturn==0x1014810b,"point world leaf caller");require(nativeObserved.visited.size()<nativeObserved.selection.leaves.size()&&c.Eax==nativeObserved.selection.leaves[nativeObserved.visited.size()],"point stationary leaf order");nativeObserved.visited.push_back(c.Eax);if(!standaloneQuery){const auto leaves=address<uintptr_t>(activeWorld+0x88);nativeObserved.visitedNonSolid|=!(address<uint32_t>(leaves+16*c.Eax)&1u);}setWorldSlot(c,0,uint32_t(leafReturn));}
 else if(slot==0&&c.Eip==leafReturn){nativeObserved.leafEnds.push_back({bits(at<float>(work,0x80)),bits(at<float>(work,0x8c)),work[0x8a],work[0x8b]});setWorldSlot(c,0,0x10147f40);}
 else if(slot==1&&c.Eip==0x1014ce00){require(address<uintptr_t>(c.Esp+4)==activeWork&&address<uint32_t>(c.Esp)==0x1014d4a7,"point world fallback caller");++nativeObserved.fallbackCalls;std::memcpy(priorEnd.data(),work+12,12);setWorldSlot(c,1,0x10149030);}
 else if(slot==1&&c.Eip==0x10149030){require(c.Ecx==activeWork&&c.Edx==0&&work[0x100]==1&&work[0x101]==0&&at<uint32_t>(work,0x11c)==1&&at<uint32_t>(work,0x120)==17&&at<uint32_t>(work,0x124)==nativeObserved.fallbackCalls,"point world nested generation/route");setWorldSlot(c,1,0x10148950);}
 else if(slot==1&&c.Eip==0x10148950){require(c.Ecx==activeWork&&c.Edx==0&&address<uintptr_t>(c.Esp+12)==activeWork&&address<uintptr_t>(c.Esp+16)==activeWork+12,"nested actual point geometry");++totalPointEntries;setWorldSlot(c,1,0x1014d4a7);}
 else if(slot==1&&c.Eip==0x1014d4a7){require(std::memcmp(priorEnd.data(),work+12,12)==0&&at<uint32_t>(work,0x11c)==0,"point world fallback restoration");setWorldSlot(c,1,0x1014ce00);}
 else if(slot==2){const auto p=address<uintptr_t>(c.Esp+4);require(c.Eip==0x10146c30&&c.Ecx==activeWork&&p>=brushBase&&(p-brushBase)%8==0&&address<uint32_t>(c.Esp)==0x10147fbb,"stationary point brush call");nativeObserved.brushes.push_back(uint32_t((p-brushBase)/8));}
 else if(slot==3){require(c.Eip==0x1014d320&&address<uintptr_t>(c.Esp+4)==activeWork&&address<uint32_t>(c.Esp)==0x10148020&&!nativeObserved.visited.empty(),"point stationary terrain gate");nativeObserved.terrainLeaves.push_back(nativeObserved.visited.back());}
 else throw std::runtime_error("unexpected point world boundary");
 c.EFlags|=0x10000u;
}
void bindRefs(NativeWorld& world,const PointTables& tables,std::vector<uint16_t>& refs){refs.clear();for(size_t i=0;i<tables.refs.size();++i){require(refs.size()+tables.refs[i].size()<65536,"point world reference span");world.leaves[i].dispFirst=uint16_t(refs.size());world.leaves[i].dispCount=uint16_t(tables.refs[i].size());for(auto id:tables.refs[i])refs.push_back(uint16_t(id));}at<uintptr_t>(world.fields.data(),0xb0)=reinterpret_cast<uintptr_t>(refs.data());}
WorldResult nativeWorldTrace(NativeWorld& world,int32_t root,const Vec& point,const Vec& extents,uint32_t mask,unsigned mode,unsigned visit,uint32_t index,bool standalone){for(uint32_t i=0;i<world.surfaces.size();++i)world.surfaces[i]={0x10000000u+i,uint32_t(1000+i)|(uint32_t(materialFlags[i])<<16)};address<uint32_t>(0x10402fe8)=UINT32_MAX;address<uint32_t>(0x10402fec)=0x3344;for(size_t i=0;i<world.leaves.size();++i)world.leaves[i].contents=world.source.data.leafContents.at(i);
 WorldResult result;result.work=initialWork(extents,mask,mode,visit==2);std::memcpy(result.work.data(),point.data(),12);result.work[0x100]=1;at<uint32_t>(result.work.data(),0x148)=uint32_t(world.brushes.size());at<uint32_t>(result.work.data(),0x170)=uint32_t(pointObjects.size());result.brushVisits=initialBrushMarks(world.brushes.size(),index,visit);result.dispVisits=initialDispMarks(pointObjects.size(),index,visit);result.childBrush.assign(world.brushes.size(),visit==2?17u:0u);result.childDisp.assign(pointObjects.size(),visit==2?17u:0u);auto* work=result.work.data();at<uintptr_t>(work,0x104)=reinterpret_cast<uintptr_t>(world.fields.data());at<uintptr_t>(work,0x128)=reinterpret_cast<uintptr_t>(result.brushVisits.data());at<uintptr_t>(work,0x13c)=reinterpret_cast<uintptr_t>(result.childBrush.data());at<uintptr_t>(work,0x150)=reinterpret_cast<uintptr_t>(result.dispVisits.data());at<uintptr_t>(work,0x164)=reinterpret_cast<uintptr_t>(result.childDisp.data());alignas(16)std::array<uint8_t,80> queryRay{};std::memcpy(queryRay.data(),point.data(),12);std::memcpy(queryRay.data()+0x30,extents.data(),12);const auto beforeRay=queryRay;const auto beforeObjects=pointObjects;const auto beforeCulls=pointCulls;activeWork=reinterpret_cast<uintptr_t>(work);activeWorld=reinterpret_cast<uintptr_t>(world.fields.data());activeRay=reinterpret_cast<uintptr_t>(queryRay.data());activeRoot=root;standaloneQuery=standalone;brushBase=reinterpret_cast<uintptr_t>(world.brushes.data());nativeObserved={};if(standalone)nativeObserved.selection.leaves={uint32_t(root)};leafReturn=0;auto* observer=PrivateX86BoundaryObserverFixture::active;observer->entries={standalone?0x10147f40u:0x10145810u,0x1014ce00,0x10146c30,0x1014d320};observer->configure(true);
 if(standalone){const uintptr_t target=0x10147f40;__asm {mov ecx,work
 mov eax,root
 call target}}
 else{const uintptr_t target=0x10148030;auto* ray=queryRay.data();__asm {push root
 push ray
 mov esi,work
 call target
 add esp,8}}
 observer->configure(false);result.selection=nativeObserved.selection;result.visited=nativeObserved.visited;result.visitedNonSolid=nativeObserved.visitedNonSolid;result.brushes=nativeObserved.brushes;result.terrainLeaves=nativeObserved.terrainLeaves;result.leafEnds=nativeObserved.leafEnds;result.fallbackCalls=nativeObserved.fallbackCalls;require(queryRay==beforeRay&&pointObjects==beforeObjects&&pointCulls==beforeCulls,"point world input preservation");require(at<uint32_t>(work,0x11c)==0&&at<uint32_t>(work,0x120)==17&&at<uint32_t>(work,0x124)==(result.fallbackCalls?result.fallbackCalls:visit==2?UINT32_MAX:0u),"point world transaction finish");
 const auto name=at<uint32_t>(work,0x90);if(name==0x10350ee0u){const auto property=at<uint16_t>(work,0x94);const uint32_t id=property>=500?uint32_t(property-500):uint32_t(property-100);require(id<pointObjects.size(),"point world terrain surface identity");at<uint32_t>(work,0x90)=0x20000000u+id;}else require(name==UINT32_MAX||(name>=0x10000000u&&name<0x10000000u+materialFlags.size()),"point world surface namespace");for(size_t at:{size_t(0x104),size_t(0x128),size_t(0x13c),size_t(0x150),size_t(0x164)})std::memset(work+at,0,4);return result;
}
#endif
void prepareControlledWorld(SourceBspBrushOnlyTraceCandidate& world,bool comb){controlledWorld(world);world.data.brushes.push_back({0,6,0x80});world.data.brushes.push_back({0,6,0});int16_t texture=-1;for(size_t i=0;i<materialMapping.size();++i)if(materialFlags.at(materialMapping[i])&0x80){texture=int16_t(i);break;}require(texture>=0,"point stationary surface fixture");for(auto& side:world.data.sides)side.texture=texture;world.records=sourceBspCollisionRecordsCandidate(world.data,materialMapping);world.data.worldBrushes={0,1,2};world.data.leafBrushes={{},{0,1,2,1}};if(!comb)return;constexpr size_t count=1030;world.nodes.clear();world.data.leafBrushes.assign(count,{});world.data.leafBrushes[0]={0,1,2,1};world.data.leafContents.assign(count,0);world.data.worldNodes=count-1;for(size_t i=0;i<count-1;++i)world.nodes.push_back({0,{i+2==count?-int32_t(count):int32_t(i+1),-int32_t(i+1)}});}
void setSolidity(SourceBspBrushOnlyTraceCandidate& world,unsigned mode){for(size_t i=0;i<world.data.leafContents.size();++i)world.data.leafContents[i]=mode==0?0u:mode==1?1u:mode==2?(i+1==world.data.leafContents.size()?0u:1u):(i==0?0x81u:0x80u);}
void writeMarks(std::ofstream& out,const std::vector<uint32_t>& values){std::vector<uint8_t> bits((values.size()+7)/8);for(size_t i=0;i<values.size();++i){require(values[i]==0||values[i]==17,"point world parent visits");if(values[i])bits[i/8]|=uint8_t(1u<<(i%8));}out.write(reinterpret_cast<const char*>(bits.data()),bits.size());}
void writeRuns(std::ofstream& out,const std::vector<uint32_t>& values){std::vector<std::pair<uint16_t,uint32_t>> runs;for(size_t at=0;at<values.size();){size_t end=at+1;while(end<values.size()&&values[end]==values[at])++end;require(end-at<65536,"child visit run length");runs.push_back({uint16_t(end-at),values[at]});at=end;}require(runs.size()<65536,"child run count");const uint16_t n=uint16_t(runs.size());out.write(reinterpret_cast<const char*>(&n),2);for(const auto& run:runs){out.write(reinterpret_cast<const char*>(&run.first),2);out.write(reinterpret_cast<const char*>(&run.second),4);}}
int main(int argc,char** argv){try{require(argc==4,"expected engine or -, BSP and disposable output");const auto bsp=read(argv[2]);auto word=[&](size_t offset){return at<uint32_t>(bsp.data(),offset);};const auto texinfo=word(8+6*16),texinfoBytes=word(8+6*16+4),texdataBytes=word(8+2*16+4);materialFlags.resize(texdataBytes/32);for(uint32_t i=0;i<texinfoBytes/72;++i){const auto id=word(texinfo+72*i+68);materialMapping.push_back(id);materialFlags.at(id)|=uint16_t(word(texinfo+72*i+64));}boundFlags=std::make_unique<SourceBspTraceSurfaceFlagsCandidate>(bsp,materialMapping);SourceBspBrushOnlyTraceCandidate world(bsp,materialMapping);
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1],0x643eec);image.seal();PrivateX86BoundaryObserverFixture observer{{0x10145810,0x1014ce00,0x10146c30,0x1014d320},worldBoundary};
#endif
 std::ofstream out(argv[3],std::ios::binary);require(bool(out),"point world output");std::array<uint32_t,10> header{0x53505731,0,0,0,560,1866,43,3,2,1024};out.write(reinterpret_cast<const char*>(header.data()),sizeof(header));unsigned cases=0,leafCases=0,worldCases=0,selected=0,visited=0,solidVisited=0,solidFallbacks=0,normalFallbacks=0,multiFallback=0,maxFallback=0,brushCalls=0,terrainCalls=0,tiny=0,zero=0,premarked=0,unvisitedNonSolid=0,capped=0,binding=0;
 auto record=[&](const auto& meshes,const auto& tables,int32_t root,const Vec& point,const Vec& extents,uint32_t mask,unsigned mode,unsigned visit,uint32_t index,unsigned phase,unsigned q,unsigned rootMode,unsigned disabled,unsigned solidity,bool standalone,
#ifndef _WIN64
 NativeWorld& nativeWorld
#else
 int
#endif
 ){const bool check=cases%31==0;const auto expected=ownedWorld(world,meshes,tables,root,point,extents,mask,mode,visit,index,standalone);auto actual=expected;if(check){const auto alternate=ownedWorld(world,meshes,tables,root,point,extents,mask,mode,visit,index,standalone,true);require(alternate.work==expected.work&&alternate.brushVisits==expected.brushVisits&&alternate.dispVisits==expected.dispVisits&&alternate.childBrush==expected.childBrush&&alternate.childDisp==expected.childDisp&&alternate.selection.topNode==expected.selection.topNode&&alternate.selection.leaves==expected.selection.leaves&&alternate.visitedNonSolid==expected.visitedNonSolid&&alternate.fallbackCalls==expected.fallbackCalls,"point leaf/world binding");}
#ifndef _WIN64
 actual=nativeWorldTrace(nativeWorld,root,point,extents,mask,mode,visit,index,standalone);if(actual.work!=expected.work||actual.brushVisits!=expected.brushVisits||actual.dispVisits!=expected.dispVisits||actual.childBrush!=expected.childBrush||actual.childDisp!=expected.childDisp||actual.selection.topNode!=expected.selection.topNode||actual.selection.leaves!=expected.selection.leaves||actual.visited!=expected.visited||actual.brushes!=expected.brushes||actual.terrainLeaves!=expected.terrainLeaves||actual.leafEnds!=expected.leafEnds||actual.visitedNonSolid!=expected.visitedNonSolid||actual.fallbackCalls!=expected.fallbackCalls){std::cerr<<"stationary point world case "<<cases<<" phase "<<phase<<" root "<<root<<" point "<<point[0]<<' '<<point[1]<<' '<<point[2]<<" extent "<<extents[0]<<" mask "<<mask<<" mode "<<mode<<" visit "<<visit<<" standalone "<<standalone<<" fallback "<<actual.fallbackCalls<<'/'<<expected.fallbackCalls<<'\n';for(size_t i=0;i<560;++i)if(actual.work[i]!=expected.work[i])std::cerr<<"work byte "<<i<<" native "<<unsigned(actual.work[i])<<" owned "<<unsigned(expected.work[i])<<'\n';throw std::runtime_error("complete stationary point world mismatch");}
#endif
 std::array<uint32_t,18> input{};for(size_t a=0;a<3;++a){input[a]=bits(point[a]);input[3+a]=bits(extents[a]);}input[6]=mask;input[7]=mode;input[8]=visit;input[9]=index;input[10]=phase;input[11]=q;input[12]=rootMode;input[13]=disabled;input[14]=solidity;input[15]=standalone;input[16]=uint32_t(root);input[17]=uint32_t(actual.selection.topNode);out.write(reinterpret_cast<const char*>(input.data()),sizeof(input));const std::array<uint32_t,6> meta{uint32_t(actual.selection.leaves.size()),uint32_t(actual.visited.size()),uint32_t(actual.brushes.size()),uint32_t(actual.terrainLeaves.size()),uint32_t(actual.visitedNonSolid),actual.fallbackCalls};out.write(reinterpret_cast<const char*>(meta.data()),sizeof(meta));for(const auto* ids:{&actual.selection.leaves,&actual.visited,&actual.brushes,&actual.terrainLeaves})out.write(reinterpret_cast<const char*>(ids->data()),std::streamsize(ids->size()*4));out.write(reinterpret_cast<const char*>(actual.leafEnds.data()),std::streamsize(actual.leafEnds.size()*16));out.write(reinterpret_cast<const char*>(actual.work.data()),560);writeMarks(out,actual.brushVisits);writeMarks(out,actual.dispVisits);writeRuns(out,actual.childBrush);writeRuns(out,actual.childDisp);require(bool(out),"point leaf/world record");++cases;leafCases+=standalone;worldCases+=!standalone;selected+=unsigned(actual.selection.leaves.size());visited+=unsigned(actual.visited.size());for(auto leaf:actual.visited)solidVisited+=(world.data.leafContents.at(leaf)&1u)!=0;solidFallbacks+=!standalone&&!actual.visitedNonSolid;normalFallbacks+=actual.fallbackCalls;multiFallback+=actual.fallbackCalls>1;maxFallback=std::max(maxFallback,actual.fallbackCalls);brushCalls+=unsigned(actual.brushes.size());terrainCalls+=unsigned(actual.terrainLeaves.size());tiny+=extents!=Vec{};zero+=extents==Vec{};premarked+=visit!=0;unvisitedNonSolid+=!standalone&&!actual.visitedNonSolid&&std::any_of(actual.selection.leaves.begin(),actual.selection.leaves.end(),[&](uint32_t id){return !(world.data.leafContents[id]&1u);});capped+=actual.selection.leaves.size()==1024;binding+=check;
 };
 for(unsigned hdr=0;hdr<2;++hdr){const SourceBspDisplacementCollisionCandidate imported(bsp,int32_t(hdr));const auto& meshes=imported.meshes;const auto tables=pointTables(world,meshes);
#ifndef _WIN64
 pointObjectsPrepare(meshes,tables);NativeWorld nativeWorld(world);std::vector<uint16_t> boundRefs;bindRefs(nativeWorld,tables,boundRefs);
#endif
 for(uint32_t index=0;index<meshes.size();++index){const auto& mesh=meshes[index];const auto& b=mesh.tree.expandedBounds;const auto& t=mesh.triangles[index%mesh.triangles.size()];Vec centroid{};for(size_t a=0;a<3;++a)centroid[a]=(mesh.vertices[t.packedVertices[0]&511u][a]+mesh.vertices[t.packedVertices[1]&511u][a]+mesh.vertices[t.packedVertices[2]&511u][a])/3.f;
 for(unsigned q=0;q<3;++q){Vec point=centroid;if(q<2)for(size_t a=0;a<3;++a)point[a]=(b[a]+b[a+3])*.5f+(q?-.0001220703125f:8.f)*mesh.baseNormal[a];
 for(unsigned hull=0;hull<3;++hull)for(uint32_t mask:{0u,1u,0x2080u,0xffffffffu})for(unsigned mode:{0u,1u,2u,4u,9u})for(unsigned visit=0;visit<3;++visit)for(bool standalone:{false,true}){const Vec extents=hull==0?Vec{}:hull==1?Vec{.000732421875f,.000244140625f,0}:Vec{.000244140625f,.000244140625f,.000244140625f};int32_t root=world.root;if(standalone){Vec expanded;for(size_t a=0;a<3;++a)expanded[a]=extents[a]+1.f;const auto selection=sourceStationaryLeafSelectionCandidate(world,point,expanded);root=int32_t(selection.leaves.at(index%selection.leaves.size()));}record(meshes,tables,root,point,extents,mask,mode,visit,index,0,q,0,0,0,standalone,
#ifndef _WIN64
 nativeWorld
#else
 0
#endif
 );}
 }} }
 header[1]=cases;
 for(unsigned topology=0;topology<3;++topology){const bool comb=topology==2;prepareControlledWorld(world,comb);if(topology==1)world.planes[0]={{.6f,.8f,0},0,3};auto meshes=controlledMeshes();const auto tables=pointTables(world,meshes);
#ifndef _WIN64
 pointObjectsPrepare(meshes,tables);NativeWorld nativeWorld(world);std::vector<uint16_t> boundRefs;bindRefs(nativeWorld,tables,boundRefs);
#endif
 for(unsigned q=0;q<(comb?1u:4u);++q)for(unsigned hull=0;hull<(comb?2u:3u);++hull)for(unsigned rootMode=0;rootMode<(comb?3u:4u);++rootMode)for(uint32_t mask:(comb?std::vector<uint32_t>{0,1}:std::vector<uint32_t>{0,1,0x2080,0xffffffffu}))for(unsigned mode:(comb?std::vector<unsigned>{0,4,9}:std::vector<unsigned>{0,1,2,4,9}))for(unsigned visit:(comb?std::vector<unsigned>{0,2}:std::vector<unsigned>{0,1,2}))for(unsigned disabled=0;disabled<(comb?1u:3u);++disabled)for(unsigned solidity=0;solidity<4;++solidity){setSolidity(world,solidity);meshes[0].runtimeFlags=disabled==0?0u:disabled==1?4u:8u;
#ifndef _WIN64
 at<uint32_t>(pointObjects[0].data(),0x2c)=meshes[0].runtimeFlags;
#endif
 const Vec point{comb?0.f:std::array<float,4>{-3,0,4,4.0001220703125f}[q],0,0},extents=hull==0?Vec{}:hull==1?Vec{.000732421875f,.000244140625f,0}:Vec{.000244140625f,.000244140625f,.000244140625f};const bool standalone=rootMode==3;const int32_t root=rootMode==0?0:rootMode==1?-1:standalone?int32_t(world.data.leafContents.size()-1):-int32_t(world.data.leafContents.size());record(meshes,tables,root,point,extents,mask,mode,visit,0,topology+1,q,rootMode,disabled,solidity,standalone,
#ifndef _WIN64
 nativeWorld
#else
 0
#endif
 );}
 if(!comb)header[2]=cases-header[1];else header[3]=cases-header[1]-header[2];
 }
 out.seekp(0);out.write(reinterpret_cast<const char*>(header.data()),sizeof(header));out.flush();require(bool(out),"point world output finish");
#ifndef _WIN64
 image.verify();require(totalPointEntries==normalFallbacks,"all point world nested routes observed");
#endif
 require(leafCases&&worldCases&&solidVisited&&solidFallbacks&&normalFallbacks&&multiFallback&&unvisitedNonSolid&&capped,"stationary point leaf/world coverage");std::cout<<"{\"cases\":"<<cases<<",\"benchmark_cases\":"<<header[1]<<",\"controlled_cases\":"<<header[2]<<",\"capacity_cases\":"<<header[3]<<",\"leaf_queries\":"<<leafCases<<",\"world_queries\":"<<worldCases<<",\"selected_leaves\":"<<selected<<",\"visited_leaves\":"<<visited<<",\"solid_leaves_visited\":"<<solidVisited<<",\"solid_leaf_finishes\":"<<solidFallbacks<<",\"normal_fallback_calls\":"<<normalFallbacks<<",\"multiple_fallback_queries\":"<<multiFallback<<",\"max_fallbacks_per_query\":"<<maxFallback<<",\"brush_calls\":"<<brushCalls<<",\"terrain_dispatch_calls\":"<<terrainCalls<<",\"tiny_extents\":"<<tiny<<",\"zero_extents\":"<<zero<<",\"premarked_queries\":"<<premarked<<",\"unvisited_non_solid_selected\":"<<unvisitedNonSolid<<",\"capacity_limited_queries\":"<<capped<<",\"binding_checks\":"<<binding;
#ifndef _WIN64
 std::cout<<",\"native_point_observations\":"<<totalPointEntries;
#endif
 std::cout<<",\"passed\":true}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

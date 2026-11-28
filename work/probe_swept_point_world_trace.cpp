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
#include "source_swept_point_world_trace_candidate.h"
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
struct PointTables {std::vector<std::vector<uint32_t>> refs;std::vector<SourceLeafDisplacementCullCandidate> culls;std::vector<std::array<uint16_t,2>> properties;};
PointTables pointTables(const SourceBspBrushOnlyTraceCandidate& world,const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes){PointTables tables;tables.refs.resize(world.data.leafBrushes.size());for(size_t i=0;i<meshes.size();++i){const auto& b=meshes[i].tree.expandedBounds;tables.culls.push_back({{b[0],b[1],b[2]},{b[3],b[4],b[5]},meshes[i].contents,uint32_t(i)});tables.properties.push_back({uint16_t(100+i),uint16_t(500+i)});for(auto leaf:sourceDisplacementLeafAssignmentCandidate(world,{b[0],b[1],b[2]},{b[3],b[4],b[5]}))tables.refs.at(leaf).push_back(uint32_t(i));}return tables;}

#ifndef _WIN64
std::vector<std::array<uint8_t,244>> pointObjects;std::vector<std::array<uint8_t,48>> pointCulls;
// Original point tree uses object+98 for its logical internal-node count.
void pointObjectsPrepare(const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,const PointTables& tables){pointObjects.resize(meshes.size());pointCulls.resize(meshes.size());for(size_t i=0;i<meshes.size();++i){const auto& m=meshes[i];auto* object=pointObjects[i].data();pointObjects[i].fill(0);at<uint32_t>(object,0x20)=m.contents;at<uint32_t>(object,0x28)=m.power;at<uint32_t>(object,0x2c)=m.runtimeFlags;std::memcpy(object+4,m.tree.expandedBounds.data(),12);std::memcpy(object+0x14,m.tree.expandedBounds.data()+3,12);std::memcpy(object+0x60,m.baseNormal.data(),12);std::memcpy(object+0x6c,tables.properties[i].data(),4);at<uintptr_t>(object,0x70)=reinterpret_cast<uintptr_t>(m.vertices.data());at<uintptr_t>(object,0x80)=reinterpret_cast<uintptr_t>(m.triangles.data());at<uintptr_t>(object,0x90)=reinterpret_cast<uintptr_t>(m.tree.nodes.data());at<uintptr_t>(object,0xa0)=reinterpret_cast<uintptr_t>(m.tree.leafPairs.data());for(auto row:{std::array<uint32_t,2>{0x70,uint32_t(m.vertices.size())},std::array<uint32_t,2>{0x80,uint32_t(m.triangles.size())},std::array<uint32_t,2>{0x90,uint32_t(m.tree.nodes.size())},std::array<uint32_t,2>{0xa0,uint32_t(m.tree.leafPairs.size())}}){at<uint32_t>(object,row[0]+4)=row[1];at<uint32_t>(object,row[0]+8)=row[1];at<uint32_t>(object,row[0]+12)=row[1];}auto* c=pointCulls[i].data();pointCulls[i].fill(0);std::memcpy(c,tables.culls[i].mins.data(),12);std::memcpy(c+16,tables.culls[i].maxs.data(),12);at<uint32_t>(c,32)=tables.culls[i].visitIndex;at<uint32_t>(c,36)=tables.culls[i].contents;}address<uintptr_t>(0x10637820)=reinterpret_cast<uintptr_t>(pointObjects.data());address<uintptr_t>(0x10637824)=reinterpret_cast<uintptr_t>(pointCulls.data());}

#endif
std::vector<SourceDisplacementRuntimeMeshCandidate> mixedPointMeshes(){std::vector<SourceDisplacementRuntimeMeshCandidate> meshes;for(unsigned id=0;id<4;++id){SourceDisplacementRuntimeMeshCandidate mesh;mesh.power=2;mesh.contents=std::array<uint32_t,4>{1,0x80,0x4000,0x20}[id];mesh.runtimeFlags=id==2?4u:0u;for(unsigned row=0;row<5;++row)for(unsigned column=0;column<5;++column){const float x=16.f*float(column)-32.f,y=16.f*float(row)-32.f,z=id==0?0.f:id==1?24.f:id==2?8.f+.25f*x-.125f*y:-8.f;mesh.vertices.push_back({x,y,z});}std::vector<std::array<uint16_t,3>> packed;for(const auto& grid:sourceDisplacementGridTrianglesCandidate(2,std::vector<uint16_t>(32))){SourceDisplacementCollisionTriangleCandidate triangle;triangle.packedVertices=grid.vertices;triangle.flags=uint16_t(id&1?0x440:0x240);mesh.triangles.push_back(sourceDisplacementCollisionTriangleCandidate(mesh.vertices,triangle));packed.push_back(grid.vertices);}mesh.tree=sourceDisplacementCollisionTreeCandidate(2,mesh.vertices,packed);mesh.baseNormal=mesh.triangles.front().normal;meshes.push_back(std::move(mesh));}return meshes;}
void mixedPointWorld(SourceBspBrushOnlyTraceCandidate& w,unsigned profile,PointTables& tables){w.data.planes.clear();w.data.sides.clear();w.data.brushes.clear();int16_t flagged=-1,plain=-1;for(size_t i=0;i<materialMapping.size();++i){if((materialFlags[materialMapping[i]]&0x80)&&flagged<0)flagged=int16_t(i);if(!(materialFlags[materialMapping[i]]&0x80)&&plain<0)plain=int16_t(i);}require(flagged>=0&&plain>=0,"point material flag fixtures");const std::array<Vec,8> normals{Vec{1,0,0},Vec{-1,0,0},Vec{0,1,0},Vec{0,-1,0},Vec{0,0,1},Vec{0,0,-1},Vec{.6f,.8f,0},Vec{-.5f,.5f,.70710677f}};const std::array<uint32_t,8> contents{1,0x80,0x81,0x2080,0x80,1,0x4000,0x80};
 for(unsigned brush=0;brush<8;++brush){const float center=float(int(brush%3)-1)*32;const auto first=uint32_t(w.data.sides.size()),count=brush<4?6u:8u;for(unsigned i=0;i<count;++i){const auto index=uint16_t(w.data.planes.size());w.data.planes.push_back(sourceBspCollisionPlaneCandidate(normals[i],12+normals[i][0]*center,i<6?i/2:3));const int16_t texture=brush==3||brush==7?-1:brush%2==0?flagged:plain;w.data.sides.push_back({index,texture,-1,int16_t(i>=6)});}w.data.brushes.push_back({first,count,contents[brush]});}
 w.planes={{{0,1,0},0,1},{{1,0,0},0,0},{{1,0,0},0,0}};w.nodes={{0,{1,2}},{1,{-1,-2}},{2,{-3,-4}}};w.root=0;w.data.leafBrushes={{0,1,0,2,3,4,5,6,7},{7,6,5,4,3,2,1,0,7},{0,2,4,6,0,1,3,5,7},{1,1,3,3,5,5,7,0,2,4,6}};
 if(profile==0)tables.refs={{0,1,0,2,3},{3,2,1,0,1},{0,1,2,3,0},{1,0,1,2,3}};
 if(profile==1){w.data.leafBrushes={{},{},{1,1},{1}};tables.refs={{1,0,1},{1,0},{2,3},{3,2}};const auto first=w.data.brushes[1].first;w.data.planes[w.data.sides[first+4].plane].distance=40;w.data.planes[w.data.sides[first+5].plane].distance=-32;}
 if(profile==2){w.data.leafBrushes={{1,1},{1},{},{}};tables.refs={{2,3},{3,2},{1,0,1},{1,0}};}
 if(profile==3){w.nodes.clear();w.root=-1;w.data.leafBrushes={{0,1,2,3,4,5,6,7,0,1}};tables.refs={{0,1,0,2,3,1}};}
 w.data.leafContents.assign(w.data.leafBrushes.size(),0);w.data.worldNodes=w.nodes.size();w.data.worldBrushes={0,1,2,3,4,5,6,7};w.records=sourceBspCollisionRecordsCandidate(w.data,materialMapping);
}

Record owned(const SourceBspBrushOnlyTraceCandidate& world,const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,const SourcePreparedHullWorldTablesCandidate& tables,const std::vector<std::array<uint16_t,2>>& properties,const SourceServerHullRayCandidate& ray,uint32_t mask,bool compute,bool available){return pack(sourceSweptPointWorldTraceCandidate(world,meshes,tables,properties,ray,mask,{0x3344,0},compute,available,[&](uint32_t index){return excludedBrush(world,index,0);},[&](const SourceCollisionSurfaceCandidate& surface){return details(surface,0);},[&](const SourceCollisionSurfaceCandidate& surface){return boundFlags->identity(surface);}));}
#ifndef _WIN64
uintptr_t activeWork=0,activeInputRay=0;const SourceServerHullRayCandidate* activeRay=nullptr;int32_t activeRoot=0;uint32_t activeMask=0;unsigned poolAcquires=0,poolReleases=0,setupObservations=0,finishObservations=0,leafObservations=0;
void slot0(CONTEXT& c,uint32_t entry){PrivateX86BoundaryObserverFixture::active->entries[0]=entry;c.Dr0=entry;c.ContextFlags|=CONTEXT_DEBUG_REGISTERS;}
void boundary(unsigned index,CONTEXT& c){using O=PrivateX86BoundaryObserverFixture;auto* work=reinterpret_cast<uint8_t*>(activeWork);
 if(index==0&&c.Eip==0x10149230){require(++poolAcquires==1&&address<uint32_t>(c.Esp)==0x10149ebb,"public point work acquire");O::returnCdecl(c,uint32_t(activeWork));slot0(c,0x10148950);}
 else if(index==0&&c.Eip==0x10148950){require(c.Ecx==activeWork&&int32_t(c.Edx)==activeRoot&&activeRay&&address<uint32_t>(c.Esp)==0x1014a0be&&address<float>(c.Esp+4)==0&&address<float>(c.Esp+8)==1&&address<uintptr_t>(c.Esp+12)==activeWork&&address<uintptr_t>(c.Esp+16)==activeWork+12,"public swept point geometry route");const auto& ray=*activeRay;for(size_t a=0;a<3;++a){const float end=a==0?ray.center[a]+ray.delta[a]:ray.delta[a]+ray.center[a],negative=ray.extents[a]*-1.f,inverse=ray.delta[a]==0.f?std::numeric_limits<float>::max():1.f/ray.delta[a];require(std::memcmp(work+4*a,&ray.center[a],4)==0&&std::memcmp(work+12+4*a,&end,4)==0&&std::memcmp(work+0x18+4*a,&negative,4)==0&&std::memcmp(work+0x24+4*a,&ray.extents[a],4)==0&&std::memcmp(work+0x30+4*a,&ray.extents[a],4)==0&&std::memcmp(work+0x3c+4*a,&ray.delta[a],4)==0&&std::memcmp(work+0x48+4*a,&inverse,4)==0,"public point setup field bits");}require(at<uint32_t>(work,0xfc)==activeMask&&work[0x100]==1&&work[0x101]==1&&at<uint32_t>(work,0x114)==0,"public point flags/marker reset");for(size_t a=0;a<3;++a)require(at<uint32_t>(work,0x108+4*a)==0,"public point normal reset");SourceSweptHullWorldTraceCandidate reset;reset.state.surfaceProperty=0x3344;const auto trace=pack(reset);require(std::memcmp(work+0x54,trace.data(),84)==0,"public point complete trace reset");++setupObservations;slot0(c,0x1014a0c5);c.EFlags|=0x10000u;}
 else if(index==0&&c.Eip==0x1014a0c5){require(c.Esi==activeWork,"public geometry return owner");slot0(c,0x10149230);c.EFlags|=0x10000u;}
 else if(index==1){require(c.Eip==0x1000e020&&c.Ecx==0x10637798&&address<uintptr_t>(c.Esp+4)==activeWork-4&&address<int32_t>(activeWork+0x11c)==-1&&++poolReleases==1,"public point work release");O::returnCdecl(c);c.Esp+=4;slot0(c,0x10149230);}
 else if(index==2){require(c.Eip==0x10148270&&c.Ecx==activeWork,"public actual point leaf");++leafObservations;c.EFlags|=0x10000u;}
 else if(index==3){require(c.Eip==0x10145e20&&c.Ecx==activeWork+0x54&&c.Edx==activeInputRay&&address<uint32_t>(c.Esp)==0x1014a0d4,"original public coordinate finisher");for(size_t a=0;a<6;++a)require(at<uint32_t>(work,0x54+4*a)==0,"collision altered position output before finishing");++finishObservations;c.EFlags|=0x10000u;}
 else throw std::runtime_error("unexpected public swept point boundary");
}
std::array<uint8_t,80> rayBytes(const SourceServerHullRayCandidate& ray){std::array<uint8_t,80> bytes{};std::memcpy(bytes.data(),ray.center.data(),12);std::memcpy(bytes.data()+0x10,ray.delta.data(),12);std::memcpy(bytes.data()+0x20,ray.offset.data(),12);std::memcpy(bytes.data()+0x30,ray.extents.data(),12);bytes[0x44]=uint8_t(ray.isRay);bytes[0x45]=uint8_t(ray.swept);return bytes;}
Record native(NativeWorld& world,const std::vector<std::vector<uint32_t>>& refs,const SourceServerHullRayCandidate& ray,uint32_t mask,bool compute,bool available,bool stale){
 for(uint32_t i=0;i<world.surfaces.size();++i)world.surfaces[i]={0x10000000u+i,uint32_t(1000+i)|(uint32_t(materialFlags[i])<<16)};address<uint32_t>(0x10402fe8)=0xffffffffu;address<uint32_t>(0x10402fec)=0x3344;require(address<uint32_t>(0x106c1228)==0,"wrapper memset feature");
 std::vector<uint16_t> dispRefs;for(size_t leaf=0;leaf<refs.size();++leaf){require(dispRefs.size()+refs[leaf].size()<65536,"public point leaf span");world.leaves[leaf].dispFirst=uint16_t(dispRefs.size());world.leaves[leaf].dispCount=uint16_t(refs[leaf].size());for(auto id:refs[leaf])dispRefs.push_back(uint16_t(id));}at<uintptr_t>(world.fields.data(),0xb0)=reinterpret_cast<uintptr_t>(dispRefs.data());at<uint32_t>(world.fields.data(),0x7c)=available?uint32_t(world.source.data.planes.size()):0;
 alignas(16)std::array<uint8_t,0x250> storage;storage.fill(0xa7);auto* work=storage.data()+16;std::vector<uint32_t> brushVisits(world.brushes.size(),stale?17u:0u),dispVisits(pointObjects.size(),stale?17u:0u),childBrush(world.brushes.size(),17u),childDisp(pointObjects.size(),17u);at<uintptr_t>(work,0x104)=reinterpret_cast<uintptr_t>(world.fields.data());at<uint32_t>(work,0x114)=0;at<uint32_t>(work,0x11c)=0;at<uint32_t>(work,0x120)=1;at<uint32_t>(work,0x124)=UINT32_MAX;at<uintptr_t>(work,0x128)=reinterpret_cast<uintptr_t>(brushVisits.data());at<uintptr_t>(work,0x13c)=reinterpret_cast<uintptr_t>(childBrush.data());at<uint32_t>(work,0x148)=uint32_t(world.brushes.size());at<uintptr_t>(work,0x150)=reinterpret_cast<uintptr_t>(dispVisits.data());at<uintptr_t>(work,0x164)=reinterpret_cast<uintptr_t>(childDisp.data());at<uint32_t>(work,0x170)=uint32_t(pointObjects.size());
 alignas(16)auto input=rayBytes(ray);const auto before=input;const auto beforeObjects=pointObjects;const auto beforeCulls=pointCulls;std::array<uint8_t,116> output;output.fill(0xa5);activeWork=reinterpret_cast<uintptr_t>(work);activeInputRay=reinterpret_cast<uintptr_t>(input.data());activeRay=&ray;activeMask=mask;activeRoot=world.source.root;poolAcquires=poolReleases=0;const auto priorSetups=setupObservations,priorFinishes=finishObservations;
 using Wrapper=void(__cdecl*)(const void*,int32_t,uint32_t,bool,void*);reinterpret_cast<Wrapper>(0x10149eb0)(input.data(),world.source.root,mask,compute,output.data()+16);require(poolAcquires==1&&poolReleases==1,"public point service balance");require(setupObservations-priorSetups==unsigned(available)&&finishObservations-priorFinishes==unsigned(available&&compute),"public point route/coordinate gates");require(input==before&&pointObjects==beforeObjects&&pointCulls==beforeCulls,"public wrapper changed input geometry");for(size_t i=0;i<16;++i)require(output[i]==0xa5&&output[100+i]==0xa5&&storage[i]==0xa7&&storage[0x240+i]==0xa7,"public output/work guards");require(at<uint32_t>(work,0x120)==1&&at<int32_t>(work,0x11c)==-1&&at<uint32_t>(work,0x124)==UINT32_MAX,"public generation/depth finish");for(auto mark:brushVisits)require(mark==1||mark==(stale?17u:0u),"public brush marks");for(auto mark:dispVisits)require(mark==1||mark==(stale?17u:0u),"public displacement marks");for(auto mark:childBrush)require(mark==17u,"unused child brush unchanged");for(auto mark:childDisp)require(mark==17u,"unused child displacement unchanged");
 Record result{};std::memcpy(result.data(),output.data()+16,84);const auto name=at<uint32_t>(result.data(),0x3c);if(name==0x10350ee0u){const auto property=at<uint16_t>(result.data(),0x40);const uint32_t id=property>=500?uint32_t(property-500):uint32_t(property-100);require(id<pointObjects.size(),"public terrain fixture identity");at<uint32_t>(result.data(),0x3c)=0x20000000u+id;}else require(name==0xffffffffu||(name>=0x10000000u&&name<0x10000000u+materialFlags.size()),"public surface namespace");at<uint32_t>(result.data(),84)=at<uint32_t>(work,0x114);return result;
}
#endif
SourceServerHullRayCandidate query(const Vec& center,const Vec& delta,unsigned variant){Vec low{},high{},start=center;if(variant==1){low={-.000732421875f,-.000244140625f,0};high={.000732421875f,.000244140625f,0};}if(variant==2){high={.00048828125f,.0009765625f,.000244140625f};for(size_t a=0;a<3;++a)start[a]-=high[a]*.5f;}if(variant==3){low=high=center;start={};}Vec end{};for(size_t a=0;a<3;++a)end[a]=start[a]+delta[a];auto ray=sourceServerHullRayCandidate(start,end,low,high);require(ray.isRay&&ray.swept,"constructed swept point flags");return ray;}
int main(int argc,char** argv){try{
 require(argc==4,"expected engine or -, BSP and disposable output");const auto bsp=read(argv[2]);auto word=[&](size_t offset){uint32_t value;require(offset<=bsp.size()&&bsp.size()-offset>=4,"BSP word");std::memcpy(&value,bsp.data()+offset,4);return value;};const auto texinfo=word(8+6*16),texinfoBytes=word(8+6*16+4),texdataBytes=word(8+2*16+4);materialFlags.resize(texdataBytes/32);for(uint32_t i=0;i<texinfoBytes/72;++i){const auto id=word(texinfo+72*i+68);materialMapping.push_back(id);materialFlags.at(id)|=uint16_t(word(texinfo+72*i+64));}boundFlags=std::make_unique<SourceBspTraceSurfaceFlagsCandidate>(bsp,materialMapping);SourceBspBrushOnlyTraceCandidate world(bsp,materialMapping);require(world.root==0&&world.data.brushes.size()==1866,"public point inventory");
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1],0x643eec);image.seal();PrivateX86BoundaryObserverFixture observer{{0x10149230,0x1000e020,0x10148270,0x10145e20},boundary};
#endif
 std::ofstream out(argv[3],std::ios::binary);require(bool(out),"public swept point output");std::array<uint32_t,8> header{0x53575031,0,0,0,0,176,0,0};out.write(reinterpret_cast<const char*>(header.data()),sizeof(header));unsigned cases=0,hits=0,solid=0,allSolid=0,empty=0,positionsOff=0,tiny=0,offsets=0,rounded=0,staleCount=0,zeroMasks=0,terrain=0,brush=0,negativeZero=0;
 auto record=[&](const auto& meshes,const PointTables& prepared,const SourceServerHullRayCandidate& ray,uint32_t mask,bool compute,bool available,bool stale,unsigned phase,unsigned index,unsigned q,unsigned variant,unsigned disabled,
#ifndef _WIN64
 NativeWorld& nativeWorld
#else
 int
#endif
 ){SourcePreparedHullWorldTablesCandidate tables;tables.leafDisplacements=prepared.refs;tables.culls=prepared.culls;require(tables.caches.empty(),"point trace must not require hull cache/permutation");const auto expected=owned(world,meshes,tables,prepared.properties,ray,mask,compute,available);auto actual=expected;
#ifndef _WIN64
 actual=native(nativeWorld,tables.leafDisplacements,ray,mask,compute,available,stale);if(actual!=expected){std::cerr<<"public swept point case "<<cases<<" phase "<<phase<<" index "<<index<<" q "<<q<<" mask "<<mask<<" variant "<<variant<<" compute "<<compute<<" available "<<available<<'\n';for(size_t i=0;i<actual.size();++i)if(actual[i]!=expected[i])std::cerr<<"byte "<<i<<" native "<<unsigned(actual[i])<<" owned "<<unsigned(expected[i])<<'\n';throw std::runtime_error("full public swept point mismatch");}
#endif
 std::array<uint32_t,22> input{};for(size_t a=0;a<3;++a){input[a]=bits(ray.center[a]);input[3+a]=bits(ray.delta[a]);input[6+a]=bits(ray.offset[a]);input[9+a]=bits(ray.extents[a]);}input[12]=uint32_t(ray.isRay)|(uint32_t(ray.swept)<<1);input[13]=mask;input[14]=compute;input[15]=available;input[16]=stale;input[17]=phase;input[18]=index;input[19]=q;input[20]=variant;input[21]=disabled;out.write(reinterpret_cast<const char*>(input.data()),sizeof(input));out.write(reinterpret_cast<const char*>(actual.data()),actual.size());require(bool(out),"public point record");++cases;hits+=at<float>(actual.data(),0x2c)<1;solid+=actual[0x37]!=0;allSolid+=actual[0x36]!=0;empty+=!available;positionsOff+=!compute;tiny+=ray.extents!=Vec{};offsets+=ray.offset!=Vec{};bool lost=false,nz=false;for(size_t a=0;a<3;++a){const float end=ray.center[a]+ray.delta[a];lost|=end-ray.center[a]!=ray.delta[a];}for(size_t a=0;a<6;++a)nz|=at<uint32_t>(actual.data(),4*a)==0x80000000u;rounded+=lost;negativeZero+=nz;staleCount+=stale;zeroMasks+=mask==0;const auto name=at<uint32_t>(actual.data(),0x3c);terrain+=name>=0x20000000u&&name<0x20000000u+meshes.size();brush+=name>=0x10000000u&&name<0x10000000u+materialFlags.size();};
 for(unsigned hdr=0;hdr<2;++hdr){const SourceBspDisplacementCollisionCandidate imported(bsp,int32_t(hdr));const auto& meshes=imported.meshes;const auto prepared=pointTables(world,meshes);
#ifndef _WIN64
 pointObjectsPrepare(meshes,prepared);observer.configure(true);NativeWorld nativeWorld(world);
#else
 int nativeWorld=0;
#endif
 for(unsigned index=0;index<meshes.size();++index){const auto& m=meshes[index];const auto& b=m.tree.expandedBounds;const auto& t=m.triangles[index%m.triangles.size()];Vec centroid{};for(size_t a=0;a<3;++a)centroid[a]=(m.vertices[t.packedVertices[0]&511u][a]+m.vertices[t.packedVertices[1]&511u][a]+m.vertices[t.packedVertices[2]&511u][a])/3.f;
 for(unsigned q=0;q<6;++q){Vec start{(b[0]+b[3])*.5f,(b[1]+b[4])*.5f,(b[2]+b[5])*.5f},delta{};if(q==0){start[2]=b[5]+128;delta={0,0,-256};}if(q==1){start[2]=b[2]-128;delta={0,0,256};}if(q==2||q==3)for(size_t a=0;a<3;++a){const float sign=q==2?1.f:-1.f;start[a]=centroid[a]+sign*64*t.normal[a];delta[a]=-sign*128*t.normal[a];}if(q==4)delta={-1e-7f,0,-0.f};if(q==5)for(size_t a=0;a<3;++a){start[a]+=128;delta[a]=-256;}
 for(unsigned variant=0;variant<4;++variant)for(uint32_t mask:{0u,1u,0x2080u,0xffffffffu})for(bool compute:{false,true})for(bool available:{true,false})for(bool stale:{false,true}){if(!available&&q!=0)continue;auto ray=query(start,q==4?Vec{-1,0,0}:delta,variant);if(q==4){ray.delta=delta;ray.swept=true;}record(meshes,prepared,ray,mask,compute,available,stale,0,index,q,variant,0,nativeWorld);}
 }}
#ifndef _WIN64
 observer.configure(false);
#endif
 }
 header[2]=cases;
 auto meshes=mixedPointMeshes();auto prepared=pointTables(world,meshes);
 for(unsigned profile=0;profile<4;++profile){mixedPointWorld(world,profile,prepared);if(profile==2)world.planes[0]={{.6f,.8f,0},0,3};
#ifndef _WIN64
 pointObjectsPrepare(meshes,prepared);observer.configure(true);NativeWorld nativeWorld(world);
#else
 int nativeWorld=0;
#endif
 for(unsigned q=0;q<11;++q){Vec start{},delta{};switch(q){case 0:start={0,0,64};delta={0,0,-128};break;case 1:start={0,0,-64};delta={0,0,128};break;case 2:start={-32,48,8};delta={64,-96,0};break;case 3:start={32,-48,24};delta={-64,96,-32};break;case 4:start={0,48,36};delta={0,-96,0};break;case 5:start={0,-48,36};delta={0,96,-24};break;case 6:start={-32,0,0};delta={96,0,0};break;case 7:start={32,0,0};delta={-96,0,0};break;case 8:start={.000244140625f,0,64};delta={0,0,-128};break;case 9:start={0,.0001220703125f,48};delta={0,0,-96};break;case 10:start={-0.f,-0.f,0};delta={-0.f,2,-3};break;}
 for(unsigned variant=0;variant<4;++variant)for(uint32_t mask:{0u,1u,0x80u,0x2080u,0xffffffffu})for(bool compute:{false,true})for(bool available:{true,false})for(bool stale:{false,true})for(unsigned disabled=0;disabled<2;++disabled){meshes[1].runtimeFlags=disabled?8u:0u;
#ifndef _WIN64
 at<uint32_t>(pointObjects[1].data(),0x2c)=meshes[1].runtimeFlags;
#endif
 record(meshes,prepared,query(start,delta,variant),mask,compute,available,stale,profile+1,q,q,variant,disabled,nativeWorld);}
 }
 if(profile==3){for(unsigned flags=0;flags<4;++flags)for(bool compute:{false,true}){SourceServerHullRayCandidate ray;ray.isRay=(flags&1)!=0;ray.swept=(flags&2)!=0;record(meshes,prepared,ray,0xffffffffu,compute,false,false,5,0,0,flags,0,nativeWorld);++header[4];}
 for(unsigned variant=0;variant<4;++variant)for(uint32_t mask:{0u,0xffffffffu})for(bool compute:{false,true})for(bool available:{true,false})for(bool stale:{false,true}){SourceServerHullRayCandidate ray;ray.isRay=ray.swept=true;ray.extents=variant&1?Vec{.000732421875f,.000244140625f,0}:Vec{};if(variant==0){ray.center={-0.f,-0.f,0};ray.offset={-0.f,0,-0.f};ray.delta={-0.f,2,-3};}if(variant==1){ray.center={100000000,-100000000,17};ray.offset={-100000000,100000000,-17};ray.delta={1,-3,.25f};}if(variant==2){ray.center={-2,3,-4};ray.offset={.5f,-.25f,1};ray.delta={5,-6,7};}if(variant==3){ray.center={.03125f,-.03125f,2};ray.offset={-.0625f,.0625f,-2};ray.delta={.125f,.25f,-.5f};}record(meshes,prepared,ray,mask,compute,available,stale,6,0,0,variant,0,nativeWorld);++header[6];}}
#ifndef _WIN64
 observer.configure(false);
#endif
 }

 // Authored single leaf with no brushes isolates analytic horizontal/sloped
 // terrain hits, one-sided rejection, contents eligibility and runtime flags.
 mixedPointWorld(world,3,prepared);world.data.leafBrushes={{}};prepared.refs={{0,1,2,3}};
#ifndef _WIN64
 pointObjectsPrepare(meshes,prepared);observer.configure(true);NativeWorld analyticWorld(world);
#else
 int analyticWorld=0;
#endif
 for(unsigned q=0;q<3;++q){const Vec start=q==2?Vec{100,100,64}:q==1?Vec{0,0,-64}:Vec{0,0,64};const Vec delta=q==1?Vec{0,0,128}:Vec{0,0,-128};
 for(unsigned variant=0;variant<4;++variant)for(uint32_t mask:{0u,1u,0x80u,0x20u,0x4000u,0xffffffffu})for(bool compute:{false,true})for(bool available:{true,false})for(bool stale:{false,true})for(unsigned disabled=0;disabled<2;++disabled){meshes[1].runtimeFlags=disabled?8u:0u;
#ifndef _WIN64
 at<uint32_t>(pointObjects[1].data(),0x2c)=meshes[1].runtimeFlags;
#endif
 record(meshes,prepared,query(start,delta,variant),mask,compute,available,stale,7,q,q,variant,disabled,analyticWorld);}}
#ifndef _WIN64
 observer.configure(false);
#endif
 header[1]=cases;header[3]=cases-header[2]-header[4]-header[6];out.seekp(0);out.write(reinterpret_cast<const char*>(header.data()),sizeof(header));out.flush();require(bool(out),"public point output finish");
#ifndef _WIN64
 image.verify();
#endif
 require(hits&&solid&&allSolid&&empty&&positionsOff&&tiny&&offsets&&rounded&&staleCount&&terrain&&brush&&negativeZero,"public swept point coverage");std::cout<<"{\"cases\":"<<cases<<",\"benchmark_cases\":"<<header[2]<<",\"controlled_cases\":"<<header[3]<<",\"empty_flag_cases\":"<<header[4]<<",\"provided_ray_cases\":"<<header[6]<<",\"hits\":"<<hits<<",\"start_solid\":"<<solid<<",\"all_solid\":"<<allSolid<<",\"empty_worlds\":"<<empty<<",\"positions_disabled\":"<<positionsOff<<",\"tiny_extents\":"<<tiny<<",\"nonzero_offsets\":"<<offsets<<",\"rounded_deltas\":"<<rounded<<",\"stale_parent_storage\":"<<staleCount<<",\"zero_masks\":"<<zeroMasks<<",\"terrain_names\":"<<terrain<<",\"brush_names\":"<<brush<<",\"negative_zero_positions\":"<<negativeZero;
#ifndef _WIN64
 std::cout<<",\"native_setup_observations\":"<<setupObservations<<",\"native_leaf_observations\":"<<leafObservations<<",\"native_finish_observations\":"<<finishObservations;
#endif
 std::cout<<",\"passed\":true}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

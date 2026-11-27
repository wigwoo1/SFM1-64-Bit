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
#include "source_prepared_world_hull_candidate.h"
#include "source_displacement_grid_triangles_candidate.h"

using Vec=std::array<float,3>;using Row=std::array<uint32_t,16>;
void require(bool v,const char* s){if(!v)throw std::runtime_error(s);}
uint32_t bits(float f){uint32_t u;std::memcpy(&u,&f,4);return u;}
template<class T>T& at(uint8_t* p,size_t n){return *reinterpret_cast<T*>(p+n);}
std::vector<uint8_t> read(const char* p){std::ifstream f(p,std::ios::binary);require(bool(f),"BSP read");return {(std::istreambuf_iterator<char>(f)),{}};}
std::vector<uint32_t> identities(){std::vector<uint32_t> ids(32768);for(uint32_t i=0;i<ids.size();++i)ids[i]=i;return ids;}
bool excludedSurface(uint32_t id,unsigned flags){return id==65535?(flags&2)!=0:(flags&1)&&((id&1)!=0);}
bool excludedBrush(const SourceBspBrushOnlyTraceCandidate& world,uint32_t index,unsigned flags){const auto& r=world.records.at(index);if(r.optimized){for(unsigned a=0;a<6;++a){uint16_t id;std::memcpy(&id,r.box.data()+32+2*a,2);if(excludedSurface(id,flags))return true;}}else{const auto& b=world.data.brushes.at(index);for(uint32_t a=0;a<b.count;++a){const auto texture=world.data.sides.at(b.first+a).texture;if(excludedSurface(texture<0?65535:uint32_t(texture),flags))return true;}}return false;}
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
 explicit NativeWorld(const SourceBspBrushOnlyTraceCandidate& w):source(w),surfaces(32768){
  static_assert(sizeof(SourceLineWalkPlaneCandidate)==20&&sizeof(NativeNode)==12&&sizeof(NativeLeaf)==16&&sizeof(NativeSide)==8,"native record widths");
  for(const auto& n:w.nodes)nodes.push_back({&w.planes.at(n.plane),n.children});
  for(const auto& s:w.data.sides)sides.push_back({&w.data.planes.at(s.plane),uint16_t(s.texture),uint16_t(s.bevel)});
  for(size_t i=0;i<w.records.size();++i){boxes.push_back({w.records[i].box});const auto& b=w.data.brushes.at(i);require(b.first<65536&&b.count<65535&&i<65536,"runtime span");brushes.push_back({b.contents,w.records[i].optimized?(uint32_t(i)<<16)|65535:(b.first<<16)|b.count});}
  for(const auto& row:w.data.leafBrushes){require(refs.size()<65536&&row.size()<65536,"leaf span");leaves.push_back({0,0,uint16_t(refs.size()),uint16_t(row.size()),0,0});for(auto i:row){require(i<65536,"brush reference");refs.push_back(uint16_t(i));}}
  const auto p=reinterpret_cast<uintptr_t>(fields.data());address<uintptr_t>(p)=reinterpret_cast<uintptr_t>(nodes.data());address<uintptr_t>(p+0x68)=reinterpret_cast<uintptr_t>(sides.data());address<uintptr_t>(p+0x70)=reinterpret_cast<uintptr_t>(boxes.data());address<uintptr_t>(p+0x88)=reinterpret_cast<uintptr_t>(leaves.data());address<uintptr_t>(p+0x98)=reinterpret_cast<uintptr_t>(refs.data());address<uintptr_t>(p+0xa8)=reinterpret_cast<uintptr_t>(brushes.data());address<uintptr_t>(p+0x218)=reinterpret_cast<uintptr_t>(surfaces.data());
 }
};
#endif
void synthetic(SourceBspBrushOnlyTraceCandidate& w){
 w.data.planes.clear();w.data.sides.clear();w.data.brushes.clear();const std::array<Vec,8> normals{Vec{1,0,0},Vec{-1,0,0},Vec{0,1,0},Vec{0,-1,0},Vec{0,0,1},Vec{0,0,-1},Vec{.6f,.8f,0},Vec{-.5f,.5f,.70710677f}};
 const std::array<uint32_t,8> contents{1,0x80,0x81,0x2080,0x80,1,0x4000,0x80};
 for(unsigned b=0;b<8;++b){const float center=float(int(b%3)-1)*32;const auto first=uint32_t(w.data.sides.size()),count=b<4?6u:8u;for(unsigned i=0;i<count;++i){const float distance=12+normals[i][0]*center;const auto index=uint16_t(w.data.planes.size());w.data.planes.push_back(sourceBspCollisionPlaneCandidate(normals[i],distance,i/2<3?i/2:3));w.data.sides.push_back({index,int16_t(b==3||b==7?-1:b),-1,int16_t(i>=6)});}w.data.brushes.push_back({first,count,contents[b]});}
 w.records=sourceBspCollisionRecordsCandidate(w.data,identities());w.planes={{{1,0,0},0,0},{{0,1,0},0,1},{{0,0,1},0,2}};w.nodes={{0,{1,2}},{1,{-1,-2}},{2,{-3,-4}}};w.root=0;
 w.data.leafBrushes={{0,1,0,2,3,4,5,6,7},{7,6,5,4,3,2,1,0,7},{0,2,4,6,0,1,3,5,7},{1,1,3,3,5,5,7,0,2,4,6}};
}
SourcePreparedWorldHullStateCandidate initial(unsigned mode){
 SourcePreparedWorldHullStateCandidate s;s.trace.fraction=mode%3==0?1.f:mode%3==1?.25f:0.f;if(mode>=6)s.trace.fraction=1;
 s.trace.leftSolid=mode&2?.75f:0.f;s.trace.startSolid=mode==6;s.trace.allSolid=mode==7;s.trace.contents=0x12345678;s.trace.plane={9,8,7,6};s.trace.planeTags={5,4,3,2};s.trace.surface=mode<3?SourceCollisionSurfaceCandidate{SourceCollisionSurfaceKindCandidate::TextureInfo,77}:SourceCollisionSurfaceCandidate{SourceCollisionSurfaceKindCandidate::Displacement,3};s.displacementFlags=0xbeef;s.displacementHit=mode<3?0u:0xffffffffu;s.surfaceProperty=0xabcd;s.surfaceFlags=0xef01;return s;
}
std::array<uint16_t,2> details(const SourceCollisionSurfaceCandidate& surface,unsigned flags){const auto id=surface.kind==SourceCollisionSurfaceKindCandidate::Default?65535u:surface.index;return {uint16_t(id==65535?0x3344:1000+id),uint16_t(excludedSurface(id,flags)?0x80:0)};}
Row row(const SourcePreparedWorldHullStateCandidate& s){
 Row r{bits(s.trace.fraction),bits(s.trace.leftSolid),s.trace.startSolid,s.trace.allSolid,s.trace.contents};for(size_t a=0;a<4;++a)r[5+a]=bits(s.trace.plane[a]);std::memcpy(&r[9],s.trace.planeTags.data(),4);r[10]=s.trace.surface.kind==SourceCollisionSurfaceKindCandidate::Default?0u:s.trace.surface.kind==SourceCollisionSurfaceKindCandidate::Displacement?2u:1u;r[11]=r[10]?s.trace.surface.index:0u;r[12]=s.displacementFlags;r[13]=s.displacementHit;r[14]=s.surfaceProperty;r[15]=s.surfaceFlags;return r;
}
Row owned(const SourceBspBrushOnlyTraceCandidate& world,const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,const std::vector<SourceDisplacementHullCacheCandidate>& caches,const std::vector<std::vector<uint32_t>>& refs,const std::vector<SourceLeafDisplacementCullCandidate>& culls,const std::vector<std::array<uint16_t,2>>& properties,const Vec& start,const Vec& delta,const Vec& extents,uint32_t mask,unsigned mode,unsigned flags){
 auto s=initial(mode);sourcePreparedWorldHullCandidate(world,meshes,caches,refs,culls,properties,start,delta,extents,mask,s,[&](uint32_t index){return excludedBrush(world,index,flags);},[&](const SourceCollisionSurfaceCandidate& surface){return details(surface,flags);});return row(s);
}
void layout(SourceBspBrushOnlyTraceCandidate& w,unsigned profile,std::vector<std::vector<uint32_t>>& refs){
 synthetic(w);w.planes={{{0,1,0},0,1},{{1,0,0},0,0},{{1,0,0},0,0}};
 if(profile==0)refs={{0,1,0,2,3},{3,2,1,0,1},{0,1,2,3,0},{1,0,1,2,3}};
 if(profile==1){w.data.leafBrushes={{},{},{1,1},{1}};refs={{1,0,1},{1,0},{2,3},{3,2}};const auto first=w.data.brushes[1].first;w.data.planes[w.data.sides[first+4].plane].distance=40;w.data.planes[w.data.sides[first+5].plane].distance=-32;w.records=sourceBspCollisionRecordsCandidate(w.data,identities());}
 if(profile==2){w.data.leafBrushes={{1,1},{1},{},{}};refs={{2,3},{3,2},{1,0,1},{1,0}};}
}
#ifndef _WIN64
alignas(16)std::array<std::array<uint8_t,244>,4> nativeObjects{};
std::array<std::vector<Vec>,4> storedPlanes;std::array<std::vector<std::array<uint16_t,9>>,4> storedIndices;
std::array<std::array<uint8_t,48>,4> nativeCulls{};
bool queryPhase=false;uintptr_t activeWork=0;int openCache=-1;uint32_t currentCandidate=UINT32_MAX,selectedIndex=UINT32_MAX;float nativeBeforeFraction=1,initialFraction=1;bool candidateHadBrush=false,caseHadDisp=false,caseBrushThenDisp=false;unsigned nativeDispThenBrush=0,nativeBrushThenDisp=0;
void boundary(unsigned slot,CONTEXT& c){if(!queryPhase){allocationBoundary(slot,c);return;}using O=PrivateX86BoundaryObserverFixture;auto* work=reinterpret_cast<uint8_t*>(activeWork);
 if(slot==0){const auto base=reinterpret_cast<uintptr_t>(nativeObjects.data());require(c.Ecx==activeWork&&c.Edx>=base&&(c.Edx-base)%244==0&&(c.Edx-base)/244<4,"world displacement identity");currentCandidate=uint32_t((c.Edx-base)/244);nativeBeforeFraction=at<float>(work,0x80);candidateHadBrush=at<uint32_t>(work,0x90)!=0x10350ee0u&&nativeBeforeFraction<initialFraction;c.EFlags|=0x10000u;}
 else if(slot==1){require(c.Esi==activeWork&&currentCandidate<4,"world displacement return boundary");if(at<float>(work,0x80)<nativeBeforeFraction){selectedIndex=currentCandidate;caseHadDisp=true;if(candidateHadBrush)caseBrushThenDisp=true;}currentCandidate=UINT32_MAX;c.EFlags|=0x10000u;}
 else if(slot==2){const auto base=reinterpret_cast<uintptr_t>(nativeObjects.data());require(openCache==-1&&c.Ecx>=base&&(c.Ecx-base)%244==0&&(c.Ecx-base)/244<4,"world prepared cache acquire");openCache=int((c.Ecx-base)/244);++acquires;O::returnCdecl(c);}
 else if(slot==3){require(openCache>=0&&c.Ecx==0x10643680&&address<uint32_t>(c.Esp+4)==0x1234u+uint32_t(openCache),"world prepared cache release");openCache=-1;++releases;O::returnCdecl(c);c.Esp+=4;}
 else throw std::runtime_error("unexpected world boundary");
}
void prepareObjects(const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,const std::vector<SourceDisplacementHullCacheCandidate>& caches,const std::vector<std::array<uint16_t,2>>& properties,const std::vector<SourceLeafDisplacementCullCandidate>& culls,unsigned buckets,bool masked){
 for(size_t i=0;i<4;++i){const auto& input=meshes.at(i);std::array<std::array<uint16_t,9>,512> indices{};prepare(input.power,buckets,masked,input.vertices,input.triangles,input.tree,caches.at(i),indices);storedPlanes[i].assign(nativePlanes.begin(),nativePlanes.begin()+caches[i].planes.size());storedIndices[i].assign(indices.begin(),indices.begin()+input.triangles.size());nativeObjects[i]=mesh;auto* object=nativeObjects[i].data();at<uintptr_t>(object,0xb0)=reinterpret_cast<uintptr_t>(storedIndices[i].data());at<uintptr_t>(object,0xc4)=reinterpret_cast<uintptr_t>(storedPlanes[i].data());at<uint32_t>(object,0x24)=0x1234u+uint32_t(i);at<uint32_t>(object,0x20)=input.contents;at<uint32_t>(object,0x2c)=input.runtimeFlags;std::memcpy(object+0x6c,properties[i].data(),4);auto* c=nativeCulls[i].data();nativeCulls[i].fill(0);std::memcpy(c,culls[i].mins.data(),12);std::memcpy(c+16,culls[i].maxs.data(),12);at<uint32_t>(c,32)=culls[i].visitIndex;at<uint32_t>(c,36)=culls[i].contents;}
 address<uintptr_t>(0x10637820)=reinterpret_cast<uintptr_t>(nativeObjects.data());address<uintptr_t>(0x10637824)=reinterpret_cast<uintptr_t>(nativeCulls.data());
}
Row native(NativeWorld& world,const std::vector<std::vector<uint32_t>>& refs,const Vec& start,const Vec& delta,const Vec& extents,uint32_t mask,unsigned mode,unsigned flags){
 for(uint32_t i=0;i<world.surfaces.size();++i){const auto d=details({SourceCollisionSurfaceKindCandidate::RuntimeSurface,i},flags);world.surfaces[i]={0x10000000u+i,uint32_t(d[0])|(uint32_t(d[1])<<16)};}const auto defaultDetails=details({},flags);address<uint32_t>(0x10402fe8)=0xffffffffu;address<uint32_t>(0x10402fec)=uint32_t(defaultDetails[0])|(uint32_t(defaultDetails[1])<<16);
 std::vector<uint16_t> dispRefs;for(size_t leaf=0;leaf<refs.size();++leaf){world.leaves[leaf].dispFirst=uint16_t(dispRefs.size());world.leaves[leaf].dispCount=uint16_t(refs[leaf].size());for(auto id:refs[leaf])dispRefs.push_back(uint16_t(id));}at<uintptr_t>(world.fields.data(),0xb0)=reinterpret_cast<uintptr_t>(dispRefs.data());
 alignas(16)std::array<uint8_t,0x220> work{};auto* w=work.data();std::vector<uint32_t> brushVisits(world.brushes.size()),dispVisits(4);for(size_t a=0;a<3;++a){at<float>(w,4*a)=start[a];at<float>(w,12+4*a)=start[a]+delta[a];at<float>(w,0x30+4*a)=extents[a];at<float>(w,0x3c+4*a)=delta[a];at<float>(w,0x48+4*a)=delta[a]==0?std::numeric_limits<float>::max():1.f/delta[a];}
 const auto s=initial(mode);std::memcpy(w+0x6c,s.trace.plane.data(),16);std::memcpy(w+0x7c,s.trace.planeTags.data(),4);at<float>(w,0x80)=s.trace.fraction;at<uint32_t>(w,0x84)=s.trace.contents;at<uint16_t>(w,0x88)=s.displacementFlags;w[0x8a]=uint8_t(s.trace.allSolid);w[0x8b]=uint8_t(s.trace.startSolid);at<float>(w,0x8c)=s.trace.leftSolid;at<uint32_t>(w,0x90)=s.trace.surface.kind==SourceCollisionSurfaceKindCandidate::Displacement?0x10350ee0u:0x10000000u+s.trace.surface.index;at<uint16_t>(w,0x94)=s.surfaceProperty;at<uint16_t>(w,0x96)=s.surfaceFlags;at<uint32_t>(w,0x114)=s.displacementHit;at<uintptr_t>(w,0x104)=reinterpret_cast<uintptr_t>(world.fields.data());at<uint32_t>(w,0xfc)=mask;w[0x101]=1;at<uint32_t>(w,0x120)=1;at<uintptr_t>(w,0x128)=reinterpret_cast<uintptr_t>(brushVisits.data());at<uintptr_t>(w,0x150)=reinterpret_cast<uintptr_t>(dispVisits.data());
 activeWork=reinterpret_cast<uintptr_t>(w);openCache=-1;currentCandidate=UINT32_MAX;selectedIndex=s.trace.surface.kind==SourceCollisionSurfaceKindCandidate::Displacement?s.trace.surface.index:UINT32_MAX;initialFraction=s.trace.fraction;caseHadDisp=caseBrushThenDisp=false;acquires=releases=0;
 using Walk=void(__fastcall*)(void*,int,float,float,const float*,const float*);reinterpret_cast<Walk>(0x10148c30)(w,world.source.root,0,1,reinterpret_cast<const float*>(w),reinterpret_cast<const float*>(w+12));require(openCache==-1&&acquires==releases&&currentCandidate==UINT32_MAX,"world cache/boundary balance");
 Row r{bits(at<float>(w,0x80)),bits(at<float>(w,0x8c)),w[0x8b],w[0x8a],at<uint32_t>(w,0x84)};std::memcpy(r.data()+5,w+0x6c,20);const auto name=at<uint32_t>(w,0x90);if(name==0x10350ee0u){require(selectedIndex<4,"world displacement surface ordinal");r[10]=2;r[11]=selectedIndex;}else if(name==0xffffffffu){r[10]=0;r[11]=0;}else{require(name>=0x10000000u&&name<0x10008000u,"world brush surface name");r[10]=1;r[11]=name-0x10000000u;}r[12]=at<uint16_t>(w,0x88);r[13]=at<uint32_t>(w,0x114);r[14]=at<uint16_t>(w,0x94);r[15]=at<uint16_t>(w,0x96);nativeDispThenBrush+=caseHadDisp&&r[10]!=2&&r[13]==0;nativeBrushThenDisp+=caseBrushThenDisp;return r;
}
#endif
int main(int argc,char** argv){try{
 require(argc==5,"expected engine or -, BSP, private permutation and disposable output");std::array<uint32_t,256> raw{};std::ifstream fixture(argv[3],std::ios::binary);fixture.read(reinterpret_cast<char*>(raw.data()),sizeof(raw));require(fixture.gcount()==sizeof(raw)&&fixture.peek()==std::char_traits<char>::eof(),"private permutation fixture");SourcePlaneHashPermutationCandidate permutation{};for(size_t i=0;i<raw.size();++i){require(raw[i]<256,"permutation value");permutation[i]=uint8_t(raw[i]);}
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1],0x643eec);image.seal();require(std::memcmp(raw.data(),reinterpret_cast<void*>(0x104129b0),1024)==0,"permutation identity");PrivateX86BoundaryObserverFixture observer{{0x1015b150,0x10001610,0x1015c760,0x10261300},boundary};
#endif
 std::ofstream out(argv[4],std::ios::binary);require(bool(out),"record output");SourceBspBrushOnlyTraceCandidate world(read(argv[2]),identities());unsigned cases=0,grids=0,changed=0,brushHits=0,dispHits=0,solid=0,cleared=0,rounded=0;uint32_t random=0x439abc;auto sample=[&](){random=random*1664525u+1013904223u;return float(int((random>>8)%4097)-2048)/16.f;};
 for(unsigned power=1;power<=4;++power)for(unsigned shape=0;shape<4;++shape){const unsigned cells=1u<<power,width=cells+1,count=2*cells*cells,buckets=shape%2?17u:64u;const bool masked=shape%2==0;
  auto orient=[&](Vec v){if(shape==3)std::swap(v[0],v[2]);return v;};std::vector<SourceDisplacementRuntimeMeshCandidate> meshes(4);std::vector<SourceDisplacementHullCacheCandidate> caches;std::vector<SourceLeafDisplacementCullCandidate> culls;std::vector<std::array<uint16_t,2>> properties;
  for(unsigned id=0;id<4;++id){auto& input=meshes[id];input.power=power;input.contents=std::array<uint32_t,4>{1,0x80,0x4000,0x20}[id];input.runtimeFlags=id==1?8u:id==2?4u:0u;
   for(unsigned y=0;y<width;++y)for(unsigned x=0;x<width;++x){const float z=(shape==0?0.f:shape==1?float(x)*.75f-float(y)*.25f:shape==2?float((x*y)%7):float((x+y)%3)*.03125f)+float(id)*24;Vec vertex{float(x)*8-16,float(y)*8+3,z};if(id==3)vertex[0]+=2000;input.vertices.push_back(orient(vertex));}
   auto source=sourceDisplacementGridTrianglesCandidate(power,std::vector<uint16_t>(count));std::vector<std::array<uint16_t,3>> packed;for(auto t:source){packed.push_back(t.vertices);SourceDisplacementCollisionTriangleCandidate c;c.packedVertices=t.vertices;c.flags=uint16_t((input.triangles.size()%32)<<6);input.triangles.push_back(sourceDisplacementCollisionTriangleCandidate(input.vertices,c));}input.tree=sourceDisplacementCollisionTreeCandidate(power,input.vertices,packed);caches.push_back(sourceDisplacementHullCacheCandidate(power,input.vertices,input.triangles,permutation,buckets,masked));const auto& bounds=input.tree.expandedBounds;culls.push_back({{bounds[0],bounds[1],bounds[2]},{bounds[3],bounds[4],bounds[5]},std::array<uint32_t,4>{1,0x80,0x2080,1}[id],std::array<uint32_t,4>{2,0,3,1}[id]});properties.push_back({uint16_t(100+4*grids+id),uint16_t(500+4*grids+id)});
  }
#ifndef _WIN64
  queryPhase=false;observer.entries={0x1015b150,0x10001610,0x1015c760,0x10261300};observer.configure(true);prepareObjects(meshes,caches,properties,culls,buckets,masked);observer.configure(false);queryPhase=true;observer.entries={0x1014d100,0x10148831,0x1015c760,0x10261300};observer.configure(true);
#endif
  ++grids;
  for(unsigned profile=0;profile<3;++profile){std::vector<std::vector<uint32_t>> refs;layout(world,profile,refs);
#ifndef _WIN64
   NativeWorld nativeWorld(world);
#endif
   for(unsigned qid=0;qid<24;++qid){const float cx=float(cells)*4-16,cy=float(cells)*4+3;Vec start{sample(),sample(),sample()},delta{sample(),sample(),sample()};switch(qid%8){case 0:start={cx,40,80};delta={0,-80,-160};break;case 1:start={cx,cy,80};delta={0,0,-160};break;case 2:start={-56,cy,0};delta={float(cells)*8+80,0,0};break;case 3:start={cx,cy,0};delta={0,-0.f,0};break;case 4:break;case 5:start={-8,11,.03125f};delta={.0625f,.03125f,-.0625f};break;case 6:start={-300,-300,80};delta={0,0,-160};break;case 7:start={-56,cy-40,60};delta={float(cells)*8+80,80,-120};break;}
    if(qid==17){start={12,0,0};delta={-1e-7f,0,0};}start=orient(start);delta=orient(delta);const unsigned flags=qid%4;const Vec extents=qid%4==0?Vec{2,24,2}:qid%4==1?Vec{24,24,41}:qid%4==2?Vec{.125f,2.75f,7.0625f}:Vec{};
    bool roundedDelta=false;for(size_t axis=0;axis<3;++axis){const float end=start[axis]+delta[axis];roundedDelta|=end-start[axis]!=delta[axis];}
    for(uint32_t mask:{0u,1u,0x80u,0x2080u,0x2081u,0x4000u,0xffffffffu})for(unsigned mode=0;mode<8;++mode){
     const auto expected=owned(world,meshes,caches,refs,culls,properties,start,delta,extents,mask,mode,flags);auto result=expected;
#ifndef _WIN64
     result=native(nativeWorld,refs,start,delta,extents,mask,mode,flags);if(result!=expected){std::cerr<<"case "<<cases<<" power "<<power<<" shape "<<shape<<" profile "<<profile<<" q "<<qid<<" mask "<<mask<<" mode "<<mode<<"\n";for(size_t i=0;i<result.size();++i)if(result[i]!=expected[i])std::cerr<<"field "<<i<<" native "<<result[i]<<" owned "<<expected[i]<<"\n";throw std::runtime_error("mixed prepared world hull mismatch");}
#endif
     out.write(reinterpret_cast<const char*>(result.data()),sizeof(result));require(bool(out),"record write");++cases;changed+=result[0]!=bits(initial(mode).trace.fraction);brushHits+=result[10]==1&&result[0]!=bits(initial(mode).trace.fraction);dispHits+=result[10]==2&&result[0]!=bits(initial(mode).trace.fraction);solid+=result[2]!=0;cleared+=initial(mode).displacementHit!=0&&result[13]==0;rounded+=roundedDelta;
    }
   }
  }
#ifndef _WIN64
  observer.configure(false);for(size_t i=0;i<4;++i){require(storedPlanes[i].size()==caches[i].planes.size()&&storedIndices[i]==caches[i].indices,"leaf sweep cache coverage/indices");require(storedPlanes[i].empty()||std::memcmp(storedPlanes[i].data(),caches[i].planes.data(),storedPlanes[i].size()*12)==0,"leaf sweep modified plane bytes");}
#endif
 }
#ifndef _WIN64
 require(nativeDispThenBrush&&nativeBrushThenDisp,"native mixed winner transfer coverage");image.verify();
#endif
 require(changed&&brushHits&&dispHits&&solid&&cleared&&rounded,"mixed world coverage");std::cout<<"{\"cases\":"<<cases<<",\"grids\":"<<grids<<",\"changed_fraction\":"<<changed<<",\"brush_winners\":"<<brushHits<<",\"displacement_winners\":"<<dispHits<<",\"start_solid\":"<<solid<<",\"marker_cleared\":"<<cleared<<",\"rounded_delta_cases\":"<<rounded;
#ifndef _WIN64
 std::cout<<",\"native_displacement_then_brush\":"<<nativeDispThenBrush<<",\"native_brush_then_displacement\":"<<nativeBrushThenDisp;
#endif
 std::cout<<",\"passed\":true}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

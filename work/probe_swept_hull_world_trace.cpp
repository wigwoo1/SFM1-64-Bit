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
void boundary(unsigned slot,CONTEXT& c){if(!queryPhase){allocationBoundary(slot,c);return;}using O=PrivateX86BoundaryObserverFixture;auto* work=reinterpret_cast<uint8_t*>(activeWork);
 if(slot==0&&c.Eip==0x10149230){require(++poolAcquires==1&&address<uint32_t>(c.Esp)==0x10149ebb,"work acquire boundary");O::returnCdecl(c,uint32_t(activeWork));slotZero(c,0x10148c30);}
 else if(slot==0&&c.Eip==0x10148c30){require(c.Ecx==activeWork&&activeRay,"wrapper work setup owner");const auto& ray=*activeRay;for(size_t a=0;a<3;++a){const float end=ray.center[a]+ray.delta[a],negative=ray.extents[a]*-1.f,inverse=ray.delta[a]==0?std::numeric_limits<float>::max():1.f/ray.delta[a];require(std::memcmp(work+4*a,&ray.center[a],4)==0&&std::memcmp(work+12+4*a,&end,4)==0&&std::memcmp(work+0x18+4*a,&negative,4)==0&&std::memcmp(work+0x24+4*a,&ray.extents[a],4)==0&&std::memcmp(work+0x30+4*a,&ray.extents[a],4)==0&&std::memcmp(work+0x3c+4*a,&ray.delta[a],4)==0&&std::memcmp(work+0x48+4*a,&inverse,4)==0,"wrapper geometry work fields");}require(at<uint32_t>(work,0xfc)==activeMask&&!work[0x100]&&work[0x101]==1&&at<uint32_t>(work,0x114)==0,"wrapper reset fields");SourceSweptHullWorldTraceCandidate reset;reset.state.surfaceProperty=0x3344;auto expected=pack(reset);require(std::memcmp(work+0x54,expected.data(),84)==0,"wrapper full trace reset");++setupObservations;slotZero(c,0x1014d100);c.EFlags|=0x10000u;}
 else if(slot==0&&c.Eip==0x1014d100){const auto base=reinterpret_cast<uintptr_t>(nativeObjects.data());require(c.Ecx==activeWork&&c.Edx>=base&&(c.Edx-base)%244==0&&(c.Edx-base)/244<nativeObjects.size(),"wrapper displacement identity");currentCandidate=uint32_t((c.Edx-base)/244);nativeBeforeFraction=at<float>(work,0x80);slotZero(c,0x10148831);c.EFlags|=0x10000u;}
 else if(slot==0&&c.Eip==0x10148831){require(c.Esi==activeWork&&currentCandidate<nativeObjects.size(),"wrapper displacement return");if(at<float>(work,0x80)<nativeBeforeFraction)selectedIndex=currentCandidate;currentCandidate=UINT32_MAX;slotZero(c,0x1014d100);c.EFlags|=0x10000u;}
 else if(slot==1){require(c.Ecx==0x10637798&&address<uintptr_t>(c.Esp+4)==activeWork-4&&address<int32_t>(activeWork+0x11c)==-1&&++poolReleases==1,"work release boundary");O::returnCdecl(c);c.Esp+=4;slotZero(c,0x10149230);}
 else if(slot==2){const auto base=reinterpret_cast<uintptr_t>(nativeObjects.data());require(openCache==-1&&c.Ecx>=base&&(c.Ecx-base)%244==0&&(c.Ecx-base)/244<nativeObjects.size(),"wrapper prepared cache acquire");openCache=int((c.Ecx-base)/244);++acquires;O::returnCdecl(c);}
 else if(slot==3){require(openCache>=0&&c.Ecx==0x10643680&&address<uint32_t>(c.Esp+4)==0x1234u+uint32_t(openCache),"wrapper prepared cache release");openCache=-1;++releases;O::returnCdecl(c);c.Esp+=4;}
 else throw std::runtime_error("unexpected wrapper boundary");
}
void prepareObjects(const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,const std::vector<SourceDisplacementHullCacheCandidate>& caches,const std::vector<std::array<uint16_t,2>>& properties,const std::vector<SourceLeafDisplacementCullCandidate>& culls,unsigned buckets,bool masked){
 nativeObjects.resize(meshes.size());storedPlanes.resize(meshes.size());storedIndices.resize(meshes.size());nativeCulls.resize(meshes.size());
 for(size_t i=0;i<meshes.size();++i){const auto& input=meshes.at(i);std::array<std::array<uint16_t,9>,512> indices{};prepare(input.power,buckets,masked,input.vertices,input.triangles,input.tree,caches.at(i),indices);storedPlanes[i].assign(nativePlanes.begin(),nativePlanes.begin()+caches[i].planes.size());storedIndices[i].assign(indices.begin(),indices.begin()+input.triangles.size());nativeObjects[i]=mesh;auto* object=nativeObjects[i].data();at<uintptr_t>(object,0xb0)=reinterpret_cast<uintptr_t>(storedIndices[i].data());at<uintptr_t>(object,0xc4)=reinterpret_cast<uintptr_t>(storedPlanes[i].data());at<uint32_t>(object,0x24)=0x1234u+uint32_t(i);at<uint32_t>(object,0x20)=input.contents;at<uint32_t>(object,0x2c)=input.runtimeFlags;std::memcpy(object+0x6c,properties[i].data(),4);auto* c=nativeCulls[i].data();nativeCulls[i].fill(0);std::memcpy(c,culls[i].mins.data(),12);std::memcpy(c+16,culls[i].maxs.data(),12);at<uint32_t>(c,32)=culls[i].visitIndex;at<uint32_t>(c,36)=culls[i].contents;}
 address<uintptr_t>(0x10637820)=reinterpret_cast<uintptr_t>(nativeObjects.data());address<uintptr_t>(0x10637824)=reinterpret_cast<uintptr_t>(nativeCulls.data());
}
std::array<uint8_t,0x50> rayBytes(const SourceServerHullRayCandidate& ray){std::array<uint8_t,0x50> bytes{};std::memcpy(bytes.data(),ray.center.data(),12);std::memcpy(bytes.data()+0x10,ray.delta.data(),12);std::memcpy(bytes.data()+0x20,ray.offset.data(),12);std::memcpy(bytes.data()+0x30,ray.extents.data(),12);bytes[0x44]=uint8_t(ray.isRay);bytes[0x45]=uint8_t(ray.swept);return bytes;}
Record native(NativeWorld& world,const std::vector<std::vector<uint32_t>>& refs,const SourceServerHullRayCandidate& ray,uint32_t mask,bool compute,bool available){
 for(uint32_t i=0;i<world.surfaces.size();++i)world.surfaces[i]={0x10000000u+i,uint32_t(1000+i)|(uint32_t(materialFlags[i])<<16)};address<uint32_t>(0x10402fe8)=0xffffffffu;address<uint32_t>(0x10402fec)=0x3344;
 std::vector<uint16_t> dispRefs;for(size_t leaf=0;leaf<refs.size();++leaf){require(dispRefs.size()<65536&&refs[leaf].size()<65536,"wrapper displacement leaf span");world.leaves[leaf].dispFirst=uint16_t(dispRefs.size());world.leaves[leaf].dispCount=uint16_t(refs[leaf].size());for(auto id:refs[leaf])dispRefs.push_back(uint16_t(id));}at<uintptr_t>(world.fields.data(),0xb0)=reinterpret_cast<uintptr_t>(dispRefs.data());at<uint32_t>(world.fields.data(),0x7c)=available?uint32_t(world.source.data.planes.size()):0;
 alignas(16)std::array<uint8_t,0x240> storage{};storage.fill(0xa7);auto* work=storage.data()+16;std::vector<uint32_t> brushVisits(world.brushes.size()),dispVisits(nativeObjects.size());at<uintptr_t>(work,0x104)=reinterpret_cast<uintptr_t>(world.fields.data());at<uint32_t>(work,0x114)=0;at<uint32_t>(work,0x11c)=0;at<uint32_t>(work,0x120)=1;at<uintptr_t>(work,0x128)=reinterpret_cast<uintptr_t>(brushVisits.data());at<uintptr_t>(work,0x150)=reinterpret_cast<uintptr_t>(dispVisits.data());
 alignas(16)auto input=rayBytes(ray);std::array<uint8_t,116> output;output.fill(0xa5);activeWork=reinterpret_cast<uintptr_t>(work);activeRay=&ray;activeMask=mask;poolAcquires=poolReleases=0;openCache=-1;currentCandidate=selectedIndex=UINT32_MAX;acquires=releases=0;
 using Wrapper=void(__cdecl*)(const void*,int32_t,uint32_t,bool,void*);reinterpret_cast<Wrapper>(0x10149eb0)(input.data(),world.source.root,mask,compute,output.data()+16);require(poolAcquires==1&&poolReleases==1&&openCache==-1&&acquires==releases,"wrapper service balance");for(size_t i=0;i<16;++i)require(output[i]==0xa5&&output[100+i]==0xa5,"wrapper output guard");Record result{};std::memcpy(result.data(),output.data()+16,84);const auto name=at<uint32_t>(result.data(),0x3c);if(name==0x10350ee0u){require(selectedIndex<nativeObjects.size(),"wrapper displacement ordinal");at<uint32_t>(result.data(),0x3c)=0x20000000u+selectedIndex;}else require(name==0xffffffffu||(name>=0x10000000u&&name<0x10000000u+materialFlags.size()),"wrapper surface namespace");at<uint32_t>(result.data(),84)=at<uint32_t>(work,0x114);return result;
}
Record nativeFinish(const SourceServerHullRayCandidate& ray,const SourceSweptHullWorldTraceCandidate& before){auto result=pack(before);alignas(16)auto input=rayBytes(ray);using Finish=void(__fastcall*)(void*,const void*);reinterpret_cast<Finish>(0x10145e20)(result.data(),input.data());return result;}

#endif

int main(int argc,char** argv){try{
 require(argc==5,"expected engine or -, BSP, private permutation and disposable output");const auto bsp=read(argv[2]);auto word=[&](size_t offset){require(offset<=bsp.size()&&bsp.size()-offset>=4,"BSP header word");uint32_t value;std::memcpy(&value,bsp.data()+offset,4);return value;};const auto texinfo=word(8+6*16),texinfoBytes=word(8+6*16+4),texdataBytes=word(8+2*16+4);require(texinfoBytes%72==0&&texdataBytes%32==0&&texinfo<=bsp.size()&&texinfoBytes<=bsp.size()-texinfo,"benchmark material lump spans");materialFlags.resize(texdataBytes/32);
 for(uint32_t i=0;i<texinfoBytes/72;++i){const auto id=word(texinfo+72*i+68);require(id<materialFlags.size(),"benchmark material binding");materialMapping.push_back(id);materialFlags[id]|=uint16_t(word(texinfo+72*i+64));}boundFlags=std::make_unique<SourceBspTraceSurfaceFlagsCandidate>(bsp,materialMapping);SourceBspBrushOnlyTraceCandidate world(bsp,materialMapping);
 std::array<uint32_t,256> raw{};std::ifstream fixture(argv[3],std::ios::binary);fixture.read(reinterpret_cast<char*>(raw.data()),sizeof(raw));require(fixture.gcount()==sizeof(raw)&&fixture.peek()==std::char_traits<char>::eof(),"private permutation fixture");SourcePlaneHashPermutationCandidate permutation{};for(size_t i=0;i<raw.size();++i){require(raw[i]<256,"permutation value");permutation[i]=uint8_t(raw[i]);}
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1],0x643eec);image.seal();require(std::memcmp(raw.data(),reinterpret_cast<void*>(0x104129b0),1024)==0,"permutation identity");PrivateX86BoundaryObserverFixture observer{{0x1015b150,0x10001610,0x1015c760,0x10261300},boundary};
#endif
 std::ofstream out(argv[4],std::ios::binary);require(bool(out),"record output");unsigned cases=0,hits=0,solid=0,positionsOff=0,emptyCases=0,offsetCases=0,rounded=0,totalTriangles=0,totalPlanes=0,totalRefs=0;
 for(unsigned hdrType=0;hdrType<2;++hdrType){const SourceBspDisplacementCollisionCandidate imported(bsp,int32_t(hdrType));const auto& meshes=imported.meshes;require(meshes.size()==43&&imported.selectedFaceLump==(hdrType?58u:7u),"benchmark displacement schema");auto tables=sourcePreparedHullWorldTablesCandidate(world,meshes,permutation,64,true);std::vector<std::array<uint16_t,2>> properties;unsigned triangles=0;
  for(size_t i=0;i<meshes.size();++i){properties.push_back({uint16_t(100+i),uint16_t(500+i)});triangles+=unsigned(meshes[i].triangles.size());totalPlanes+=unsigned(tables.caches[i].planes.size());}for(const auto& refs:tables.leafDisplacements)totalRefs+=unsigned(refs.size());require(triangles==5312,"benchmark triangle coverage");totalTriangles+=triangles;
  const std::array<uint32_t,4> header{hdrType,imported.selectedFaceLump,uint32_t(meshes.size()),triangles};out.write(reinterpret_cast<const char*>(header.data()),sizeof(header));
#ifndef _WIN64
  queryPhase=false;observer.entries={0x1015b150,0x10001610,0x1015c760,0x10261300};observer.configure(true);prepareObjects(meshes,tables.caches,properties,tables.culls,64,true);observer.configure(false);queryPhase=true;observer.entries={0x10149230,0x1000e020,0x1015c760,0x10261300};observer.configure(true);NativeWorld nativeWorld(world);
#endif
  uint32_t random=0x439abc;auto sample=[&](){random=random*1664525u+1013904223u;return float(int((random>>8)%4097)-2048)/8.f;};
  for(size_t index=0;index<meshes.size();++index){const auto& input=meshes[index];const auto& bounds=input.tree.expandedBounds;Vec center{};for(size_t a=0;a<3;++a)center[a]=(bounds[a]+bounds[a+3])*.5f;const auto& triangle=input.triangles[index%input.triangles.size()];Vec centroid{};for(size_t a=0;a<3;++a)centroid[a]=(input.vertices[triangle.packedVertices[0]&511][a]+input.vertices[triangle.packedVertices[1]&511][a]+input.vertices[triangle.packedVertices[2]&511][a])/3.f;
   for(unsigned qid=0;qid<6;++qid){Vec start=center,delta{};switch(qid){case 0:start[2]=bounds[5]+128;delta={0,0,-256};break;case 1:start[2]=bounds[2]-128;delta={0,0,256};break;case 2:for(size_t a=0;a<3;++a){start[a]=centroid[a]+64*triangle.normal[a];delta[a]=-128*triangle.normal[a];}break;case 3:for(size_t a=0;a<3;++a){start[a]+=128;delta[a]=-256;}break;case 4:delta={-1e-7f,0,-0.f};break;case 5:for(size_t a=0;a<3;++a){start[a]+=sample();delta[a]=sample();}break;}
    for(uint32_t mask:{0u,1u,0x2081u,0xffffffffu})for(unsigned variant=0;variant<4;++variant)for(bool compute:{false,true})for(bool available:{true,false}){if(!available&&qid!=0)continue;
     const std::array<Vec,4> minimum{Vec{-2,-24,-2},Vec{-24,-24,0},Vec{0,0,0},Vec{-2,-2,-2}},maximum{Vec{2,24,2},Vec{24,24,82},Vec{.25f,5.5f,14.125f},Vec{4,6,8}};Vec end{};for(size_t a=0;a<3;++a)end[a]=start[a]+delta[a];auto ray=sourceServerHullRayCandidate(start,end,minimum[variant],maximum[variant]);if(qid==4){ray.delta=delta;ray.swept=true;}require(ray.swept&&!ray.isRay,"admitted benchmark hull flags");const auto expected=owned(world,meshes,tables,properties,ray,mask,compute,available);auto result=expected;
#ifndef _WIN64
     result=native(nativeWorld,tables.leafDisplacements,ray,mask,compute,available);if(result!=expected){std::cerr<<"case "<<cases<<" hdr "<<hdrType<<" displacement "<<index<<" q "<<qid<<" mask "<<mask<<" variant "<<variant<<" compute "<<compute<<" available "<<available<<'\n';for(size_t i=0;i<result.size();++i)if(result[i]!=expected[i])std::cerr<<"byte "<<i<<" native "<<unsigned(result[i])<<" owned "<<unsigned(expected[i])<<'\n';throw std::runtime_error("full swept hull wrapper mismatch");}
#endif
     out.write(reinterpret_cast<const char*>(result.data()),result.size());require(bool(out),"record write");++cases;hits+=at<float>(result.data(),0x2c)<1;solid+=result[0x37]!=0;positionsOff+=!compute;emptyCases+=!available;offsetCases+=ray.offset!=Vec{};bool differs=false;for(size_t a=0;a<3;++a){const float roundedEnd=ray.center[a]+ray.delta[a];differs|=roundedEnd-ray.center[a]!=ray.delta[a];}rounded+=differs;
    }
   }
  }
#ifndef _WIN64
  observer.configure(false);for(size_t i=0;i<meshes.size();++i){require(storedPlanes[i].size()==tables.caches[i].planes.size()&&storedIndices[i]==tables.caches[i].indices,"wrapper cached indices changed");require(storedPlanes[i].empty()||std::memcmp(storedPlanes[i].data(),tables.caches[i].planes.data(),storedPlanes[i].size()*12)==0,"wrapper cached plane bytes changed");}
#endif
 }
 const std::array<uint32_t,4> finishHeader{2,0,36,0};out.write(reinterpret_cast<const char*>(finishHeader.data()),16);unsigned finishCases=0;
 for(unsigned variant=0;variant<4;++variant)for(float fraction:{0.f,.25f,1.f})for(float left:{0.f,.25f,1.f}){SourceServerHullRayCandidate ray;if(variant==0){ray.center={-0.f,-0.f,0};ray.offset={-0.f,0,-0.f};ray.delta={-0.f,2,-3};}if(variant==1){ray.center={100000000,-100000000,17};ray.offset={-100000000,100000000,-17};ray.delta={1,-3,.25f};}if(variant==2){ray.center={-2,3,-4};ray.offset={.5f,-.25f,1};ray.delta={5,-6,7};}if(variant==3){ray.center={.03125f,-.03125f,2};ray.offset={-.0625f,.0625f,-2};ray.delta={.125f,.25f,-.5f};}SourceSweptHullWorldTraceCandidate before;before.state.trace.fraction=fraction;before.state.trace.leftSolid=left;before.startPosition={91,92,93};before.endPosition={94,95,96};auto value=before;sourceHullWorldTracePositionsCandidate(ray,value);const auto expected=pack(value);auto result=expected;
#ifndef _WIN64
 result=nativeFinish(ray,before);require(result==expected,"direct coordinate finisher mismatch");
#endif
 out.write(reinterpret_cast<const char*>(result.data()),result.size());++finishCases;
 }
#ifndef _WIN64
 require(setupObservations>0,"wrapper setup observation coverage");image.verify();
#endif
 require(hits&&solid&&positionsOff&&emptyCases&&offsetCases&&rounded,"full wrapper coverage");std::cout<<"{\"cases\":"<<cases<<",\"finish_cases\":"<<finishCases<<",\"face_modes\":2,\"meshes_per_mode\":43,\"triangles_total\":"<<totalTriangles<<",\"planes_total\":"<<totalPlanes<<",\"leaf_refs_total\":"<<totalRefs<<",\"materials\":"<<materialFlags.size()<<",\"texinfo\":"<<materialMapping.size()<<",\"hits\":"<<hits<<",\"start_solid\":"<<solid<<",\"positions_disabled\":"<<positionsOff<<",\"empty_world\":"<<emptyCases<<",\"nonzero_offset\":"<<offsetCases<<",\"rounded_delta_cases\":"<<rounded<<",\"passed\":true}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

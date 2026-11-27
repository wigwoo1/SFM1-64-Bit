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
#include "source_displacement_world_hull_candidate.h"
#include "source_leaf_hull_displacement_policy_candidate.h"
#include "source_displacement_grid_triangles_candidate.h"

using Vec=std::array<float,3>;using Work=std::array<uint8_t,0x220>;
void require(bool v,const char* s){if(!v)throw std::runtime_error(s);}
template<class T>T& at(uint8_t* p,size_t n){return *reinterpret_cast<T*>(p+n);}
template<class T>T at(const uint8_t* p,size_t n){T value;std::memcpy(&value,p+n,sizeof(value));return value;}
struct Query {Vec start{},delta{},inverse{},extents{};unsigned mode=0,policy=0;uint32_t mask=0,generation=13;bool swept=true,brushes=false,startSolid=false,allSolid=false,priorSurface=false;uint32_t marker=0;std::vector<uint32_t> ordered,visits;};
struct Record {Work work{};std::vector<uint32_t> visits,calls;uint32_t brushVisit=0;};
Record initial(const Query& q){Record r;r.work.fill(0xa5);r.visits=q.visits;r.brushVisit=q.generation^0x12345u;auto* w=r.work.data();std::memcpy(w,q.start.data(),12);std::memcpy(w+0x30,q.extents.data(),12);std::memcpy(w+0x3c,q.delta.data(),12);std::memcpy(w+0x48,q.inverse.data(),12);const std::array<float,4> plane{0,0,1,96};std::memcpy(w+0x6c,plane.data(),16);at<float>(w,0x80)=q.mode==0?1.f:q.mode==1?.25f:0.f;at<uint32_t>(w,0x84)=0x24681357;at<uint16_t>(w,0x88)=0xbeef;w[0x8a]=uint8_t(q.allSolid);w[0x8b]=uint8_t(q.startSolid);at<uint32_t>(w,0x90)=q.priorSurface?0x10350ee0u:0x12345678u;at<uint16_t>(w,0x94)=0xabcd;at<uint16_t>(w,0x96)=0xef01;at<uint32_t>(w,0xfc)=q.mask;w[0x100]=0;w[0x101]=uint8_t(q.swept);at<uint32_t>(w,0x104)=0x11111111;at<uint32_t>(w,0x114)=q.marker;at<uint32_t>(w,0x11c)=0;at<uint32_t>(w,0x120)=q.generation;at<uint32_t>(w,0x128)=0x22222222;at<uint32_t>(w,0x150)=0x33333333;return r;}
Record owned(const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,const std::vector<SourceDisplacementHullCacheCandidate>& caches,const std::vector<std::array<uint16_t,2>>& properties,const std::vector<SourceLeafDisplacementCullCandidate>& culls,const Query& q){
 auto r=initial(q);if(q.brushes)r.brushVisit=q.generation;SourceDisplacementWorldHullStateCandidate state;state.hull.fraction=at<float>(r.work.data(),0x80);state.hull.normal={0,0,1};state.hull.distance=96;state.hull.displacementFlags=0xbeef;state.contents=0x24681357;state.displacementHit=q.marker;state.surfaceIsDisplacement=q.priorSurface;state.surfaceProperty=0xabcd;state.surfaceFlags=0xef01;
 SourceLeafDisplacementStateCandidate phase;phase.fraction=state.hull.fraction;phase.normal=state.hull.normal;phase.startSolid=q.startSolid;phase.allSolid=q.allSolid;phase.displacementHit=q.marker!=0;
 sourceLeafHullDisplacementPolicyCandidate(q.ordered,culls,r.visits,q.generation,q.mask,q.swept,q.brushes,phase,q.start,q.delta,q.inverse,q.extents,[&](uint32_t id,auto& hit){r.calls.push_back(id);state.hull.fraction=hit.fraction;state.hull.normal=hit.normal;state.displacementHit=uint32_t(hit.displacementHit);sourceDisplacementWorldHullCandidate(meshes.at(id),caches.at(id),properties.at(id),q.start,q.delta,q.inverse,q.extents,state);hit.fraction=state.hull.fraction;hit.normal=state.hull.normal;hit.displacementHit=state.displacementHit!=0;});
 auto* w=r.work.data();std::memcpy(w+0x6c,state.hull.normal.data(),12);at<float>(w,0x78)=state.hull.distance;at<float>(w,0x80)=phase.fraction;at<uint32_t>(w,0x84)=state.contents;at<uint16_t>(w,0x88)=state.hull.displacementFlags;w[0x8a]=uint8_t(phase.allSolid);w[0x8b]=uint8_t(phase.startSolid);at<uint32_t>(w,0x90)=state.surfaceIsDisplacement?0x10350ee0u:0x12345678u;at<uint16_t>(w,0x94)=state.surfaceProperty;at<uint16_t>(w,0x96)=state.surfaceFlags;at<uint32_t>(w,0x114)=state.displacementHit;return r;
}
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
alignas(16)std::array<std::array<uint8_t,244>,4> nativeObjects{};
std::array<std::vector<Vec>,4> storedPlanes;
std::array<std::vector<std::array<uint16_t,9>>,4> storedIndices;
std::array<std::array<uint8_t,48>,4> nativeCulls{};
bool queryPhase=false;uintptr_t activeWork=0;const Query* activeQuery=nullptr;
const std::vector<SourceLeafDisplacementCullCandidate>* activeCulls=nullptr;
std::vector<uint32_t> outerCalls;int openCache=-1;unsigned boundObservations=0;
void boundary(unsigned slot,CONTEXT& c){
 if(!queryPhase){allocationBoundary(slot,c);return;}
 using O=PrivateX86BoundaryObserverFixture;
 if(slot==0){
  const auto base=reinterpret_cast<uintptr_t>(nativeObjects.data());require(c.Ecx==activeWork&&c.Edx>=base&&(c.Edx-base)%244==0&&(c.Edx-base)/244<4,"outer hull candidate identity");outerCalls.push_back(uint32_t((c.Edx-base)/244));c.EFlags|=0x10000u;
 }else if(slot==1){
  require(c.Edi<4&&activeQuery&&activeCulls,"bounds candidate identity");const auto& q=*activeQuery;const auto& source=activeCulls->at(c.Edi);Vec low{},high{};for(size_t a=0;a<3;++a){low[a]=source.mins[a]-q.extents[a];high[a]=source.maxs[a]+q.extents[a];}
  require(std::memcmp(reinterpret_cast<const void*>(c.Ecx),low.data(),12)==0&&std::memcmp(reinterpret_cast<const void*>(c.Edx),high.data(),12)==0,"native hull-expanded cull input");
  require(address<uintptr_t>(c.Esp+4)==activeWork&&address<uintptr_t>(c.Esp+8)==activeWork+0x3c&&address<uintptr_t>(c.Esp+12)==activeWork+0x48&&address<float>(c.Esp+16)==.03125f,"bounds query arguments");++boundObservations;c.EFlags|=0x10000u;
 }else if(slot==2){
  const auto base=reinterpret_cast<uintptr_t>(nativeObjects.data());require(openCache==-1&&c.Ecx>=base&&(c.Ecx-base)%244==0&&(c.Ecx-base)/244<4,"prepared leaf cache acquire");openCache=int((c.Ecx-base)/244);++acquires;O::returnCdecl(c);
 }else if(slot==3){
  require(openCache>=0&&c.Ecx==0x10643680&&address<uint32_t>(c.Esp+4)==0x1234u+uint32_t(openCache),"prepared leaf cache release");openCache=-1;++releases;O::returnCdecl(c);c.Esp+=4;
 }else throw std::runtime_error("unexpected leaf boundary");
}
void prepareObjects(const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,const std::vector<SourceDisplacementHullCacheCandidate>& caches,const std::vector<std::array<uint16_t,2>>& properties,const std::vector<SourceLeafDisplacementCullCandidate>& culls,unsigned buckets,bool masked){
 for(size_t i=0;i<4;++i){
  const auto& input=meshes.at(i);std::array<std::array<uint16_t,9>,512> indices{};prepare(input.power,buckets,masked,input.vertices,input.triangles,input.tree,caches.at(i),indices);
  storedPlanes[i].assign(nativePlanes.begin(),nativePlanes.begin()+caches[i].planes.size());storedIndices[i].assign(indices.begin(),indices.begin()+input.triangles.size());nativeObjects[i]=mesh;auto* object=nativeObjects[i].data();at<uintptr_t>(object,0xb0)=reinterpret_cast<uintptr_t>(storedIndices[i].data());at<uintptr_t>(object,0xc4)=reinterpret_cast<uintptr_t>(storedPlanes[i].data());at<uint32_t>(object,0x24)=0x1234u+uint32_t(i);at<uint32_t>(object,0x20)=input.contents;at<uint32_t>(object,0x2c)=input.runtimeFlags;std::memcpy(object+0x6c,properties[i].data(),4);
  auto* c=nativeCulls[i].data();nativeCulls[i].fill(0);std::memcpy(c,culls[i].mins.data(),12);std::memcpy(c+16,culls[i].maxs.data(),12);at<uint32_t>(c,32)=culls[i].visitIndex;at<uint32_t>(c,36)=culls[i].contents;
 }
 address<uintptr_t>(0x10637820)=reinterpret_cast<uintptr_t>(nativeObjects.data());address<uintptr_t>(0x10637824)=reinterpret_cast<uintptr_t>(nativeCulls.data());
}
Record native(const Query& q,const std::vector<SourceLeafDisplacementCullCandidate>& culls){
 Record result=initial(q);std::array<uint8_t,0x230> world{};std::array<uint16_t,8> leaf{};std::vector<uint16_t> refs;for(auto id:q.ordered)refs.push_back(uint16_t(id));leaf[5]=q.brushes?1:0;leaf[7]=uint16_t(refs.size());uint16_t dummyRef=0;std::array<uint32_t,2> dummyBrush{};
 auto* w=result.work.data();at<uintptr_t>(world.data(),0x88)=reinterpret_cast<uintptr_t>(leaf.data());at<uintptr_t>(world.data(),0x98)=reinterpret_cast<uintptr_t>(&dummyRef);at<uintptr_t>(world.data(),0xa8)=reinterpret_cast<uintptr_t>(dummyBrush.data());at<uintptr_t>(world.data(),0xb0)=reinterpret_cast<uintptr_t>(refs.data());at<uintptr_t>(w,0x104)=reinterpret_cast<uintptr_t>(world.data());at<uintptr_t>(w,0x128)=reinterpret_cast<uintptr_t>(&result.brushVisit);at<uintptr_t>(w,0x150)=reinterpret_cast<uintptr_t>(result.visits.data());
 activeWork=reinterpret_cast<uintptr_t>(w);activeQuery=&q;activeCulls=&culls;outerCalls.clear();openCache=-1;acquires=releases=0;
 using Leaf=void(__fastcall*)(void*,uint32_t,float,float);reinterpret_cast<Leaf>(0x10148540)(w,0,.125f,.875f);
 require(acquires==releases&&openCache==-1,"leaf cache service balance");result.calls=outerCalls;at<uint32_t>(w,0x104)=0x11111111;at<uint32_t>(w,0x128)=0x22222222;at<uint32_t>(w,0x150)=0x33333333;return result;
}
#endif

int main(int argc,char** argv){try{
 require(argc==4,"expected engine or -, private permutation and disposable output");std::array<uint32_t,256> raw{};std::ifstream fixture(argv[2],std::ios::binary);fixture.read(reinterpret_cast<char*>(raw.data()),sizeof(raw));require(fixture.gcount()==sizeof(raw)&&fixture.peek()==std::char_traits<char>::eof(),"private permutation fixture");SourcePlaneHashPermutationCandidate permutation{};for(size_t i=0;i<raw.size();++i){require(raw[i]<256,"permutation value");permutation[i]=uint8_t(raw[i]);}
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1],0x643eec);image.seal();require(std::memcmp(raw.data(),reinterpret_cast<void*>(0x104129b0),1024)==0,"permutation identity");PrivateX86BoundaryObserverFixture observer{{0x1015b150,0x10001610,0x1015c760,0x10261300},boundary};
#endif
 std::ofstream out(argv[3],std::ios::binary);require(bool(out),"record output");unsigned cases=0,grids=0,totalCalls=0,empty=0,changed=0,newSolid=0,zeroMetadata=0,visitChanges=0,duplicateCalls=0;uint32_t random=0x439abc;auto sample=[&](){random=random*1664525u+1013904223u;return float(int((random>>8)%4097)-2048)/16.f;};
 for(unsigned power=1;power<=4;++power)for(unsigned shape=0;shape<4;++shape){const unsigned cells=1u<<power,width=cells+1,count=2*cells*cells,buckets=shape%2?17u:64u;const bool masked=shape%2==0;
  auto orient=[&](Vec v){if(shape==3)std::swap(v[0],v[2]);return v;};std::vector<SourceDisplacementRuntimeMeshCandidate> meshes(4);std::vector<SourceDisplacementHullCacheCandidate> caches;std::vector<SourceLeafDisplacementCullCandidate> culls;std::vector<std::array<uint16_t,2>> properties;
  for(unsigned id=0;id<4;++id){auto& input=meshes[id];input.power=power;input.contents=std::array<uint32_t,4>{1,0x80,0x4000,0x20}[id];input.runtimeFlags=id==1?8u:id==2?4u:0u;
   for(unsigned y=0;y<width;++y)for(unsigned x=0;x<width;++x){const float z=(shape==0?0.f:shape==1?float(x)*.75f-float(y)*.25f:shape==2?float((x*y)%7):float((x+y)%3)*.03125f)+float(id)*8;Vec vertex{float(x)*8-16,float(y)*8+3,z};if(id==3)vertex[0]+=2000;input.vertices.push_back(orient(vertex));}
   auto source=sourceDisplacementGridTrianglesCandidate(power,std::vector<uint16_t>(count));std::vector<std::array<uint16_t,3>> packed;for(auto t:source){packed.push_back(t.vertices);SourceDisplacementCollisionTriangleCandidate c;c.packedVertices=t.vertices;c.flags=uint16_t((input.triangles.size()%32)<<6);input.triangles.push_back(sourceDisplacementCollisionTriangleCandidate(input.vertices,c));}input.tree=sourceDisplacementCollisionTreeCandidate(power,input.vertices,packed);caches.push_back(sourceDisplacementHullCacheCandidate(power,input.vertices,input.triangles,permutation,buckets,masked));const auto& bounds=input.tree.expandedBounds;culls.push_back({{bounds[0],bounds[1],bounds[2]},{bounds[3],bounds[4],bounds[5]},std::array<uint32_t,4>{1,0x80,0x2080,1}[id],std::array<uint32_t,4>{2,0,3,1}[id]});properties.push_back({uint16_t(100+4*grids+id),uint16_t(500+4*grids+id)});
  }
#ifndef _WIN64
  queryPhase=false;observer.entries={0x1015b150,0x10001610,0x1015c760,0x10261300};observer.configure(true);prepareObjects(meshes,caches,properties,culls,buckets,masked);observer.configure(false);queryPhase=true;observer.entries={0x1014d100,0x1014e790,0x1015c760,0x10261300};observer.configure(true);
#endif
  ++grids;
  for(unsigned qid=0;qid<16;++qid){const float cx=float(cells)*4-16,cy=float(cells)*4+3;Vec start{sample(),sample(),sample()},delta{sample(),sample(),sample()};switch(qid%8){case 0:start={cx,cy,80};delta={0,0,-160};break;case 1:start={cx,cy,-80};delta={0,0,160};break;case 2:start={-56,cy,0};delta={float(cells)*8+80,0,0};break;case 3:start={cx,cy,0};delta={0,-0.f,0};break;case 4:break;case 5:start={-8,11,.03125f};delta={.0625f,.03125f,-.0625f};break;case 6:start={-300,-300,80};delta={0,0,-160};break;case 7:start={-56,cy-40,60};delta={float(cells)*8+80,80,-120};break;}
   for(uint32_t mask:{0u,1u,0x80u,0x2080u,0xffffffffu})for(unsigned mode=0;mode<3;++mode)for(unsigned policy=0;policy<8;++policy){Query q;q.start=orient(start);q.delta=orient(delta);for(size_t a=0;a<3;++a)q.inverse[a]=q.delta[a]==0?std::numeric_limits<float>::max():1.f/q.delta[a];q.extents=qid%3==0?Vec{}:qid%3==1?Vec{24,24,41}:Vec{.125f,2.75f,7.0625f};q.mask=mask;q.mode=mode;q.policy=policy;q.generation=13+qid%3;q.swept=policy!=4&&policy!=6;q.brushes=policy==1||policy==3;q.startSolid=policy==2||policy==3;q.allSolid=policy==4&&qid%3==0;q.marker=(qid+policy)&1;q.priorSurface=(qid&2)!=0;q.visits.resize(4);for(size_t i=0;i<4;++i)q.visits[i]=(policy==5||policy==6||(policy==7&&(qid&(1u<<i))))?q.generation:q.generation^0x12345u;
    switch(qid%5){case 0:q.ordered={0,1,0,2,3,1,2,3};break;case 1:q.ordered={3,2,1,0,1,3};break;case 2:q.ordered={};break;case 3:q.ordered={1,1,0};break;case 4:q.ordered={0,1,2,3};break;}
    const auto before=initial(q);const auto expected=owned(meshes,caches,properties,culls,q);auto result=expected;
#ifndef _WIN64
    result=native(q,culls);if(result.work!=expected.work||result.visits!=expected.visits||result.calls!=expected.calls||result.brushVisit!=expected.brushVisit){std::cerr<<"case "<<cases<<" power "<<power<<" shape "<<shape<<" q "<<qid<<" mask "<<mask<<" mode "<<mode<<" policy "<<policy<<'\n';for(size_t i=0;i<result.work.size();++i)if(result.work[i]!=expected.work[i])std::cerr<<"byte "<<i<<" native "<<unsigned(result.work[i])<<" owned "<<unsigned(expected.work[i])<<'\n';std::cerr<<"calls native";for(auto id:result.calls)std::cerr<<' '<<id;std::cerr<<" owned";for(auto id:expected.calls)std::cerr<<' '<<id;std::cerr<<'\n';for(size_t i=0;i<4;++i)if(result.visits[i]!=expected.visits[i])std::cerr<<"visit "<<i<<' '<<result.visits[i]<<'/'<<expected.visits[i]<<'\n';throw std::runtime_error("hull leaf displacement mismatch");}
#endif
    totalCalls+=unsigned(result.calls.size());empty+=result.calls.empty();changed+=at<float>(result.work.data(),0x80)<at<float>(before.work.data(),0x80);newSolid+=!q.startSolid&&result.work[0x8b]!=0;zeroMetadata+=mode==2&&std::memcmp(result.work.data()+0x6c,before.work.data()+0x6c,16)!=0;visitChanges+=result.visits!=before.visits;for(size_t i=0;i<result.calls.size();++i)for(size_t j=0;j<i;++j)if(result.calls[i]==result.calls[j]){++duplicateCalls;break;}
    const uint32_t calls=uint32_t(result.calls.size());out.write(reinterpret_cast<const char*>(&calls),4);out.write(reinterpret_cast<const char*>(result.calls.data()),std::streamsize(calls*4));out.write(reinterpret_cast<const char*>(result.work.data()),result.work.size());out.write(reinterpret_cast<const char*>(result.visits.data()),16);out.write(reinterpret_cast<const char*>(&result.brushVisit),4);require(bool(out),"record write");++cases;
   }
  }
#ifndef _WIN64
  observer.configure(false);for(size_t i=0;i<4;++i){require(storedPlanes[i].size()==caches[i].planes.size()&&storedIndices[i]==caches[i].indices,"leaf sweep cache coverage/indices");require(storedPlanes[i].empty()||std::memcmp(storedPlanes[i].data(),caches[i].planes.data(),storedPlanes[i].size()*12)==0,"leaf sweep modified plane bytes");}
#endif
 }
#ifndef _WIN64
 require(boundObservations>0,"native bounds observation coverage");image.verify();
#endif
 require(totalCalls&&empty&&changed&&newSolid&&zeroMetadata&&visitChanges&&duplicateCalls,"leaf hull coverage");std::cout<<"{\"cases\":"<<cases<<",\"grids\":"<<grids<<",\"outer_calls\":"<<totalCalls<<",\"empty_calls\":"<<empty<<",\"closer_hits\":"<<changed<<",\"new_start_solid\":"<<newSolid<<",\"zero_fraction_metadata\":"<<zeroMetadata<<",\"visit_changes\":"<<visitChanges<<",\"duplicate_calls\":"<<duplicateCalls<<",\"passed\":true}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

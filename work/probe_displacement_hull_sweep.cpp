#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>
#include "source_displacement_hull_sweep_candidate.h"
#include "source_displacement_grid_triangles_candidate.h"

using Vec=std::array<float,3>;using Trace=std::array<uint8_t,0x54>;
void require(bool v,const char* s){if(!v)throw std::runtime_error(s);}
template<class T>T& at(uint8_t* p,size_t n){return *reinterpret_cast<T*>(p+n);}
Trace initial(unsigned mode){Trace out;out.fill(0xa5);at<float>(out.data(),0x2c)=mode==0?1.f:mode==1?.25f:0.f;return out;}
Trace owned(const SourceDisplacementCollisionTreeCandidate& tree,const std::vector<Vec>& vertices,
 const std::vector<SourceDisplacementCollisionTriangleCandidate>& triangles,const SourceDisplacementHullCacheCandidate& cache,
 unsigned flags,const Vec& start,const Vec& delta,const Vec& inverse,const Vec& extents,unsigned mode,bool& result){
 Trace out=initial(mode);SourceDisplacementHullTraceCandidate trace;trace.fraction=at<float>(out.data(),0x2c);std::memcpy(trace.normal.data(),out.data()+0x18,12);trace.distance=at<float>(out.data(),0x24);trace.displacementFlags=at<uint16_t>(out.data(),0x34);
 result=sourceDisplacementHullSweepCandidate(tree,vertices,triangles,cache,flags,start,delta,inverse,extents,trace);
 at<float>(out.data(),0x2c)=trace.fraction;std::memcpy(out.data()+0x18,trace.normal.data(),12);at<float>(out.data(),0x24)=trace.distance;at<uint16_t>(out.data(),0x34)=trace.displacementFlags;return out;
}
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
template<class T>T& address(uintptr_t p){return *reinterpret_cast<T*>(p);}
struct Entry{Vec plane;uint32_t id;};struct Bucket{Entry* entries;uint32_t allocated,grow,count,unused;};
std::vector<std::array<Entry,8192>> entryStorage;std::vector<Bucket> nativeBuckets;
std::array<Vec,8192> nativePlanes{};alignas(16)std::array<uint8_t,0xf4> mesh{};
unsigned inserts=0,appends=0,acquires=0,releases=0;bool sweeping=false;
void boundary(unsigned slot,CONTEXT& c){using O=PrivateX86BoundaryObserverFixture;const auto m=reinterpret_cast<uintptr_t>(mesh.data());
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
Trace native(unsigned flags,const Vec& start,const Vec& delta,const Vec& inverse,const Vec& extents,unsigned mode,bool& result){
 alignas(16)std::array<uint8_t,0x50> ray{};Trace out=initial(mode);at<uint32_t>(mesh.data(),0x2c)=flags;
 std::memcpy(ray.data(),start.data(),12);std::memcpy(ray.data()+0x10,delta.data(),12);std::memcpy(ray.data()+0x30,extents.data(),12);ray[0x45]=1;
 acquires=releases=0;sweeping=true;using Sweep=bool(__thiscall*)(void*,const void*,const float*,void*);result=reinterpret_cast<Sweep>(0x1015c840)(mesh.data(),ray.data(),inverse.data(),out.data());sweeping=false;require(acquires==releases,"cache acquire/release balance");if(flags&4u)require(!acquires,"disabled cache not touched");return out;
}
#endif
int main(int argc,char** argv){try{
 require(argc==4,"expected engine or -, private permutation and disposable record output");
 std::array<uint32_t,256> raw{};std::ifstream fixture(argv[2],std::ios::binary);fixture.read(reinterpret_cast<char*>(raw.data()),sizeof(raw));require(fixture.gcount()==sizeof(raw)&&fixture.peek()==std::char_traits<char>::eof(),"permutation fixture");SourcePlaneHashPermutationCandidate permutation{};for(size_t i=0;i<raw.size();++i){require(raw[i]<256,"permutation value");permutation[i]=uint8_t(raw[i]);}
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1],0x643eec);image.seal();require(std::memcmp(raw.data(),reinterpret_cast<void*>(0x104129b0),1024)==0,"permutation identity");PrivateX86BoundaryObserverFixture observer{{0x1015b150,0x10001610,0x1015c760,0x10261300},boundary};observer.configure(true);
#endif
 std::ofstream output(argv[3],std::ios::binary);require(bool(output),"record output");unsigned cases=0,meshes=0,changed=0,traceChanges=0,disabled=0,zeroMetadata=0,misses=0,nonAxial=0;uint32_t random=0x947abc;auto sample=[&](){random=random*1664525u+1013904223u;return float(int((random>>8)%4097)-2048)/16.f;};
 for(unsigned power=1;power<=4;++power)for(unsigned shape=0;shape<4;++shape)for(unsigned configuration=0;configuration<3;++configuration){
  const unsigned cells=1u<<power,width=cells+1,count=2*cells*cells,buckets=configuration==0?1u:configuration==1?17u:64u;const bool masked=configuration==2;auto orient=[&](Vec v){if(shape==3)std::swap(v[0],v[2]);return v;};
  std::vector<Vec> vertices;for(unsigned y=0;y<width;++y)for(unsigned x=0;x<width;++x){const float z=shape==0?0.f:shape==1?float(x)*.75f-float(y)*.25f:shape==2?float((x*y)%7):float((x+y)%3)*.03125f;vertices.push_back(orient({float(x)*8-16,float(y)*8+3,z}));}
  auto grid=sourceDisplacementGridTrianglesCandidate(power,std::vector<uint16_t>(count));std::vector<std::array<uint16_t,3>> packed;std::vector<SourceDisplacementCollisionTriangleCandidate> triangles;for(auto t:grid){packed.push_back(t.vertices);SourceDisplacementCollisionTriangleCandidate c;c.packedVertices=t.vertices;c.flags=uint16_t((triangles.size()%32)<<6);triangles.push_back(sourceDisplacementCollisionTriangleCandidate(vertices,c));}
  auto tree=sourceDisplacementCollisionTreeCandidate(power,vertices,packed);auto cache=sourceDisplacementHullCacheCandidate(power,vertices,triangles,permutation,buckets,masked);
#ifndef _WIN64
  std::array<std::array<uint16_t,9>,512> nativeIndices{};prepare(power,buckets,masked,vertices,triangles,tree,cache,nativeIndices);
#endif
  ++meshes;
  for(unsigned q=0;q<24;++q){const float cx=float(cells)*4-16,cy=float(cells)*4+3;Vec start{sample(),sample(),sample()},delta{sample(),sample(),sample()};
   switch(q%8){case 0:start={cx,cy,80};delta={0,0,-160};break;case 1:start={cx,cy,-80};delta={0,0,160};break;case 2:start={-56,cy,0};delta={float(cells)*8+80,0,0};break;case 3:start={cx,cy,0};delta={0,-0.f,0};break;case 4:break;case 5:start={-8,11,.03125f};delta={.0625f,.03125f,-.0625f};break;case 6:start={-300,-300,80};delta={0,0,-160};break;case 7:start={-56,cy-40,60};delta={float(cells)*8+80,80,-120};break;}
   start=orient(start);delta=orient(delta);Vec inverse{};for(size_t a=0;a<3;++a)inverse[a]=delta[a]==0?std::numeric_limits<float>::max():1.f/delta[a];
   for(unsigned hull=0;hull<3;++hull)for(unsigned mode=0;mode<3;++mode)for(unsigned flags:{0u,4u,8u,12u}){
    const Vec extents=hull==0?Vec{}:hull==1?Vec{24,24,41}:Vec{.125f,2.75f,7.0625f};bool expectedResult=false;const auto expected=owned(tree,vertices,triangles,cache,flags,start,delta,inverse,extents,mode,expectedResult);bool result=expectedResult;Trace actual=expected;
#ifndef _WIN64
    actual=native(flags,start,delta,inverse,extents,mode,result);if(actual!=expected||result!=expectedResult){std::cerr<<"case "<<cases<<" power "<<power<<" shape "<<shape<<" configuration "<<configuration<<" q "<<q<<" hull "<<hull<<" mode "<<mode<<" flags "<<flags<<" result "<<result<<'/'<<expectedResult<<'\n';for(size_t i=0;i<actual.size();++i)if(actual[i]!=expected[i])std::cerr<<"byte "<<i<<" native "<<unsigned(actual[i])<<" owned "<<unsigned(expected[i])<<'\n';throw std::runtime_error("composed displacement hull sweep mismatch");}
#endif
    const bool touched=actual!=initial(mode);if(flags&4u){require(!touched&&!result,"disabled sweep retains trace");++disabled;}if(result)require(at<float>(actual.data(),0x2c)<at<float>(initial(mode).data(),0x2c),"changed result lowers fraction");
    changed+=result;traceChanges+=touched;zeroMetadata+=touched&&mode==2;misses+=!touched&&!(flags&4u);if(touched){Vec n;std::memcpy(n.data(),actual.data()+0x18,12);nonAxial+=(n[0]!=0&&n[1]!=0)||(n[0]!=0&&n[2]!=0)||(n[1]!=0&&n[2]!=0);}
    const uint32_t recordResult=result;output.write(reinterpret_cast<const char*>(&recordResult),4);output.write(reinterpret_cast<const char*>(actual.data()),actual.size());require(bool(output),"record write");++cases;
   }
  }
#ifndef _WIN64
  require(std::memcmp(nativeIndices.data(),cache.indices.data(),18*triangles.size())==0,"sweeps changed cache indices");
  require(cache.planes.empty()||std::memcmp(nativePlanes.data(),cache.planes.data(),12*cache.planes.size())==0,"sweeps changed cache planes");
#endif
 }
#ifndef _WIN64
 observer.configure(false);image.verify();
#endif
 require(changed&&traceChanges&&disabled&&misses&&nonAxial,"composed sweep coverage");std::cout<<"{\"cases\":"<<cases<<",\"meshes\":"<<meshes<<",\"changed\":"<<changed<<",\"trace_changes\":"<<traceChanges<<",\"disabled\":"<<disabled<<",\"zero_fraction_metadata\":"<<zeroMetadata<<",\"misses\":"<<misses<<",\"non_axial_hits\":"<<nonAxial<<",\"passed\":true}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

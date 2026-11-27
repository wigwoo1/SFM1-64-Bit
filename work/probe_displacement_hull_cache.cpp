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
#include "source_displacement_hull_cache_candidate.h"
#include "source_displacement_grid_triangles_candidate.h"

using Plane=std::array<float,3>;
void require(bool v,const char* s){if(!v)throw std::runtime_error(s);}
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
template<class T>T& at(uintptr_t p){return *reinterpret_cast<T*>(p);}
struct Entry{Plane plane;uint32_t id;};
struct Bucket{Entry* entries;uint32_t allocated,grow,count,unused;};
std::vector<std::array<Entry,8192>> entryStorage;std::vector<Bucket> nativeBuckets;
std::array<Plane,8192> nativePlanes{};std::array<uint8_t,0xf4> mesh{};unsigned inserts=0,appends=0;
void boundary(unsigned slot,CONTEXT& c){using O=PrivateX86BoundaryObserverFixture;
 if(slot==0){auto* b=reinterpret_cast<Bucket*>(c.Ecx);require(b>=nativeBuckets.data()&&b<nativeBuckets.data()+nativeBuckets.size(),"bucket insert owner");const auto index=at<uint32_t>(c.Esp+4);require(index==b->count&&index<8192,"bucket insertion position");const auto e=*reinterpret_cast<const Entry*>(at<uintptr_t>(c.Esp+8));b->entries[index]=e;++b->count;++inserts;O::returnCdecl(c,index);c.Esp+=8;}
 else if(slot==1){const auto base=reinterpret_cast<uintptr_t>(mesh.data());require(c.Ecx==base+0xc4,"plane append owner");const auto index=at<uint32_t>(c.Esp+4);require(index==at<uint32_t>(base+0xd0)&&index<8192,"plane append index");nativePlanes[index]=*reinterpret_cast<const Plane*>(at<uintptr_t>(c.Esp+8));at<uint32_t>(base+0xd0)=index+1;++appends;O::returnCdecl(c,index);c.Esp+=8;}
 else throw std::runtime_error("unexpected breakpoint");
}
void prepare(unsigned count,bool masked){entryStorage.clear();entryStorage.resize(count);nativeBuckets.clear();for(unsigned i=0;i<count;++i)nativeBuckets.push_back({entryStorage[i].data(),8192,uint32_t(-1),0,0});mesh.fill(0);const auto m=reinterpret_cast<uintptr_t>(mesh.data());at<uintptr_t>(m+0xc4)=reinterpret_cast<uintptr_t>(nativePlanes.data());at<uint32_t>(m+0xc8)=8192;const uintptr_t owner=0x106436dc;at<uintptr_t>(owner)=reinterpret_cast<uintptr_t>(nativeBuckets.data());at<uint32_t>(owner+0xc)=count;at<uint8_t>(owner+0x16)=masked;at<uint32_t>(owner+0x18)=count-1;inserts=appends=0;}
#endif
int main(int argc,char** argv){try{
 require(argc==4,"expected engine or -, private permutation and disposable records");
 std::array<uint32_t,256> raw{};std::ifstream fixture(argv[2],std::ios::binary);fixture.read(reinterpret_cast<char*>(raw.data()),sizeof(raw));require(fixture.gcount()==sizeof(raw)&&fixture.peek()==std::char_traits<char>::eof(),"permutation fixture");SourcePlaneHashPermutationCandidate permutation{};for(size_t i=0;i<raw.size();++i){require(raw[i]<256,"permutation value");permutation[i]=uint8_t(raw[i]);}
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1],0x643eec);image.seal();require(std::memcmp(raw.data(),reinterpret_cast<void*>(0x104129b0),1024)==0,"permutation identity");PrivateX86BoundaryObserverFixture observer{{0x1015b150,0x10001610,0,0},boundary};observer.configure(true);
#endif
 std::ofstream out(argv[3],std::ios::binary);require(bool(out),"record output");unsigned cases=0,totalPlanes=0,totalTriangles=0,signedIndices=0,skipped=0;
 for(unsigned power=1;power<=4;++power)for(unsigned shape=0;shape<4;++shape)for(unsigned configuration=0;configuration<3;++configuration){const unsigned cells=1u<<power,width=cells+1,count=2*cells*cells,buckets=configuration==0?1u:configuration==1?17u:64u;const bool masked=configuration==2;
  std::vector<Plane> vertices;for(unsigned y=0;y<width;++y)for(unsigned x=0;x<width;++x){float z=shape==0?0.f:shape==1?float(x)*.75f-float(y)*.25f:shape==2?float((x*y)%7):float((x+y)%3)*.03125f;vertices.push_back({float(x)*8-16,float(y)*8+3,z});}
  auto grid=sourceDisplacementGridTrianglesCandidate(power,std::vector<uint16_t>(count));std::vector<SourceDisplacementCollisionTriangleCandidate> triangles;for(auto t:grid){SourceDisplacementCollisionTriangleCandidate c;c.packedVertices=t.vertices;triangles.push_back(sourceDisplacementCollisionTriangleCandidate(vertices,c));}
  auto expected=sourceDisplacementHullCacheCandidate(power,vertices,triangles,permutation,buckets,masked);
#ifndef _WIN64
  prepare(buckets,masked);std::array<std::array<uint16_t,9>,512> indices{};const auto m=reinterpret_cast<uintptr_t>(mesh.data());at<uintptr_t>(m+0x70)=reinterpret_cast<uintptr_t>(vertices.data());at<uintptr_t>(m+0x80)=reinterpret_cast<uintptr_t>(triangles.data());at<uint32_t>(m+0x28)=power;at<uintptr_t>(m+0xb0)=reinterpret_cast<uintptr_t>(indices.data());at<uint32_t>(m+0xb4)=512;at<uint32_t>(m+0xb8)=uint32_t(-1);
  using Build=void(__thiscall*)(void*);reinterpret_cast<Build>(0x1015c630)(mesh.data());
  require(at<uint32_t>(m+0xbc)==count&&at<uint32_t>(m+0xd0)==expected.planes.size(),"cache vector counts");require(std::memcmp(indices.data(),expected.indices.data(),18*count)==0,"cache triangle indices");require(expected.planes.empty()||std::memcmp(nativePlanes.data(),expected.planes.data(),12*expected.planes.size())==0,"cache plane bytes");require(inserts==appends&&appends==expected.planes.size(),"cache append counts");for(const auto& b:nativeBuckets)require(b.count==0&&b.entries!=nullptr,"external bucket cleanup");
  const unsigned prior=inserts;const auto savedIndices=indices;const auto savedPlanes=nativePlanes;reinterpret_cast<Build>(0x1015c630)(mesh.data());require(inserts==prior&&indices==savedIndices&&std::memcmp(nativePlanes.data(),savedPlanes.data(),sizeof(nativePlanes))==0,"unchanged-size native cache rebuild fast path");
#endif
  const std::array<uint32_t,4> header{power,shape,uint32_t(expected.indices.size()),uint32_t(expected.planes.size())};out.write(reinterpret_cast<const char*>(header.data()),sizeof(header));for(auto row:expected.indices){out.write(reinterpret_cast<const char*>(row.data()),18);for(auto id:row){signedIndices+=id!=65535&&(id&32768)!=0;skipped+=id==65535;}}for(auto p:expected.planes)out.write(reinterpret_cast<const char*>(p.data()),12);totalPlanes+=unsigned(expected.planes.size());totalTriangles+=count;++cases;
 }
#ifndef _WIN64
 observer.configure(false);image.verify();
#endif
 require(bool(out)&&totalPlanes&&signedIndices&&skipped,"mesh cache coverage");std::cout<<"{\"cases\":"<<cases<<",\"triangles\":"<<totalTriangles<<",\"planes\":"<<totalPlanes<<",\"signed_indices\":"<<signedIndices<<",\"skipped\":"<<skipped<<",\"passed\":true}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

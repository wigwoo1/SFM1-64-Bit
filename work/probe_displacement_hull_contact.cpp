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
#include "source_displacement_hull_triangle_candidate.h"

using Vec=std::array<float,3>;using Trace=std::array<uint8_t,0x54>;
void require(bool v,const char* s){if(!v)throw std::runtime_error(s);}
template<class T>T& at(uint8_t* p,size_t n){return *reinterpret_cast<T*>(p+n);}
struct Fixture{std::vector<Vec> vertices,planes;SourceDisplacementCollisionTriangleCandidate triangle;std::array<uint16_t,9> indices{};};
Fixture fixture(unsigned shape,unsigned encoding){
 Fixture f;const std::array<std::array<Vec,3>,4> points{std::array<Vec,3>{Vec{0,0,0},Vec{0,32,0},Vec{32,0,0}},std::array<Vec,3>{Vec{-9,-7,2},Vec{2,29,9},Vec{37,-4,17}},std::array<Vec,3>{Vec{0,0,0},Vec{0,0,32},Vec{0,32,0}},std::array<Vec,3>{Vec{3.125f,1.75f,9.5f},Vec{19.5f,2.25f,31.75f},Vec{-4.5f,35.25f,-2.5f}}};
 f.vertices.assign(points[shape].begin(),points[shape].end());SourceDisplacementCollisionTriangleCandidate t;t.packedVertices={0,1,2};t.flags=uint16_t(0x40u|(shape<<7));f.triangle=sourceDisplacementCollisionTriangleCandidate(f.vertices,t);
 for(size_t family=0;family<3;++family)for(size_t edge=0;edge<3;++edge){size_t a=(family+1)%3,b=(family+2)%3;const auto& p=f.vertices[edge];const auto& q=f.vertices[(edge+1)%3];const auto& other=f.vertices[(edge+2)%3];const float da=q[a]-p[a],db=q[b]-p[b],length=std::sqrt(da*da+db*db);const size_t slot=family*3+edge;if(length==0){f.indices[slot]=65535;continue;}Vec plane{};plane[a]=db/length;plane[b]=-da/length;plane[family]=plane[a]*p[a]+plane[b]*p[b];if(plane[a]*other[a]+plane[b]*other[b]>plane[family])for(auto& v:plane)v=-v;const auto id=uint16_t(f.planes.size());if(encoding==1&&(slot&1)){for(auto& v:plane)v=-v;f.indices[slot]=uint16_t(id|32768);}else f.indices[slot]=id;f.planes.push_back(plane);if(encoding==2&&slot%3==2)f.indices[slot]=65535;}
 return f;
}
Trace initial(unsigned mode){Trace out{};out.fill(0xa5);at<float>(out.data(),0x2c)=mode==0?1.f:mode==1?.25f:0.f;return out;}
Trace owned(const Fixture& f,const Vec& start,const Vec& delta,const Vec& extents,const Vec& direction,unsigned mode){Trace out=initial(mode);SourceDisplacementHullTraceCandidate trace;trace.fraction=at<float>(out.data(),0x2c);std::memcpy(trace.normal.data(),out.data()+0x18,12);trace.distance=at<float>(out.data(),0x24);trace.displacementFlags=at<uint16_t>(out.data(),0x34);sourceDisplacementHullTriangleCandidate(f.vertices,f.triangle,f.planes,f.indices,start,delta,extents,direction,trace);at<float>(out.data(),0x2c)=trace.fraction;std::memcpy(out.data()+0x18,trace.normal.data(),12);at<float>(out.data(),0x24)=trace.distance;at<uint16_t>(out.data(),0x34)=trace.displacementFlags;return out;}
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
Trace native(const Fixture& f,const Vec& start,const Vec& delta,const Vec& extents,const Vec& direction,unsigned mode){
 alignas(16)std::array<uint8_t,0xf4> mesh{};alignas(16)std::array<uint8_t,0x50> ray{};Trace out=initial(mode);
 at<uintptr_t>(mesh.data(),0x70)=reinterpret_cast<uintptr_t>(f.vertices.data());at<uintptr_t>(mesh.data(),0xb0)=reinterpret_cast<uintptr_t>(f.indices.data());at<uintptr_t>(mesh.data(),0xc4)=reinterpret_cast<uintptr_t>(f.planes.data());std::memcpy(ray.data(),start.data(),12);std::memcpy(ray.data()+0x10,delta.data(),12);std::memcpy(ray.data()+0x30,extents.data(),12);ray[0x45]=1;
 using Contact=void(__thiscall*)(void*,const void*,const float*,int,const void*,void*);reinterpret_cast<Contact>(0x1015abb0)(mesh.data(),ray.data(),direction.data(),0,&f.triangle,out.data());return out;
}
#endif
int main(int argc,char** argv){try{require(argc==3,"expected engine or - and disposable records");
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1],0x643eec);image.seal();
#endif
 std::ofstream output(argv[2],std::ios::binary);require(bool(output),"record output");unsigned cases=0,hits=0,edgeHits=0,rawFacingRejects=0;uint32_t random=0x567abc;auto sample=[&](){random=random*1664525u+1013904223u;return float(int((random>>8)%2049)-1024)/16.f;};
 for(unsigned shape=0;shape<4;++shape)for(unsigned encoding=0;encoding<3;++encoding){auto f=fixture(shape,encoding);for(unsigned q=0;q<160;++q)for(unsigned hull=0;hull<3;++hull)for(unsigned mode=0;mode<3;++mode){Vec start{sample(),sample(),sample()},delta{sample(),sample(),sample()};if(q%5==0){start={8,8,60};delta={0,0,-120};}if(q%5==1){start={60,8,8};delta={-120,0,0};}if(q%5==2){start={8,8,-60};delta={0,0,120};}if(q%5==3)delta={0,-0.f,0};const Vec extents=hull==0?Vec{}:hull==1?Vec{24,24,41}:Vec{.125f,2.75f,7.0625f};Vec direction=delta;engineProjectionNormalizeCandidate(direction.data());const auto& n=f.triangle.normal;const float rawXY=delta[1]*n[1]+delta[0]*n[0],unitXY=direction[1]*n[1]+direction[0]*n[0];rawFacingRejects+=(rawXY+delta[2]*n[2]>.03125f)&&(unitXY+direction[2]*n[2]<=.03125f);const auto expected=owned(f,start,delta,extents,direction,mode);Trace actual=expected;
#ifndef _WIN64
  actual=native(f,start,delta,extents,direction,mode);if(actual!=expected){std::cerr<<"case "<<cases<<" shape "<<shape<<" encoding "<<encoding<<" q "<<q<<" hull "<<hull<<" mode "<<mode<<"\n";for(size_t i=0;i<actual.size();++i)if(actual[i]!=expected[i])std::cerr<<"byte "<<i<<" native "<<unsigned(actual[i])<<" owned "<<unsigned(expected[i])<<"\n";throw std::runtime_error("hull triangle contact mismatch");}
#endif
  const bool changed=actual!=initial(mode);hits+=changed;if(changed){Vec normal{};std::memcpy(normal.data(),actual.data()+0x18,12);edgeHits+=normal!=f.triangle.normal;}++cases;output.write(reinterpret_cast<const char*>(actual.data()),actual.size());require(bool(output),"record write");}}
#ifndef _WIN64
 image.verify();
#endif
 require(hits&&edgeHits&&rawFacingRejects,"contact coverage");std::cout<<"{\"cases\":"<<cases<<",\"hits\":"<<hits<<",\"non_face_hits\":"<<edgeHits<<",\"raw_facing_rejects\":"<<rawFacingRejects<<",\"passed\":true}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

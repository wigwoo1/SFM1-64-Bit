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
#include "source_bsp_brush_only_hull_trace_candidate.h"

using Vec=std::array<float,3>;using Row=std::array<uint32_t,11>;
void require(bool v,const char* s){if(!v)throw std::runtime_error(s);}
uint32_t bits(float f){uint32_t u;std::memcpy(&u,&f,4);return u;}
std::vector<uint8_t> read(const char* p){std::ifstream f(p,std::ios::binary);require(bool(f),"BSP read");return {(std::istreambuf_iterator<char>(f)),{}};}
std::vector<uint32_t> identities(){std::vector<uint32_t> ids(32768);for(uint32_t i=0;i<ids.size();++i)ids[i]=i;return ids;}
bool excludedSurface(uint32_t id,unsigned flags){return id==65535?(flags&2)!=0:(flags&1)&&((id&1)!=0);}
bool excludedBrush(const SourceBspBrushOnlyTraceCandidate& world,uint32_t index,unsigned flags){const auto& r=world.records.at(index);if(r.optimized){for(unsigned a=0;a<6;++a){uint16_t id;std::memcpy(&id,r.box.data()+32+2*a,2);if(excludedSurface(id,flags))return true;}}else{const auto& b=world.data.brushes.at(index);for(uint32_t a=0;a<b.count;++a){const auto texture=world.data.sides.at(b.first+a).texture;if(excludedSurface(texture<0?65535:uint32_t(texture),flags))return true;}}return false;}
SourceOptimizedBoxLineStateCandidate initial(unsigned mode){SourceOptimizedBoxLineStateCandidate s;s.fraction=mode&1?.25f:1.f;s.leftSolid=mode&2?.75f:0;s.contents=0x12345678;s.plane={9,8,7,6};s.planeTags={5,4,3,2};s.surface={SourceCollisionSurfaceKindCandidate::TextureInfo,77};return s;}
Row owned(const SourceBspBrushOnlyTraceCandidate& world,const Vec& start,const Vec& delta,const Vec& extents,uint32_t mask,unsigned mode,unsigned flags){auto s=initial(mode);sourceBspBrushOnlyHullTraceCandidate(world,start,delta,extents,mask,s,[&](uint32_t i){return excludedBrush(world,i,flags);});Row r{bits(s.fraction),bits(s.leftSolid),s.startSolid,s.allSolid,s.contents};for(size_t a=0;a<4;++a)r[5+a]=bits(s.plane[a]);std::memcpy(&r[9],s.planeTags.data(),4);r[10]=s.surface.kind==SourceCollisionSurfaceKindCandidate::Default?65535:s.surface.index;return r;}
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
template<class T>T& at(uintptr_t p){return *reinterpret_cast<T*>(p);}
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
  const auto p=reinterpret_cast<uintptr_t>(fields.data());at<uintptr_t>(p)=reinterpret_cast<uintptr_t>(nodes.data());at<uintptr_t>(p+0x68)=reinterpret_cast<uintptr_t>(sides.data());at<uintptr_t>(p+0x70)=reinterpret_cast<uintptr_t>(boxes.data());at<uintptr_t>(p+0x88)=reinterpret_cast<uintptr_t>(leaves.data());at<uintptr_t>(p+0x98)=reinterpret_cast<uintptr_t>(refs.data());at<uintptr_t>(p+0xa8)=reinterpret_cast<uintptr_t>(brushes.data());at<uintptr_t>(p+0x218)=reinterpret_cast<uintptr_t>(surfaces.data());
 }
 Row trace(const Vec& start,const Vec& delta,const Vec& extents,uint32_t mask,unsigned mode,unsigned flags){
  for(uint32_t i=0;i<surfaces.size();++i)surfaces[i]={0x10000000u+i,excludedSurface(i,flags)?0x00800000u:0};
  at<uint32_t>(0x10402fe8)=0xffffffffu;at<uint32_t>(0x10402fec)=excludedSurface(65535,flags)?0x00800000u:0;
  alignas(16)std::array<uint8_t,0x170> work{};std::vector<uint32_t> visited(brushes.size());const auto p=reinterpret_cast<uintptr_t>(work.data());
  for(size_t a=0;a<3;++a){at<float>(p+4*a)=start[a];at<float>(p+12+4*a)=start[a]+delta[a];at<float>(p+0x30+4*a)=extents[a];at<float>(p+0x3c+4*a)=delta[a];at<float>(p+0x48+4*a)=delta[a]==0?std::numeric_limits<float>::max():1.f/delta[a];}
  auto s=initial(mode);at<float>(p+0x80)=s.fraction;at<float>(p+0x8c)=s.leftSolid;at<uint32_t>(p+0x84)=s.contents;std::memcpy(work.data()+0x6c,s.plane.data(),16);std::memcpy(work.data()+0x7c,s.planeTags.data(),4);std::memcpy(work.data()+0x90,surfaces[77].data(),8);
  at<uintptr_t>(p+0x104)=reinterpret_cast<uintptr_t>(fields.data());at<uint32_t>(p+0xfc)=mask;at<uint8_t>(p+0x101)=1;at<uint32_t>(p+0x120)=1;at<uintptr_t>(p+0x128)=reinterpret_cast<uintptr_t>(visited.data());
  using Walk=void(__fastcall*)(void*,int,float,float,const float*,const float*);reinterpret_cast<Walk>(0x10148c30)(work.data(),source.root,0,1,reinterpret_cast<float*>(p),reinterpret_cast<float*>(p+12));
  Row r{bits(at<float>(p+0x80)),bits(at<float>(p+0x8c)),at<uint8_t>(p+0x8b),at<uint8_t>(p+0x8a),at<uint32_t>(p+0x84)};std::memcpy(r.data()+5,work.data()+0x6c,20);const auto surface=at<uint32_t>(p+0x90);r[10]=surface==0xffffffffu?65535:surface-0x10000000u;require(r[10]==65535||r[10]<surfaces.size(),"native surface identity");return r;
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
int main(int argc,char** argv){try{
 require(argc==4,"expected engine or -, BSP, disposable records");SourceBspBrushOnlyTraceCandidate world(read(argv[2]),identities());
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1],0x643eec);image.seal();
#endif
 std::ofstream output(argv[3],std::ios::binary);require(bool(output),"record output");unsigned cases=0,hits=0,solid=0;uint32_t random=0x1234abcd;auto sample=[&](){random=random*1664525u+1013904223u;return float(int((random>>8)%4097)-2048)/8.f;};
 for(unsigned dataset=0;dataset<2;++dataset){if(dataset)synthetic(world);
#ifndef _WIN64
  NativeWorld native(world);
#endif
  for(unsigned q=0;q<(dataset?96u:32u);++q)for(uint32_t mask:{0u,1u,0x80u,0x2080u,0x2081u,0x4000u,0xffffffffu})for(unsigned mode=0;mode<4;++mode){const unsigned flags=q%4;Vec start{sample(),sample(),sample()},delta{sample(),sample(),sample()};if(q%3==0){start={80,0,0};delta={-160,0,0};}if(q%3==1){start={0,0,100};delta={0,0,-500};}const Vec extents=q%2?Vec{24,24,41}:Vec{.125f,2.75f,7.0625f};auto expected=owned(world,start,delta,extents,mask,mode,flags);Row row=expected;
#ifndef _WIN64
   row=native.trace(start,delta,extents,mask,mode,flags);if(row!=expected){std::cerr<<"case "<<cases<<" dataset "<<dataset<<" q "<<q<<" mask "<<mask<<" mode "<<mode<<"\n";for(size_t a=0;a<row.size();++a)if(row[a]!=expected[a])std::cerr<<"field "<<a<<" native "<<row[a]<<" owned "<<expected[a]<<"\n";throw std::runtime_error("brush world hull mismatch");}
#endif
   output.write(reinterpret_cast<const char*>(row.data()),sizeof(row));require(bool(output),"record write");++cases;hits+=row[0]!=bits(initial(mode).fraction);solid+=row[2]!=0;
  }
 }
#ifndef _WIN64
 image.verify();
#endif
 require(hits&&solid,"world contact coverage");std::cout<<"{\"cases\":"<<cases<<",\"changed_fraction\":"<<hits<<",\"start_solid\":"<<solid<<",\"passed\":true}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

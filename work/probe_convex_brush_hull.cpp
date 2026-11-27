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
#include "source_convex_brush_hull_candidate.h"

using Vec=std::array<float,3>;
using Row=std::array<uint32_t,15>;
void require(bool v,const char* s){if(!v)throw std::runtime_error(s);}
uint32_t bits(float v){uint32_t u;std::memcpy(&u,&v,4);return u;}
std::vector<uint8_t> read(const char* path){std::ifstream f(path,std::ios::binary);require(bool(f),"read fixture");return {(std::istreambuf_iterator<char>(f)),{}};}
SourceConvexBrushRayStateCandidate initial(unsigned mode){SourceConvexBrushRayStateCandidate s;s.fraction=mode&1?.25f:1.f;s.leftSolid=mode&2?.75f:0.f;s.startSolid=(mode&4)!=0;s.allSolid=(mode&8)!=0;s.contents=0x12345678;s.surface={SourceCollisionSurfaceKindCandidate::TextureInfo,77};return s;}
Row owned(const SourceBspWorldCollisionDataCandidate& data,uint32_t brush,const Vec& start,const Vec& end,const Vec& extents,unsigned mode){
 auto state=initial(mode);sourceConvexBrushHullCandidate(data,brush,start,end,extents,state);
 Row row{bits(state.fraction),bits(state.leftSolid),state.startSolid,state.allSolid,state.contents,state.side,uint32_t(state.surface.kind),state.surface.index,bits(9),bits(8),bits(7),bits(6),0x02030405,99,123};
 if(state.side!=UINT32_MAX){const auto& p=data.planes.at(data.sides.at(state.side).plane);for(size_t a=0;a<3;++a)row[8+a]=bits(p.normal[a]);row[11]=bits(p.distance);auto tags=p.tags();std::memcpy(&row[12],tags.data(),4);row[13]=0;row[14]=uint16_t(data.sides.at(state.side).texture);}
 return row;
}
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
template<class T>T& at(uintptr_t p){return *reinterpret_cast<T*>(p);}
const SourceBspWorldCollisionDataCandidate* activeData=nullptr;
const SourceBspCollisionBrushCandidate* activeBrush=nullptr;
std::vector<SourceBspCollisionPlaneCandidate> nativePlanes;
std::array<uint32_t,2> surface{};
uint32_t enteringSide=UINT32_MAX;
uintptr_t workAddress=0,worldAddress=0;
void boundary(unsigned slot,CONTEXT& c){
 if(slot==0){require(c.Ecx==worldAddress,"surface world");const auto texture=at<uint32_t>(c.Esp+4);require(texture<=65535,"surface index");if(texture==65535)surface={at<uint32_t>(0x10402fe8),at<uint32_t>(0x10402fec)};else surface={0xbeef0000u+texture,0xbaad0000u+texture};PrivateX86BoundaryObserverFixture::returnCdecl(c,reinterpret_cast<uintptr_t>(surface.data()));c.Esp+=4;}
 else if(slot==1){require(c.Ecx==workAddress+0x6c,"plane destination");const uintptr_t plane=at<uintptr_t>(c.Esp+4);bool found=false;for(uint32_t i=0;i<activeBrush->count;++i){auto side=activeBrush->first+i;if(plane==reinterpret_cast<uintptr_t>(&nativePlanes.at(activeData->sides.at(side).plane))){enteringSide=side;found=true;break;}}require(found,"entering plane identity");c.EFlags|=0x10000;}
 else throw std::runtime_error("unexpected breakpoint");
}
struct NativeSide{const SourceBspCollisionPlaneCandidate* plane;uint16_t texture,bevel;};
Row native(const SourceBspWorldCollisionDataCandidate& data,uint32_t brush,const Vec& start,const Vec& end,const Vec& extents,unsigned mode){
 activeData=&data;activeBrush=&data.brushes.at(brush);nativePlanes=data.planes;
 std::vector<NativeSide> sides;for(const auto& s:data.sides)sides.push_back({&nativePlanes.at(s.plane),uint16_t(s.texture),uint16_t(s.bevel)});
 alignas(16)std::array<uint8_t,0x130> work{};std::array<uint8_t,0x110> world{};
 workAddress=reinterpret_cast<uintptr_t>(work.data());worldAddress=reinterpret_cast<uintptr_t>(world.data());
 at<uintptr_t>(worldAddress+0x68)=reinterpret_cast<uintptr_t>(sides.data());at<uintptr_t>(workAddress+0x104)=worldAddress;
 std::memcpy(work.data(),start.data(),12);std::memcpy(work.data()+12,end.data(),12);std::memcpy(work.data()+0x30,extents.data(),12);
 auto state=initial(mode);at<float>(workAddress+0x80)=state.fraction;at<float>(workAddress+0x8c)=state.leftSolid;
 at<uint8_t>(workAddress+0x8b)=state.startSolid;at<uint8_t>(workAddress+0x8a)=state.allSolid;at<uint32_t>(workAddress+0x84)=state.contents;
 at<uint32_t>(workAddress+0x90)=0x1111;at<uint32_t>(workAddress+0x94)=0x2222;
 const std::array<float,4> sentinel{9,8,7,6};std::memcpy(work.data()+0x6c,sentinel.data(),16);at<uint32_t>(workAddress+0x7c)=0x02030405;
 at<uint32_t>(workAddress+0x114)=99;at<uint16_t>(workAddress+0x9e)=123;enteringSide=UINT32_MAX;
 const auto& b=*activeBrush;require(b.count<65535&&b.first<65536,"general brush span");
 std::array<uint32_t,2> record{b.contents,(b.first<<16)|b.count};
 using Trace=void(__fastcall*)(void*,const void*);reinterpret_cast<Trace>(0x10147920)(work.data(),record.data());
 Row row{bits(at<float>(workAddress+0x80)),bits(at<float>(workAddress+0x8c)),at<uint8_t>(workAddress+0x8b),at<uint8_t>(workAddress+0x8a),at<uint32_t>(workAddress+0x84),enteringSide,0,0};
 const auto first=at<uint32_t>(workAddress+0x90),second=at<uint32_t>(workAddress+0x94);
 if(first==0x1111&&second==0x2222){row[6]=uint32_t(SourceCollisionSurfaceKindCandidate::TextureInfo);row[7]=77;}
 else if((first&0xffff0000u)==0xbeef0000u){row[6]=uint32_t(SourceCollisionSurfaceKindCandidate::TextureInfo);row[7]=first&65535;require(second==0xbaad0000u+row[7],"surface record");}
 else require(first==at<uint32_t>(0x10402fe8)&&second==at<uint32_t>(0x10402fec),"default surface record");
 std::memcpy(row.data()+8,work.data()+0x6c,20);row[13]=at<uint32_t>(workAddress+0x114);row[14]=at<uint16_t>(workAddress+0x9e);return row;
}
#endif
int main(int argc,char** argv){try{
 require(argc==4,"expected engine or -, benchmark BSP and disposable output");
 SourceBspWorldCollisionDataCandidate data(read(argv[2]));
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1],0x643eec);image.seal();PrivateX86BoundaryObserverFixture observer{{0x10146440,0x100e9720,0,0},boundary};observer.configure(true);
#endif
 std::ofstream output(argv[3],std::ios::binary);require(bool(output),"output");unsigned cases=0,entering=0,startSolid=0,allSolid=0,bevelHits=0,leftSolidEntering=0,newStartSolid=0;
 auto test=[&](uint32_t b,const Vec& start,const Vec& end,const Vec& extents,unsigned mode){
  auto expected=owned(data,b,start,end,extents,mode);Row row=expected;
#ifndef _WIN64
  row=native(data,b,start,end,extents,mode);if(row!=expected){std::cerr<<"case "<<cases<<" brush "<<b<<" mode "<<mode<<"\n";for(size_t i=0;i<row.size();++i)if(row[i]!=expected[i])std::cerr<<"field "<<i<<" native "<<row[i]<<" owned "<<expected[i]<<"\n";throw std::runtime_error("convex hull mismatch");}
#endif
  output.write(reinterpret_cast<const char*>(row.data()),sizeof(row));require(bool(output),"record write");++cases;leftSolidEntering+=(mode&2)&&row[5]!=UINT32_MAX;newStartSolid+=!(mode&4)&&row[2]!=0;entering+=row[5]!=UINT32_MAX;startSolid+=row[2]!=0;allSolid+=row[3]!=0;if(row[5]!=UINT32_MAX)bevelHits+=data.sides.at(row[5]).bevel!=0;
 };
 uint32_t random=0x23456789;auto sample=[&](){random=random*1664525u+1013904223u;return float(int((random>>8)%8193)-4096)/8.f;};
 // Actual benchmark brushes: use the general route deliberately, with source
 // plane order preserved. Optimized route selection is a separate concern.
 const size_t actual=std::min<size_t>(data.brushes.size(),192);
 for(uint32_t b=0;b<actual;++b)for(unsigned mode=0;mode<4;++mode)for(unsigned q=0;q<4;++q){Vec start{sample(),sample(),sample()},end{sample(),sample(),sample()};test(b,start,end,q&1?Vec{24,24,41}:Vec{1.25f,3.5f,9.125f},mode);}
 data.planes.clear();data.sides.clear();data.brushes.clear();
 const std::array<Vec,8> normals{Vec{1,0,0},Vec{-1,0,0},Vec{0,1,0},Vec{0,-1,0},Vec{0,0,1},Vec{0,0,-1},Vec{.6f,.8f,0},Vec{-.5f,.5f,.70710677f}};
 for(unsigned shape=0;shape<4;++shape){data.planes.clear();data.sides.clear();for(unsigned i=0;i<normals.size();++i){data.planes.push_back(sourceBspCollisionPlaneCandidate(normals[i],i<6?16.f:shape&1?5.f:20.f,i/2<3?i/2:3));data.sides.push_back({uint16_t(i),int16_t(shape==3?-1:i+1),-1,int16_t(i>=6)});}data.brushes={{0,shape==0?6u:8u,1u<<shape}};
  for(unsigned q=0;q<100;++q)for(unsigned mode=0;mode<16;++mode){Vec start{sample()/8,sample()/8,sample()/8},end{sample()/8,sample()/8,sample()/8};if(q%5==0)start={0,0,0};if(q%5==1)end=start;if(q%5==2){start={100,0,0};end={-100,0,0};}if(q%5==3){start={0,100,0};end={0,-100,0};}const Vec extents=q%3==0?Vec{}:q%3==1?Vec{24,24,41}:Vec{.125f,2.75f,7.0625f};test(0,start,end,extents,mode);}
 }
 data.brushes={{0,0,1}};for(unsigned mode=0;mode<16;++mode)test(0,Vec{},Vec{},Vec{},mode);
#ifndef _WIN64
 observer.configure(false);image.verify();
#endif
 require(entering&&startSolid&&allSolid&&bevelHits&&leftSolidEntering&&newStartSolid,"contact coverage");std::cout<<"{\"cases\":"<<cases<<",\"entering\":"<<entering<<",\"start_solid\":"<<startSolid<<",\"all_solid\":"<<allSolid<<",\"bevel_hits\":"<<bevelHits<<",\"left_solid_entering\":"<<leftSolidEntering<<",\"new_start_solid\":"<<newStartSolid<<",\"passed\":true}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

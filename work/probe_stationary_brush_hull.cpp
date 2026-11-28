#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>
#include "source_stationary_brush_hull_candidate.h"

using Vec=std::array<float,3>;using Record=std::array<uint8_t,84>;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T& at(uint8_t* p,size_t n){return *reinterpret_cast<T*>(p+n);}
std::vector<uint8_t> read(const char* path){std::ifstream f(path,std::ios::binary);require(bool(f),"benchmark read");return {(std::istreambuf_iterator<char>(f)),{}};}
SourceOptimizedBoxLineStateCandidate initial(unsigned mode){SourceOptimizedBoxLineStateCandidate s;s.fraction=mode%3==0?1.f:mode%3==1?.25f:0.f;s.leftSolid=mode&2?.75f:0.f;s.startSolid=(mode&4)!=0;s.allSolid=(mode&8)!=0;s.contents=0x12345678;s.plane={9,8,7,6};s.planeTags={5,4,3,2};s.surface={SourceCollisionSurfaceKindCandidate::TextureInfo,77};return s;}
Record pack(const SourceOptimizedBoxLineStateCandidate& s){
 Record r;for(size_t i=0;i<r.size();++i)r[i]=uint8_t(0x40+i);
 std::memcpy(r.data()+0x18,s.plane.data(),16);std::memcpy(r.data()+0x28,s.planeTags.data(),4);
 at<float>(r.data(),0x2c)=s.fraction;at<uint32_t>(r.data(),0x30)=s.contents;
 r[0x36]=uint8_t(s.allSolid);r[0x37]=uint8_t(s.startSolid);at<float>(r.data(),0x38)=s.leftSolid;
 require(s.surface.kind==SourceCollisionSurfaceKindCandidate::TextureInfo&&s.surface.index==77,"stationary surface preserved");return r;
}
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
struct NativeSide{const SourceBspCollisionPlaneCandidate* plane;uint16_t texture,bevel;};
struct alignas(16)NativeBox{std::array<uint8_t,48> bytes;};
struct NativeInput {
 std::vector<NativeSide> sides;std::vector<NativeBox> boxes;std::array<uint8_t,0x110> world{};
 explicit NativeInput(const SourceBspWorldCollisionDataCandidate& data,const std::vector<SourceBspCollisionRecordCandidate>& records){
  static_assert(sizeof(NativeSide)==8&&sizeof(SourceBspCollisionPlaneCandidate)==20,"native record width");
  for(const auto& s:data.sides)sides.push_back({&data.planes.at(s.plane),uint16_t(s.texture),uint16_t(s.bevel)});
  for(const auto& r:records)boxes.push_back({r.box});
  at<uintptr_t>(world.data(),0x68)=reinterpret_cast<uintptr_t>(sides.data());at<uintptr_t>(world.data(),0x70)=reinterpret_cast<uintptr_t>(boxes.data());
 }
 Record trace(const SourceBspWorldCollisionDataCandidate& data,const std::vector<SourceBspCollisionRecordCandidate>& records,uint32_t index,
  bool general,const Vec& center,const Vec& minimum,const Vec& maximum,unsigned mode,const Record& expected){
  alignas(16)std::array<uint8_t,0x130> work;work.fill(0xa7);
  std::memcpy(work.data(),center.data(),12);std::memcpy(work.data()+0x18,minimum.data(),12);std::memcpy(work.data()+0x24,maximum.data(),12);
  at<uintptr_t>(work.data(),0x104)=reinterpret_cast<uintptr_t>(world.data());const auto before=pack(initial(mode));std::memcpy(work.data()+0x54,before.data(),before.size());
  const auto& b=data.brushes.at(index);require(b.count<65535&&b.first<65536&&index<65536,"native brush span");
  const std::array<uint32_t,2> brush{b.contents,!general&&records[index].optimized?(index<<16)|65535:(b.first<<16)|b.count};
  auto wanted=work;std::memcpy(wanted.data()+0x54,expected.data(),expected.size());
  using Primitive=void(__thiscall*)(void*,const void*);reinterpret_cast<Primitive>(0x10146c30)(work.data(),brush.data());
  if(work!=wanted){for(size_t i=0;i<work.size();++i)if(work[i]!=wanted[i])std::cerr<<"work byte "<<i<<" native "<<unsigned(work[i])<<" owned "<<unsigned(wanted[i])<<'\n';throw std::runtime_error("stationary hull full work mismatch");}
  Record result;std::memcpy(result.data(),work.data()+0x54,result.size());return result;
 }
};
#endif

int main(int argc,char** argv){try{
 require(argc==4,"expected engine or -, benchmark BSP, disposable output");SourceBspWorldCollisionDataCandidate data(read(argv[2]));
 std::vector<uint32_t> identities(32768);for(uint32_t i=0;i<identities.size();++i)identities[i]=i;
 auto records=sourceBspCollisionRecordsCandidate(data,identities);const unsigned actualBrushes=unsigned(data.brushes.size());require(actualBrushes==1866,"benchmark brush inventory");
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1],0x643eec);image.seal();
#endif
 std::ofstream out(argv[3],std::ios::binary);require(bool(out),"output");
 unsigned cases=0,hits=0,misses=0,optimized=0,generalCount=0,nonzero=0,asymmetric=0,boundaryHits=0,bevelHits=0;
 const std::array<Vec,6> mins{Vec{},Vec{-1,-3,-5},Vec{-24,-24,-41},Vec{0,0,0},Vec{-5,1,-7},Vec{-0.f,-0.f,0}},
  maxs{Vec{},Vec{1,3,5},Vec{24,24,41},Vec{1,5,82},Vec{-2,9,-3},Vec{0,0,0}};
 auto generalRecords=records;for(auto& record:generalRecords)record.optimized=false;
#ifndef _WIN64
 auto native=std::make_unique<NativeInput>(data,records);
#endif
 auto test=[&](uint32_t index,bool forceGeneral,const Vec& center,unsigned variant,unsigned mode,bool boundary){
  const auto& selected=forceGeneral?generalRecords:records;auto state=initial(mode);sourceStationaryBrushHullCandidate(data,selected,index,center,mins[variant],maxs[variant],state);const auto expected=pack(state);auto result=expected;
#ifndef _WIN64
  try{result=native->trace(data,records,index,forceGeneral,center,mins[variant],maxs[variant],mode,expected);}catch(...){std::cerr<<"case "<<cases<<" brush "<<index<<" forceGeneral "<<forceGeneral<<" variant "<<variant<<" mode "<<mode<<'\n';throw;}
#endif
  out.write(reinterpret_cast<const char*>(result.data()),result.size());require(bool(out),"record write");++cases;const bool hit=state.contents!=initial(mode).contents;hits+=hit;misses+=!hit;optimized+=selected[index].optimized;generalCount+=!selected[index].optimized;nonzero+=variant>0&&variant<5;asymmetric+=variant==3||variant==4;boundaryHits+=boundary&&hit;
  if(hit&&!selected[index].optimized){const auto& b=data.brushes[index];for(uint32_t i=0;i<b.count;++i)if(data.sides.at(b.first+i).bevel){++bevelHits;break;}}
 };
 // Every actual brush, both its classified route and its general plane route.
 for(uint32_t index=0;index<data.brushes.size();++index){
  Vec low{},high{},center{};std::array<bool,3> lowFound{},highFound{};
  for(uint32_t i=0;i<data.brushes[index].count;++i){const auto& p=data.planes[data.sides[data.brushes[index].first+i].plane];for(size_t a=0;a<3;++a){bool axial=true;for(size_t k=0;k<3;++k)if(a!=k&&p.normal[k]!=0)axial=false;if(axial&&p.normal[a]==1){high[a]=p.distance;highFound[a]=true;}if(axial&&p.normal[a]==-1){low[a]=-p.distance;lowFound[a]=true;}}}
  for(size_t a=0;a<3;++a){if(!lowFound[a]||!highFound[a]){low[a]=-32;high[a]=32;}center[a]=(low[a]+high[a])*.5f;}
  for(bool forced:{false,true})for(unsigned variant=0;variant<6;++variant)for(unsigned q=0;q<8;++q)for(unsigned mode:{0u,3u,6u,15u}){
   Vec point=center;if(q==1)point[0]=low[0]-maxs[variant][0];if(q==2)point[0]=std::nextafter(low[0]-maxs[variant][0],-std::numeric_limits<float>::infinity());if(q==3)point[0]=high[0]-mins[variant][0];if(q==4)point[0]=std::nextafter(high[0]-mins[variant][0],std::numeric_limits<float>::infinity());if(q==5)point[2]=low[2]-maxs[variant][2];if(q==6)point[2]=std::nextafter(high[2]-mins[variant][2],std::numeric_limits<float>::infinity());if(q==7)for(size_t a=0;a<3;++a)point[a]=high[a]+256;
   test(index,forced,point,variant,mode,q>=1&&q<=6);
  }
 }
 // Controlled oblique planes, bevel sides, signed zeros and offset-only boxes.
 uint32_t random=0x372619ab;auto sample=[&](){random=random*1664525u+1013904223u;return float(int((random>>8)%8193)-4096)/64.f;};
 const std::array<Vec,8> normals{Vec{1,0,0},Vec{-1,0,0},Vec{0,1,0},Vec{0,-1,0},Vec{0,0,1},Vec{0,0,-1},Vec{.6f,.8f,-0.f},Vec{-.5f,.5f,.70710677f}};
 for(unsigned shape=0;shape<4;++shape){data.planes.clear();data.sides.clear();for(unsigned i=0;i<normals.size();++i){data.planes.push_back(sourceBspCollisionPlaneCandidate(normals[i],i<6?16.f:shape&1?5.f:20.f,i<6?i/2:3));data.sides.push_back({uint16_t(i),int16_t(i),-1,int16_t(i>=6)});}data.brushes={{0,shape==0?6u:8u,1u<<shape}};records=sourceBspCollisionRecordsCandidate(data,identities);generalRecords=records;for(auto& r:generalRecords)r.optimized=false;
#ifndef _WIN64
  native=std::make_unique<NativeInput>(data,records);
#endif
  for(unsigned q=0;q<192;++q){Vec point{sample(),sample(),sample()};if(q%8==0)point={0,-0.f,0};if(q%8==1)point={16,0,0};if(q%8==2)point={-16,0,0};if(q%8==3)point={std::nextafter(16.f,std::numeric_limits<float>::infinity()),0,0};for(unsigned variant=0;variant<6;++variant)for(unsigned mode=0;mode<16;++mode)test(0,shape!=0,point,variant,mode,q%8>=1&&q%8<=3);}
 }
 data.brushes={{0,0,1}};records={{false,{}}};generalRecords=records;
#ifndef _WIN64
 native=std::make_unique<NativeInput>(data,records);
#endif
 for(unsigned variant=0;variant<6;++variant)for(unsigned mode=0;mode<16;++mode)test(0,true,Vec{},variant,mode,false);
#ifndef _WIN64
 image.verify();
#endif
 require(hits&&misses&&optimized&&generalCount&&nonzero&&asymmetric&&boundaryHits&&bevelHits,"stationary hull coverage");
 std::cout<<"{\"cases\":"<<cases<<",\"actual_brushes\":"<<actualBrushes<<",\"hits\":"<<hits<<",\"misses\":"<<misses<<",\"optimized\":"<<optimized<<",\"general\":"<<generalCount<<",\"nonzero_hulls\":"<<nonzero<<",\"asymmetric_hulls\":"<<asymmetric<<",\"boundary_hits\":"<<boundaryHits<<",\"bevel_hits\":"<<bevelHits<<",\"passed\":true}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

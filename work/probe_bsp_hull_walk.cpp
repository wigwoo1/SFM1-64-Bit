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
#include "source_bsp_hull_walk_candidate.h"

using Vec=std::array<float,3>;
using Event=std::array<uint32_t,3>;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
uint32_t bits(float f){uint32_t u;std::memcpy(&u,&f,4);return u;}
std::vector<Event> events;
unsigned policy=0;
void visit(uint32_t leaf,float a,float b,float& hit){
 events.push_back({leaf,bits(a),bits(b)});
 if(policy==1)hit=std::min(hit,.375f);
 if(policy==2)hit=0;
}
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
template<class T>T& at(uintptr_t p){return *reinterpret_cast<T*>(p);}
uintptr_t activeWork=0;
void boundary(unsigned slot,CONTEXT& c){
 require(slot==0&&c.Ecx==activeWork,"native leaf callback owner");
 visit(c.Edx,at<float>(c.Esp+4),at<float>(c.Esp+8),at<float>(activeWork+0x80));
 PrivateX86BoundaryObserverFixture::returnCdecl(c);c.Esp+=8;
}
struct NativePlane{Vec normal;float distance;uint8_t type;std::array<uint8_t,3> padding{};};
struct NativeNode{NativePlane* plane;std::array<int32_t,2> children;};
float nativeWalk(const std::vector<SourceLineWalkPlaneCandidate>& planes,const std::vector<SourceLineWalkNodeCandidate>& nodes,
 const Vec& start,const Vec& end,const Vec& extents,float hit){
 std::vector<NativePlane> nativePlanes;for(const auto& p:planes)nativePlanes.push_back({p.normal,p.distance,p.type,{}});
 std::vector<NativeNode> nativeNodes;for(const auto& n:nodes)nativeNodes.push_back({&nativePlanes.at(n.plane),n.children});
 alignas(16)std::array<uint8_t,0x130> work{};std::array<uintptr_t,1> world{reinterpret_cast<uintptr_t>(nativeNodes.data())};
 activeWork=reinterpret_cast<uintptr_t>(work.data());at<uintptr_t>(activeWork+0x104)=reinterpret_cast<uintptr_t>(world.data());
 std::memcpy(work.data()+0x30,extents.data(),12);at<float>(activeWork+0x80)=hit;
 using Walk=void(__fastcall*)(void*,int,float,float,const float*,const float*);
 reinterpret_cast<Walk>(0x10148c30)(work.data(),0,0.f,1.f,start.data(),end.data());
 return at<float>(activeWork+0x80);
}
#endif
int main(int argc,char** argv){try{
 require(argc==3,"expected engine or - and disposable record output");
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1],0x643eec);image.seal();
 PrivateX86BoundaryObserverFixture observer{{0x10148540,0,0,0},boundary};observer.configure(true);
#else
 (void)argv;
#endif
 std::ofstream output(argv[2],std::ios::binary);require(bool(output),"record output");
 unsigned cases=0,visits=0,overlap=0,pruned=0;
 uint32_t random=0x31415926;
 auto sample=[&](){random=random*1664525u+1013904223u;return float(int((random>>8)%4097)-2048)/32.f;};
 for(unsigned graph=0;graph<4;++graph)for(unsigned shape=0;shape<6;++shape)
 for(unsigned path=0;path<80;++path)for(unsigned clipping=0;clipping<4;++clipping){
  std::vector<SourceLineWalkPlaneCandidate> planes={{{1,0,0},0,0},{{0,1,0},3,1},{{0,0,1},-2,2},{{.6f,.8f,0},1,3},{{-.5f,.5f,.70710677f},-5,4},{{.25f,-.75f,.61237246f},0,5}};
  std::vector<SourceLineWalkNodeCandidate> nodes;
  if(graph==0)nodes={{shape,{-1,-2}}};
  if(graph==1)nodes={{shape,{1,2}},{(shape+1)%6,{-1,-2}},{(shape+3)%6,{-3,-4}}};
  if(graph==2)nodes={{shape,{1,-1}},{(shape+2)%6,{-2,2}},{(shape+4)%6,{3,-3}},{(shape+5)%6,{-4,-5}}};
  if(graph==3)nodes={{shape,{1,1}},{(shape+3)%6,{-1,-2}}};
  Vec start{sample(),sample(),sample()},end{sample(),sample(),sample()},extents{24,24,41};
  if(path%5==0){start={0,3,-2};end=start;} // Plane equality and constant centers.
  if(path%5==1){start={0,0,120};end={0,0,-380};}
  if(path%5==2)extents={.0001f,.125f,.75f};
  if(path%5==3)extents={0,0,0}; // This entry's behavior; not ray dispatch.
  if(path%5==4)extents={3.125f,7.75f,16.0625f};
  if(path>=40)extents={std::abs(sample()),std::abs(sample()),std::abs(sample())};
  float initial=clipping==3?0.f:1.f;policy=clipping==3?0:clipping;
  events.clear();float hit=initial;
#ifndef _WIN64
  hit=nativeWalk(planes,nodes,start,end,extents,initial);
  const auto nativeEvents=events;events.clear();float ownedHit=initial;
  sourceBspHullWalkCandidate(planes,nodes,0,5,start,end,extents,0,1,ownedHit,[&](uint32_t l,float a,float b){visit(l,a,b,ownedHit);});
  if(events!=nativeEvents||bits(hit)!=bits(ownedHit)){std::cerr<<"case "<<cases<<" graph "<<graph<<" plane "<<shape<<" path "<<path<<" clipping "<<clipping<<"\n";throw std::runtime_error("native hull traversal mismatch");}
#else
  sourceBspHullWalkCandidate(planes,nodes,0,5,start,end,extents,0,1,hit,[&](uint32_t l,float a,float b){visit(l,a,b,hit);});
#endif
  const std::array<uint32_t,3> header{cases,uint32_t(events.size()),bits(hit)};
  output.write(reinterpret_cast<const char*>(header.data()),sizeof(header));
  for(const auto& row:events)output.write(reinterpret_cast<const char*>(row.data()),sizeof(row));
  require(bool(output),"semantic record write");visits+=unsigned(events.size());overlap+=events.size()>1;pruned+=events.empty();++cases;
 }
#ifndef _WIN64
 observer.configure(false);image.verify();
#endif
 require(overlap&&pruned,"hull walk coverage");
 std::cout<<"{\"cases\":"<<cases<<",\"visits\":"<<visits<<",\"overlap_cases\":"<<overlap<<",\"pruned_cases\":"<<pruned<<",\"passed\":true}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

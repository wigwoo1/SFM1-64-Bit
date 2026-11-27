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
#include "source_displacement_hull_tree_candidate.h"
#include "source_displacement_grid_triangles_candidate.h"
#include "source_displacement_collision_triangle_candidate.h"

using Vec=std::array<float,3>;using Event=std::array<uint32_t,6>;
void require(bool v,const char* s){if(!v)throw std::runtime_error(s);}
uint32_t bits(float f){uint32_t u;std::memcpy(&u,&f,4);return u;}
std::vector<Event> events;unsigned policy=0,begins=0,ends=0;
void contact(uint32_t id,const Vec& normal,float& fraction){const float before=fraction;if(policy==1)fraction=std::min(fraction,float(id%5)*.125f);if(policy==2)fraction=0;events.push_back({id,bits(normal[0]),bits(normal[1]),bits(normal[2]),bits(before),bits(fraction)});}
struct Host{void beginTriangles(){++begins;}void endTriangles(){++ends;}void traceTriangle(uint32_t id,const Vec& normal,float& f){contact(id,normal,f);}};
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
template<class T>T& at(uintptr_t p){return *reinterpret_cast<T*>(p);}
uintptr_t activeMesh=0,activeRay=0,activeTrace=0,triangleBase=0;size_t triangleCount=0;
void boundary(unsigned slot,CONTEXT& c){using O=PrivateX86BoundaryObserverFixture;
 if(slot==0){require(c.Ecx==activeMesh,"cache owner");++begins;O::returnCdecl(c);}
 else if(slot==1){require(c.Ecx==activeMesh&&at<uintptr_t>(c.Esp+4)==activeRay,"triangle owner/ray");const auto id=at<uint32_t>(c.Esp+12);require(id<triangleCount&&at<uintptr_t>(c.Esp+16)==triangleBase+24*id&&at<uintptr_t>(c.Esp+20)==activeTrace,"triangle identity/output");Vec normal{};std::memcpy(normal.data(),reinterpret_cast<void*>(at<uintptr_t>(c.Esp+8)),12);contact(id,normal,at<float>(activeTrace+0x2c));O::returnCdecl(c);c.Esp+=20;}
 else if(slot==2){require(c.Ecx==0x10643680&&at<uint32_t>(c.Esp+4)==0x1234,"cache release");++ends;O::returnCdecl(c);c.Esp+=4;}
 else throw std::runtime_error("unexpected breakpoint");
}
bool native(const SourceDisplacementCollisionTreeCandidate& tree,unsigned flags,const Vec& start,const Vec& delta,const Vec& inverse,const Vec& extents,float& fraction){
 alignas(16)std::array<uint8_t,0xf4> mesh{};alignas(16)std::array<uint8_t,0x50> ray{};std::array<uint8_t,0x54> trace{};
 std::vector<SourceDisplacementCollisionTriangleCandidate> triangles(2*tree.leafPairs.size());
 activeMesh=reinterpret_cast<uintptr_t>(mesh.data());activeRay=reinterpret_cast<uintptr_t>(ray.data());activeTrace=reinterpret_cast<uintptr_t>(trace.data());triangleBase=reinterpret_cast<uintptr_t>(triangles.data());triangleCount=triangles.size();
 at<uint32_t>(activeMesh+0x2c)=flags;at<uint32_t>(activeMesh+0x24)=0x1234;at<uintptr_t>(activeMesh+0x80)=triangleBase;at<uintptr_t>(activeMesh+0x90)=reinterpret_cast<uintptr_t>(tree.nodes.data());at<uint32_t>(activeMesh+0x98)=uint32_t(tree.nodes.size());at<uintptr_t>(activeMesh+0xa0)=reinterpret_cast<uintptr_t>(tree.leafPairs.data());
 std::memcpy(ray.data(),start.data(),12);std::memcpy(ray.data()+0x10,delta.data(),12);std::memcpy(ray.data()+0x30,extents.data(),12);at<uint8_t>(activeRay+0x45)=1;at<float>(activeTrace+0x2c)=fraction;
 using Query=bool(__thiscall*)(void*,const void*,const float*,void*);const bool result=reinterpret_cast<Query>(0x1015c840)(mesh.data(),ray.data(),inverse.data(),trace.data());fraction=at<float>(activeTrace+0x2c);return result;
}
#endif
int main(int argc,char** argv){try{
 require(argc==3,"expected engine or - and disposable semantic record output");
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1],0x643eec);image.seal();PrivateX86BoundaryObserverFixture observer{{0x1015c760,0x1015abb0,0x10261300,0},boundary};observer.configure(true);
#endif
 std::ofstream output(argv[2],std::ios::binary);require(bool(output),"record output");unsigned cases=0,totalCalls=0,empty=0,changed=0;uint32_t random=0x123abc;auto sample=[&](){random=random*1664525u+1013904223u;return float(int((random>>8)%4097)-2048)/16.f;};
 for(unsigned power=1;power<=4;++power){const unsigned cells=1u<<power,width=cells+1;std::vector<Vec> vertices;for(unsigned y=0;y<width;++y)for(unsigned x=0;x<width;++x)vertices.push_back({float(x)*8,float(y)*8,float((x+y)%3)});auto grid=sourceDisplacementGridTrianglesCandidate(power,std::vector<uint16_t>(cells*cells*2));std::vector<std::array<uint16_t,3>> triangles;for(auto t:grid)triangles.push_back(t.vertices);auto tree=sourceDisplacementCollisionTreeCandidate(power,vertices,triangles);
  for(unsigned q=0;q<16;++q)for(unsigned flags:{0u,4u,8u,12u})for(float initial:{0.f,.25f,1.f})for(unsigned action=0;action<3;++action){policy=action;Vec start{sample(),sample(),sample()},delta{sample(),sample(),sample()};if(q%4==0){start={8,8,30};delta={0,0,-60};}if(q%4==1){start={-20,8,0};delta={80,0,0};}if(q%4==2)delta={-0.f,0,0};Vec inverse{};for(unsigned a=0;a<3;++a)inverse[a]=delta[a]==0?std::numeric_limits<float>::max():1.f/delta[a];const Vec extents=q%3==0?Vec{}:q%3==1?Vec{24,24,41}:Vec{.125f,2.75f,7.0625f};
   events.clear();begins=ends=0;float fraction=initial;bool result;
#ifndef _WIN64
   result=native(tree,flags,start,delta,inverse,extents,fraction);const auto actualEvents=events;const unsigned actualBegins=begins,actualEnds=ends;events.clear();begins=ends=0;float ownFraction=initial;Host host;const bool ownResult=sourceDisplacementHullTreeCandidate(tree,flags,start,delta,inverse,extents,ownFraction,host);if(events!=actualEvents||begins!=actualBegins||ends!=actualEnds||bits(fraction)!=bits(ownFraction)||result!=ownResult){std::cerr<<"case "<<cases<<" power "<<power<<" q "<<q<<" flags "<<flags<<"\n";throw std::runtime_error("displacement hull tree mismatch");}
#else
   Host host;result=sourceDisplacementHullTreeCandidate(tree,flags,start,delta,inverse,extents,fraction,host);
#endif
   const std::array<uint32_t,6> header{cases,uint32_t(events.size()),bits(fraction),result,begins,ends};output.write(reinterpret_cast<const char*>(header.data()),sizeof(header));for(auto e:events)output.write(reinterpret_cast<const char*>(e.data()),sizeof(e));require(bool(output),"record write");++cases;totalCalls+=unsigned(events.size());empty+=events.empty();changed+=result;
  }
 }
#ifndef _WIN64
 observer.configure(false);image.verify();
#endif
 require(totalCalls&&empty&&changed,"tree coverage");std::cout<<"{\"cases\":"<<cases<<",\"triangle_calls\":"<<totalCalls<<",\"empty_cases\":"<<empty<<",\"changed_cases\":"<<changed<<",\"passed\":true}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>
#include "source_displacement_box_overlap_candidate.h"
#include "source_bsp_displacement_collision_candidate.h"

using Vec=std::array<float,3>;using Row=std::array<uint32_t,9>;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T& at(uint8_t* p,size_t n){return *reinterpret_cast<T*>(p+n);}
std::vector<uint8_t> read(const char* path){std::ifstream f(path,std::ios::binary);require(bool(f),"read benchmark");return {(std::istreambuf_iterator<char>(f)),{}};}
using Triangle=SourceDisplacementCollisionTriangleCandidate;using Mesh=SourceDisplacementRuntimeMeshCandidate;
std::array<Vec,3> points(const std::vector<Vec>& vertices,const Triangle& triangle){return {vertices.at(triangle.packedVertices[0]&511u),vertices.at(triangle.packedVertices[2]&511u),vertices.at(triangle.packedVertices[1]&511u)};}
bool ownedTriangle(const Vec& center,const Vec& extents,const std::vector<Vec>& vertices,const Triangle& triangle,float tolerance){return sourceTriangleBoxOverlapCandidate(center,extents,points(vertices,triangle),triangle.normal,triangle.distance,uint8_t((triangle.flags>>3)&7u),uint8_t(triangle.flags&7u),tolerance);}
std::vector<uint32_t> ownedOrder;
bool ownedMesh(const Mesh& mesh,const Vec& low,const Vec& high){
 ownedOrder.clear();Vec center{},extents{};for(size_t a=0;a<3;++a){center[a]=(high[a]+low[a])*.5f;extents[a]=high[a]-center[a];}
 const bool result=sourceDisplacementBoxWalkCandidate(mesh.tree,mesh.runtimeFlags,low,high,[&](uint32_t id){ownedOrder.push_back(id);return ownedTriangle(center,extents,mesh.vertices,mesh.triangles.at(id),0);});
 require(result==sourceDisplacementBoxOverlapCandidate(mesh,low,high),"mesh convenience composition");return result;
}
Row summary(uint32_t kind,uint32_t mesh,uint32_t query,uint32_t variant,bool hit,const std::vector<uint32_t>& order){uint32_t hash=2166136261u;for(auto id:order)hash=(hash^id)*16777619u;return {kind,mesh,query,variant,uint32_t(hit),uint32_t(order.size()),hash,order.empty()?UINT32_MAX:order.front(),order.empty()?UINT32_MAX:order.back()};}

#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
template<class T>T address(uintptr_t p){T value;std::memcpy(&value,reinterpret_cast<const void*>(p),sizeof(value));return value;}
const Mesh* activeMesh=nullptr;Vec expectedCenter{},expectedExtents{};std::map<uint32_t,uint32_t> triangleIdentities;std::vector<uint32_t> nativeOrder;
void boundary(unsigned slot,CONTEXT& c){
 require(slot==0&&activeMesh,"triangle observation scope");const auto base=reinterpret_cast<uintptr_t>(activeMesh->vertices.data());std::array<uint32_t,3> ids{};
 for(size_t p=0;p<3;++p){const auto pointer=address<uintptr_t>(c.Esp+12+4*p);require(pointer>=base&&(pointer-base)%12==0&&(pointer-base)/12<activeMesh->vertices.size(),"native triangle vertex identity");ids[p]=uint32_t((pointer-base)/12);}
 const auto key=ids[0]|(ids[1]<<9)|(ids[2]<<18);const auto found=triangleIdentities.find(key);require(found!=triangleIdentities.end(),"native triangle order key");const auto& t=activeMesh->triangles.at(found->second);
 const auto plane=address<uintptr_t>(c.Esp+24);require(std::memcmp(reinterpret_cast<void*>(plane),t.normal.data(),12)==0&&std::memcmp(reinterpret_cast<void*>(plane+12),&t.distance,4)==0,"native triangle plane");require(address<uint8_t>(plane+16)==((t.flags>>3)&7u)&&address<uint8_t>(plane+17)==(t.flags&7u),"native triangle tags");
 require(address<float>(c.Esp+28)==0&&std::memcmp(reinterpret_cast<void*>(address<uintptr_t>(c.Esp+4)),expectedCenter.data(),12)==0&&std::memcmp(reinterpret_cast<void*>(address<uintptr_t>(c.Esp+8)),expectedExtents.data(),12)==0,"native midpoint/extents/tolerance");nativeOrder.push_back(found->second);c.EFlags|=0x10000u;
}
bool nativeTriangle(const Vec& center,const Vec& extents,const std::vector<Vec>& vertices,const Triangle& triangle,float tolerance){
 struct Plane{Vec normal;float distance;uint8_t type,signs;uint16_t reserved=0;};const Plane plane{triangle.normal,triangle.distance,uint8_t((triangle.flags>>3)&7u),uint8_t(triangle.flags&7u),0};const auto p=points(vertices,triangle);
 using Predicate=bool(__cdecl*)(const float*,const float*,const float*,const float*,const float*,const void*,float);
 return reinterpret_cast<Predicate>(0x1014f770)(center.data(),extents.data(),p[0].data(),p[1].data(),p[2].data(),&plane,tolerance);
}
struct NativeMesh {
 const Mesh& mesh;std::array<uint8_t,244> object{};
 explicit NativeMesh(const Mesh& input):mesh(input){at<uint32_t>(object.data(),0x28)=input.power;at<uintptr_t>(object.data(),0x70)=reinterpret_cast<uintptr_t>(input.vertices.data());at<uintptr_t>(object.data(),0x80)=reinterpret_cast<uintptr_t>(input.triangles.data());at<uintptr_t>(object.data(),0x90)=reinterpret_cast<uintptr_t>(input.tree.nodes.data());at<uint32_t>(object.data(),0x98)=uint32_t(input.tree.nodes.size());at<uintptr_t>(object.data(),0xa0)=reinterpret_cast<uintptr_t>(input.tree.leafPairs.data());
  triangleIdentities.clear();for(uint32_t i=0;i<input.triangles.size();++i){const auto& t=input.triangles[i];const auto key=(t.packedVertices[0]&511u)|((t.packedVertices[2]&511u)<<9)|((t.packedVertices[1]&511u)<<18);require(triangleIdentities.emplace(key,i).second,"unique prepared triangle identities");}
 }
 bool trace(const Vec& low,const Vec& high,uint32_t flags){
  activeMesh=&mesh;nativeOrder.clear();for(size_t a=0;a<3;++a){expectedCenter[a]=(high[a]+low[a])*.5f;expectedExtents[a]=high[a]-expectedCenter[a];}
  at<uint32_t>(object.data(),0x2c)=flags;const auto before=object;using Overlap=bool(__thiscall*)(void*,const float*,const float*);const bool hit=reinterpret_cast<Overlap>(0x1015a7d0)(object.data(),low.data(),high.data());require(before==object,"stationary overlap changed mesh object");return hit;
 }
};
#endif

int main(int argc,char** argv){try{
 require(argc==4,"expected engine or -, benchmark BSP and disposable output");const auto bsp=read(argv[2]);std::ofstream out(argv[3],std::ios::binary);require(bool(out),"output");
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1],0x643eec);image.seal();PrivateX86BoundaryObserverFixture observer{{0x1014f770,0,0,0},boundary};
#endif
 unsigned cases=0,direct=0,flat=0,meshCases=0,meshCount=0,hits=0,misses=0,disabled=0,triangleCalls=0,secondTriangleHits=0,multipleCandidates=0;uint32_t random=0x67a91b;auto sample=[&](){random=random*1664525u+1013904223u;return float(int((random>>8)%4097)-2048)/32.f;};
 auto emit=[&](const Row& row){out.write(reinterpret_cast<const char*>(row.data()),sizeof(row));require(bool(out),"write records");++cases;hits+=row[4]!=0;misses+=row[4]==0;};
 auto triangleTest=[&](uint32_t kind,uint32_t ordinal,uint32_t id,uint32_t variant,const Vec& center,const Vec& extents,const std::vector<Vec>& vertices,const Triangle& triangle,float tolerance){
  const bool expected=ownedTriangle(center,extents,vertices,triangle,tolerance);bool hit=expected;
#ifndef _WIN64
  hit=nativeTriangle(center,extents,vertices,triangle,tolerance);if(hit!=expected){std::cerr<<"direct case "<<cases<<" mesh "<<ordinal<<" triangle "<<id<<" variant "<<variant<<" native "<<hit<<" owned "<<expected<<'\n';throw std::runtime_error("triangle box mismatch");}
#endif
  emit(summary(kind,ordinal,id,variant,hit,{}));++direct;
 };
 // Exact dyadic flat triangle fixtures also admit independent interval checks.
 const std::vector<Vec> flatVertices{Vec{-8,-8,0},Vec{-8,8,0},Vec{8,-8,0}};Triangle flatInitial;flatInitial.packedVertices={0,1,2};const auto flatTriangle=sourceDisplacementCollisionTriangleCandidate(flatVertices,flatInitial);require(flatTriangle.normal==Vec{0,0,1}&&((flatTriangle.flags>>3)&7u)==2,"flat triangle plane convention");
 for(float x:{-9.f,-8.f,-4.f,0.f,4.f,8.f,9.f})for(float y:{-9.f,-8.f,-4.f,0.f,4.f,8.f,9.f})for(float z:{-2.f,-1.f,0.f,1.f,2.f})for(float radius:{0.f,.5f,1.f,2.f,16.f}){triangleTest(2,UINT32_MAX,flat,0,{x,y,z},{radius,radius,radius},flatVertices,flatTriangle,0);++flat;}
 auto testMesh=[&](Mesh mesh){const uint32_t ordinal=meshCount++;const auto verticesBefore=mesh.vertices;const auto trianglesBefore=mesh.triangles;const auto nodesBefore=mesh.tree.nodes;const auto pairsBefore=mesh.tree.leafPairs;
  const std::array<Vec,4> extents{Vec{},Vec{.125f,.25f,.5f},Vec{2,3,5},Vec{24,24,41}};
  const uint32_t stride=std::max<uint32_t>(1,uint32_t(mesh.triangles.size()/16));
  for(uint32_t id=0;id<mesh.triangles.size();id+=stride){const auto& t=mesh.triangles[id];const auto p=points(mesh.vertices,t);Vec centroid{};for(size_t a=0;a<3;++a)centroid[a]=(p[0][a]+p[1][a]+p[2][a])/3.f;
   for(uint32_t q=0;q<12;++q){Vec center=centroid;if(q<3)center=p[q];if(q==3)for(size_t a=0;a<3;++a)center[a]+=t.normal[a];if(q==4)for(size_t a=0;a<3;++a)center[a]-=t.normal[a];if(q==5)for(size_t a=0;a<3;++a)center[a]+=128*t.normal[a];if(q>=6)for(size_t a=0;a<3;++a)center[a]+=sample();
    for(uint32_t hull=0;hull<4;++hull)for(uint32_t tol=0;tol<2;++tol)triangleTest(0,ordinal,id,(q<<8)|(hull<<4)|tol,center,extents[hull],mesh.vertices,t,tol?.03125f:0.f);
   }
  }
#ifndef _WIN64
  NativeMesh native(mesh);observer.configure(true);
#endif
  const auto& bounds=mesh.tree.expandedBounds;Vec middle{};for(size_t a=0;a<3;++a)middle[a]=(bounds[a]+bounds[3+a])*.5f;
  for(uint32_t q=0;q<24;++q){const auto& t=mesh.triangles[q%mesh.triangles.size()];const auto p=points(mesh.vertices,t);Vec center{};for(size_t a=0;a<3;++a)center[a]=(p[0][a]+p[1][a]+p[2][a])/3.f;if(q==0)center=middle;if(q==1)for(size_t a=0;a<3;++a)center[a]=bounds[3+a]+256;if(q==2)center=p[0];if(q>=3&&q<8)for(size_t a=0;a<3;++a)center[a]+=float(int(q)-5)*.25f*t.normal[a];if(q>=8)for(size_t a=0;a<3;++a)center[a]+=sample();
   for(uint32_t hull=0;hull<4;++hull)for(uint32_t flags:{0u,1u,2u,4u,5u,8u,12u,0xffffffffu}){Vec low{},high{};for(size_t a=0;a<3;++a){low[a]=center[a]-extents[hull][a];high[a]=center[a]+extents[hull][a];}mesh.runtimeFlags=flags;const bool expected=ownedMesh(mesh,low,high);auto row=summary(1,ordinal,q|(hull<<8),flags,expected,ownedOrder);
#ifndef _WIN64
    const bool hit=native.trace(low,high,flags);const auto actual=summary(1,ordinal,q|(hull<<8),flags,hit,nativeOrder);if(actual!=row||nativeOrder!=ownedOrder){std::cerr<<"mesh case "<<cases<<" mesh "<<ordinal<<" q "<<q<<" hull "<<hull<<" flags "<<flags<<" result "<<hit<<'/'<<expected<<" order "<<nativeOrder.size()<<'/'<<ownedOrder.size()<<'\n';throw std::runtime_error("complete displacement box overlap mismatch");}row=actual;
#endif
    if(flags&4u){require(!row[4]&&!row[5],"disabled overlap early return");++disabled;}triangleCalls+=row[5];secondTriangleHits+=row[4]&&(row[8]&1u);multipleCandidates+=row[5]>2;emit(row);++meshCases;
   }
  }
#ifndef _WIN64
  observer.configure(false);
#endif
  require(mesh.vertices==verticesBefore&&mesh.tree.leafPairs==pairsBefore,"overlap changed vertex/pair input");require(std::memcmp(mesh.triangles.data(),trianglesBefore.data(),mesh.triangles.size()*sizeof(Triangle))==0&&std::memcmp(mesh.tree.nodes.data(),nodesBefore.data(),mesh.tree.nodes.size()*sizeof(SourceDisplacementCollisionNodeCandidate))==0,"overlap changed triangle/tree bytes");
 };
 for(unsigned hdr=0;hdr<2;++hdr){const SourceBspDisplacementCollisionCandidate imported(bsp,int32_t(hdr));require(imported.meshes.size()==43,"benchmark mesh inventory");for(const auto& mesh:imported.meshes)testMesh(mesh);}
 for(unsigned power=1;power<=4;++power)for(unsigned shape=0;shape<4;++shape){const unsigned cells=1u<<power,width=cells+1;Mesh mesh;mesh.power=power;for(unsigned y=0;y<width;++y)for(unsigned x=0;x<width;++x){const float z=shape==0?0.f:shape==1?float(x)*.75f-float(y)*.25f:shape==2?float((x*y)%7):float((x+y)%3)*.03125f;Vec v{float(x)*8-16,float(y)*8+3,z};if(shape==3)std::swap(v[0],v[2]);mesh.vertices.push_back(v);}std::vector<std::array<uint16_t,3>> packed;for(const auto& grid:sourceDisplacementGridTrianglesCandidate(power,std::vector<uint16_t>(2*cells*cells))){Triangle triangle;triangle.packedVertices=grid.vertices;mesh.triangles.push_back(sourceDisplacementCollisionTriangleCandidate(mesh.vertices,triangle));packed.push_back(grid.vertices);}mesh.tree=sourceDisplacementCollisionTreeCandidate(power,mesh.vertices,packed);testMesh(std::move(mesh));}
#ifndef _WIN64
 image.verify();
#endif
 require(hits&&misses&&meshCases&&direct&&disabled&&triangleCalls&&secondTriangleHits&&multipleCandidates,"displacement box coverage");std::cout<<"{\"cases\":"<<cases<<",\"direct_cases\":"<<direct<<",\"flat_cases\":"<<flat<<",\"mesh_cases\":"<<meshCases<<",\"meshes\":"<<meshCount<<",\"hits\":"<<hits<<",\"misses\":"<<misses<<",\"disabled\":"<<disabled<<",\"triangle_calls\":"<<triangleCalls<<",\"second_triangle_hits\":"<<secondTriangleHits<<",\"multiple_candidates\":"<<multipleCandidates<<",\"passed\":true}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

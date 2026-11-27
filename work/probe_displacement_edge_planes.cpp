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
#include "source_displacement_edge_planes_candidate.h"

using Vec=std::array<float,3>;
void require(bool v,const char* s){if(!v)throw std::runtime_error(s);}
std::vector<Vec> observed;
uint16_t intern(const Vec& p){const auto id=uint16_t(7*observed.size()+3);observed.push_back(p);return uint16_t(id|(observed.size()&1?32768:0));}
struct Host{uint16_t intern(const Vec& p){return ::intern(p);}};
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
uintptr_t meshAddress=0;
void boundary(unsigned slot,CONTEXT& c){require(slot==0&&c.Ecx==meshAddress,"intern owner");const auto p=*reinterpret_cast<const Vec* const*>(c.Esp+4);const auto id=intern(*p);PrivateX86BoundaryObserverFixture::returnCdecl(c,id);c.Esp+=4;}
std::array<uint16_t,9> native(const std::vector<Vec>& vertices,const SourceDisplacementCollisionTriangleCandidate& t){std::array<uint8_t,0xf4> mesh{};std::array<uint16_t,9> result{};meshAddress=reinterpret_cast<uintptr_t>(mesh.data());*reinterpret_cast<uintptr_t*>(mesh.data()+0x70)=reinterpret_cast<uintptr_t>(vertices.data());*reinterpret_cast<uintptr_t*>(mesh.data()+0xb0)=reinterpret_cast<uintptr_t>(result.data());using Build=void(__thiscall*)(void*,const void*,unsigned);reinterpret_cast<Build>(0x1015c3b0)(mesh.data(),&t,0);return result;}
#endif
int main(int argc,char** argv){try{require(argc==3,"expected engine or - and disposable records");
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1],0x643eec);image.seal();PrivateX86BoundaryObserverFixture observer{{0x1015b9f0,0,0,0},boundary};observer.configure(true);
#endif
 std::ofstream output(argv[2],std::ios::binary);require(bool(output),"record output");unsigned cases=0,planes=0,skipped=0;uint32_t random=0x234abc;auto sample=[&](){random=random*1664525u+1013904223u;return float(int((random>>8)%4097)-2048)/16.f;};
 for(unsigned q=0;q<512;++q)for(unsigned permutation=0;permutation<6;++permutation){std::vector<Vec> points{{sample(),sample(),sample()},{sample(),sample(),sample()},{sample(),sample(),sample()}};if(q%5==0)points={{0,0,0},{0,32,0},{32,0,0}};if(q%5==1)points={{0,0,0},{1,1,1},{2,2,2}};if(q%5==2)points[1]=points[0];if(q%5==3){points[2]=points[0];points[2][0]+=.03125f;}
  const std::array<std::array<uint16_t,3>,6> order{std::array<uint16_t,3>{0,1,2},std::array<uint16_t,3>{0,2,1},std::array<uint16_t,3>{1,0,2},std::array<uint16_t,3>{1,2,0},std::array<uint16_t,3>{2,0,1},std::array<uint16_t,3>{2,1,0}};SourceDisplacementCollisionTriangleCandidate t;t.packedVertices=order[permutation];for(auto& v:t.packedVertices)v=uint16_t(v|0xa000);observed.clear();std::array<uint16_t,9> indices{};
#ifndef _WIN64
  indices=native(points,t);const auto nativePlanes=observed;observed.clear();Host host;const auto own=sourceDisplacementEdgePlanesCandidate(points,t,host);if(indices!=own||observed.size()!=nativePlanes.size()||(!observed.empty()&&std::memcmp(observed.data(),nativePlanes.data(),observed.size()*12))){std::cerr<<"case "<<cases<<" q "<<q<<" permutation "<<permutation<<"\n";throw std::runtime_error("edge plane generation mismatch");}
#else
  Host host;indices=sourceDisplacementEdgePlanesCandidate(points,t,host);
#endif
  const uint32_t count=uint32_t(observed.size());output.write(reinterpret_cast<const char*>(&count),4);output.write(reinterpret_cast<const char*>(indices.data()),18);for(auto p:observed)output.write(reinterpret_cast<const char*>(p.data()),12);for(auto id:indices)skipped+=id==65535;planes+=count;++cases;
 }
#ifndef _WIN64
 observer.configure(false);image.verify();
#endif
 require(planes&&skipped,"plane generation coverage");std::cout<<"{\"cases\":"<<cases<<",\"planes\":"<<planes<<",\"skipped\":"<<skipped<<",\"passed\":true}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

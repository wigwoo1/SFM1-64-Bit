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
#include "source_displacement_plane_pool_candidate.h"

using Plane=std::array<float,3>;
void require(bool v,const char* s){if(!v)throw std::runtime_error(s);}
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
template<class T>T& at(uintptr_t p){return *reinterpret_cast<T*>(p);}
struct Entry{Plane plane;uint32_t id;};
struct Bucket{Entry* entries;uint32_t allocated,grow,count,unused;};
std::vector<std::array<Entry,512>> entryStorage;std::vector<Bucket> nativeBuckets;
std::array<Plane,512> nativePlanes{};std::array<uint8_t,0xf4> mesh{};unsigned inserts=0,appends=0;
void boundary(unsigned slot,CONTEXT& c){using O=PrivateX86BoundaryObserverFixture;
 if(slot==0){auto* b=reinterpret_cast<Bucket*>(c.Ecx);require(b>=nativeBuckets.data()&&b<nativeBuckets.data()+nativeBuckets.size(),"bucket insert owner");const auto index=at<uint32_t>(c.Esp+4);require(index==b->count&&index<512,"bucket insertion position");const auto e=*reinterpret_cast<const Entry*>(at<uintptr_t>(c.Esp+8));b->entries[index]=e;++b->count;++inserts;O::returnCdecl(c,index);c.Esp+=8;}
 else if(slot==1){const auto base=reinterpret_cast<uintptr_t>(mesh.data());require(c.Ecx==base+0xc4,"plane append owner");const auto index=at<uint32_t>(c.Esp+4);require(index==at<uint32_t>(base+0xd0)&&index<512,"plane append index");nativePlanes[index]=*reinterpret_cast<const Plane*>(at<uintptr_t>(c.Esp+8));at<uint32_t>(base+0xd0)=index+1;++appends;O::returnCdecl(c,index);c.Esp+=8;}
 else throw std::runtime_error("unexpected breakpoint");
}
void prepare(unsigned count,bool masked){entryStorage.clear();entryStorage.resize(count);nativeBuckets.clear();for(unsigned i=0;i<count;++i)nativeBuckets.push_back({entryStorage[i].data(),512,0,0,0});mesh.fill(0);const auto m=reinterpret_cast<uintptr_t>(mesh.data());at<uintptr_t>(m+0xc4)=reinterpret_cast<uintptr_t>(nativePlanes.data());at<uint32_t>(m+0xc8)=512;const uintptr_t owner=0x106436dc;at<uintptr_t>(owner)=reinterpret_cast<uintptr_t>(nativeBuckets.data());at<uint32_t>(owner+0xc)=count;at<uint8_t>(owner+0x16)=masked;at<uint32_t>(owner+0x18)=count-1;inserts=appends=0;}
#endif
int main(int argc,char** argv){try{require(argc==4,"expected engine or -, private permutation fixture and disposable records");
 std::array<uint32_t,256> raw{};std::ifstream fixture(argv[2],std::ios::binary);fixture.read(reinterpret_cast<char*>(raw.data()),sizeof(raw));require(fixture.gcount()==sizeof(raw)&&fixture.peek()==std::char_traits<char>::eof(),"private permutation fixture width");SourcePlaneHashPermutationCandidate permutation{};std::array<bool,256> seen{};for(size_t i=0;i<raw.size();++i){require(raw[i]<256&&!seen[raw[i]],"private permutation data");seen[raw[i]]=true;permutation[i]=uint8_t(raw[i]);}
#ifndef _WIN64
 NativePrivateWorldImageFixture image(argv[1],0x643eec);image.seal();require(std::memcmp(raw.data(),reinterpret_cast<void*>(0x104129b0),1024)==0,"private fixture differs from engine");PrivateX86BoundaryObserverFixture observer{{0x1015b150,0x10001610,0,0},boundary};observer.configure(true);
#endif
 std::ofstream out(argv[3],std::ios::binary);require(bool(out),"record output");unsigned cases=0,newPlanes=0,reversed=0,duplicates=0;
 for(unsigned count:{1u,3u,8u,17u,64u})for(bool masked:{false,true}){if(masked&&(count&(count-1)))continue;SourceDisplacementPlanePoolCandidate pool(count,masked,permutation);
#ifndef _WIN64
 prepare(count,masked);
#endif
 uint32_t random=0x4567abc;auto sample=[&](){random=random*1664525u+1013904223u;return float(int((random>>8)%4097)-2048)/16.f;};Plane current{};
 for(unsigned q=0;q<512;++q){Plane p{};if(q%4==0)current={sample(),sample(),sample()};p=current;if(q%4==2)for(auto& v:p)v=-v;if(q%4==3)p={q&4?-0.f:0.f,q&8?-0.f:0.f,float(q%7)};if(q%31==0)p={0,0,0};if(q%37==0)p={1.f,std::nextafter(1.f,2.f),-1.f};if(q%41==0)p={1.f,1.f,-1.f};
 const auto before=pool.planes.size();const auto hash=sourcePlaneHashCandidate(p,permutation);const auto id=pool.intern(p);
#ifndef _WIN64
 using Hash=uint32_t(__thiscall*)(const void*);require(reinterpret_cast<Hash>(0x10261bd0)(p.data())==hash,"Hash12 mismatch");using Intern=uint16_t(__thiscall*)(void*,const void*);const auto actual=reinterpret_cast<Intern>(0x1015b9f0)(mesh.data(),p.data());const auto size=at<uint32_t>(reinterpret_cast<uintptr_t>(mesh.data())+0xd0);require(actual==id&&size==pool.planes.size(),"native cache ID/count mismatch");require(!size||std::memcmp(nativePlanes.data(),pool.planes.data(),size*12)==0,"native cache plane bytes mismatch");for(unsigned b=0;b<count;++b){require(nativeBuckets[b].count==pool.buckets[b].size(),"native bucket count");for(unsigned i=0;i<nativeBuckets[b].count;++i){const auto& n=nativeBuckets[b].entries[i];const auto& own=pool.buckets[b][i];require(n.id==own.id&&std::memcmp(n.plane.data(),own.plane.data(),12)==0,"native bucket record mismatch");}}require(inserts==appends&&appends==size,"insertion service count");
#endif
 const std::array<uint32_t,4> row{hash,id,uint32_t(pool.planes.size()),pool.bucket(p)};out.write(reinterpret_cast<const char*>(row.data()),sizeof(row));++cases;newPlanes+=pool.planes.size()>before;duplicates+=pool.planes.size()==before;reversed+=(id&32768)!=0;
 }
 const uint32_t size=uint32_t(pool.planes.size());out.write(reinterpret_cast<const char*>(&size),4);for(auto p:pool.planes)out.write(reinterpret_cast<const char*>(p.data()),12);
 }
#ifndef _WIN64
 observer.configure(false);image.verify();
#endif
 require(newPlanes&&duplicates&&reversed,"pool coverage");require(bool(out),"record write");std::cout<<"{\"cases\":"<<cases<<",\"new_planes\":"<<newPlanes<<",\"duplicates\":"<<duplicates<<",\"reversed\":"<<reversed<<",\"passed\":true}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

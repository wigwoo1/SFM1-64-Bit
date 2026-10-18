#include "source_bsp_displacement_collision_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("map required");std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("map read");std::vector<uint8_t>b((std::istreambuf_iterator<char>(f)),{});
 auto word=[&](size_t p){uint32_t v;std::memcpy(&v,b.data()+p,4);return v;};const size_t info=word(8+26*16),faces=word(8+7*16),faceCount=word(8+7*16+4)/56,raw=word(8+33*16);SourceBspDisplacementCollisionCandidate original(b,0);if(original.selectedFaceLump!=7||original.meshes.size()!=43)throw std::runtime_error("benchmark loader count");
 auto mutated=b;for(size_t i=0;i<43;++i){uint32_t bogus=UINT32_MAX;uint16_t wrong=0xffff;std::memcpy(mutated.data()+info+176*i+12,&bogus,4);std::memcpy(mutated.data()+info+176*i+16,&bogus,4);std::memcpy(mutated.data()+info+176*i+36,&wrong,2);}SourceBspDisplacementCollisionCandidate ignored(mutated,0);
 for(size_t i=0;i<43;++i){const auto&a=original.meshes[i];const auto&c=ignored.meshes[i];if(a.vertices.size()!=c.vertices.size()||a.triangles.size()!=c.triangles.size()||std::memcmp(a.vertices.data(),c.vertices.data(),a.vertices.size()*12)||std::memcmp(a.triangles.data(),c.triangles.data(),a.triangles.size()*24)||a.corners!=c.corners||a.baseNormal!=c.baseNormal||a.tree.expandedBounds!=c.tree.expandedBounds)throw std::runtime_error("unused record offsets/mapFace changed geometry");}
 // Disabled HDR must not inspect the unused HDR payload/metadata.
 mutated=b;uint32_t bad=UINT32_MAX;std::memcpy(mutated.data()+8+58*16,&bad,4);SourceBspDisplacementCollisionCandidate ldr(mutated,0);if(ldr.meshes.size()!=43)throw std::runtime_error("unused HDR rejected");
 unsigned rejected=0;auto reject=[&](auto run){try{run();}catch(const std::runtime_error&){++rejected;return;}throw std::runtime_error("corrupt/unsupported BSP accepted");};
 reject([&]{SourceBspDisplacementCollisionCandidate x(mutated,1);});reject([&]{SourceBspDisplacementCollisionCandidate x(std::vector<uint8_t>(16),0);});
 mutated=b;uint32_t version=2;std::memcpy(mutated.data()+8+7*16+8,&version,4);reject([&]{SourceBspDisplacementCollisionCandidate x(mutated,0);});
 mutated=b;uint32_t compressed=1;std::memcpy(mutated.data()+8+7*16+12,&compressed,4);reject([&]{SourceBspDisplacementCollisionCandidate x(mutated,0);});
 mutated=b;for(size_t i=0;i<faceCount;++i){int16_t d;std::memcpy(&d,mutated.data()+faces+56*i+12,2);if(d==0){d=-1;std::memcpy(mutated.data()+faces+56*i+12,&d,2);}}reject([&]{SourceBspDisplacementCollisionCandidate x(mutated,0);});
 mutated=b;uint16_t mapped;std::memcpy(&mapped,b.data()+info+36,2);int16_t count=3;std::memcpy(mutated.data()+faces+size_t(mapped)*56+8,&count,2);reject([&]{SourceBspDisplacementCollisionCandidate x(mutated,0);});
 mutated=b;uint32_t shortLength=20;std::memcpy(mutated.data()+8+33*16+4,&shortLength,4);reject([&]{SourceBspDisplacementCollisionCandidate x(mutated,0);});
 mutated=b;uint32_t nan=0x7fc00000;std::memcpy(mutated.data()+raw,&nan,4);reject([&]{SourceBspDisplacementCollisionCandidate x(mutated,0);});if(rejected!=8)throw std::runtime_error("guard count");
 std::cout<<"PASS 43 BSP meshes, ignored record offsets/mapFace, unused HDR and eight corruption/unsupported cases\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

#include "source_bsp_light_cache_world_candidate.h"
#include "source_bsp_displacement_collision_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("map required");std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("map read");std::vector<uint8_t>b((std::istreambuf_iterator<char>(f)),{});SourceBspBrushOnlyTraceCandidate world(b,std::vector<uint32_t>(32768));SourceBspDisplacementCollisionCandidate loaded(b);SourcePreparedWorldRayCandidate prepared(world,loaded.meshes);SourceBspLightCacheWorldCandidate cache(b,world,prepared);
 for(const std::array<float,3>center:std::array<std::array<float,3>,4>{{{16,16,64},{-16,-16,-64},{-2032,-4528,-320},{-304,-2288,-192}}}){if(cache.position(center)!=center)throw std::runtime_error("exact cache center changed");}
 unsigned rejected=0;auto rejects=[&](auto action){bool failed=false;try{action();}catch(const std::exception&){failed=true;}if(!failed)throw std::runtime_error("invalid cache query accepted");++rejected;};
 rejects([&]{cache.position({100000,0,0});});rejects([&]{cache.position({std::numeric_limits<float>::infinity(),0,0});});rejects([&]{cache.position({0,0,0},-1);});rejects([&]{cache.position({0,0,0},int32_t(world.data.leafContents.size()));});
 std::array<float,3>target{11,12,13};const auto before=target;rejects([&]{cache.constrainToStartLeaf({std::numeric_limits<float>::infinity(),0,0},.5f,target);});if(target!=before)throw std::runtime_error("failed clip changed target");
 auto wrong=world;wrong.root=-1;rejects([&]{SourceBspLightCacheWorldCandidate invalid(b,wrong,prepared);});
 const auto point=std::array<float,3>{-2026.15625f,-4541.78125f,-351.90625f};const auto expected=cache.position(point);for(size_t i=0;i<32;++i)if(cache.position(point)!=expected)throw std::runtime_error("repeated cache queries changed result");
 if(rejected!=6)throw std::runtime_error("cache guard coverage");std::cout<<"PASS four exact-center expectations, six guards, unchanged failed-clip target and repeated-query determinism\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

#include "source_new_record_leaf_ambient_candidate.h"
#include "source_bsp_light_cache_world_candidate.h"
#include "source_bsp_displacement_collision_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"x64 required");
    if(argc!=4)throw std::runtime_error("usage: probe map.bsp hdr|ldr queries.bin");
    std::ifstream map(argv[1],std::ios::binary);if(!map)throw std::runtime_error("map read");std::vector<uint8_t>b((std::istreambuf_iterator<char>(map)),{});
    SourceBspBrushOnlyTraceCandidate world(b,std::vector<uint32_t>(32768));SourceBspDisplacementCollisionCandidate loaded(b);SourcePreparedWorldRayCandidate queries(world,loaded.meshes);SourceBspLightCacheWorldCandidate cache(b,world,queries);
    const std::string encoding=argv[2];if(encoding!="hdr"&&encoding!="ldr")throw std::runtime_error("explicit ambient encoding required");
    SourceBspLeafAmbientCandidate ambient(b,encoding=="hdr"?SourceBspAmbientEncodingCandidate::Hdr:SourceBspAmbientEncodingCandidate::Ldr);
    std::ifstream input(argv[3],std::ios::binary);if(!input)throw std::runtime_error("queries read");
    std::array<uint32_t,26>row{};size_t cases=0;
    while(input.read(reinterpret_cast<char*>(row.data()),sizeof(row))){
        std::array<float,3>point{};int32_t center=0;std::memcpy(point.data(),row.data(),12);std::memcpy(&center,row.data()+3,4);
        const auto result=sourceNewRecordLeafAmbientCandidate(point,center,{4,false,0,0},cache,ambient);
        if(result.inputLeaf!=row[4]||std::memcmp(result.samplePoint.data(),row.data()+5,12)||std::memcmp(result.cube.data(),row.data()+8,72))throw std::runtime_error("composite differs from independent anchor/cube reference at "+std::to_string(cases));
        ++cases;
    }
    if(!input.eof()||input.gcount()||!cases)throw std::runtime_error("fixture length");
    std::cout<<"PASS encoding="<<encoding<<" map_compositions="<<cases<<" exact_anchor_comparisons="<<cases<<" exact_cube_channels="<<cases*18<<" native_anchor_corpus_reused=1 fresh_original_execution=0\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

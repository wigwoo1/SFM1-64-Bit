#include "source_bsp_world_collision_data_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("usage: probe map workspace-records.bin");std::ifstream input(argv[1],std::ios::binary);if(!input)throw std::runtime_error("map open failed");std::vector<uint8_t>b((std::istreambuf_iterator<char>(input)),{});SourceBspWorldCollisionDataCandidate data(b);
 std::ofstream output(argv[2],std::ios::binary);auto word=[&](uint32_t value){output.write(reinterpret_cast<const char*>(&value),4);};
 word(uint32_t(data.planes.size()));word(uint32_t(data.sides.size()));word(uint32_t(data.brushes.size()));word(uint32_t(data.worldBrushes.size()));word(uint32_t(data.displacementRecords));word(uint32_t(data.worldNodes));
 for(auto index:data.worldBrushes){const auto&brush=data.brushes[index];word(index);word(brush.contents);word(brush.count);for(size_t i=0;i<brush.count;++i){const auto&side=data.sides[brush.first+i];word(side.plane);word(uint32_t(int32_t(side.texture)));word(uint32_t(int32_t(side.displacement)));word(uint32_t(int32_t(side.bevel)));const auto&plane=data.planes[side.plane];for(float v:plane.normal){uint32_t bits;std::memcpy(&bits,&v,4);word(bits);}uint32_t bits;std::memcpy(&bits,&plane.distance,4);word(bits);}}
 word(uint32_t(data.leafBrushes.size()));for(size_t i=0;i<data.leafBrushes.size();++i){const auto&refs=data.leafBrushes[i];word(data.leafContents.at(i));word(uint32_t(refs.size()));for(auto index:refs)word(index);}
 output.flush();if(!output)throw std::runtime_error("collision record output failed");std::cout<<"PASS checked collision records; tracing and displacement collision UNVERIFIED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

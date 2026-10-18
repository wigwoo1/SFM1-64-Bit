#include "sfm_model_geometry.h"
#include <fstream>
#include <iostream>
static std::vector<uint8_t> read(const std::string&p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("asset file missing: "+p);return {(std::istreambuf_iterator<char>(f)),{}};}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8);if(argc!=2)throw std::runtime_error("model base required");auto base=std::string(argv[1]);auto model=readSfmModelGeometryCandidate(read(base+".mdl"),read(base+".vvd"),read(base+".dx90.vtx"));size_t triangles=0;for(auto&m:model.meshes)triangles+=m.triangles.size();uint32_t fixups=0;auto vvd=read(base+".vvd");std::memcpy(&fixups,vvd.data()+48,4);std::cout<<"asset="<<base<<" vertices="<<model.geometry.vertices.size()<<" bones="<<model.bones.size()<<" meshes="<<model.meshes.size()<<" triangles="<<triangles<<" vvd_fixups="<<fixups<<" mdl49_vtx7=passed\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}

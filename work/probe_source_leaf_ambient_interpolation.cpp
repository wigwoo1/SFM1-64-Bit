#include "source_leaf_ambient_interpolation_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
int main(int argc,char**argv){try{
 if(argc!=7)throw std::runtime_error("usage: probe map hdr|ldr leaf x y z");
 std::ifstream input(argv[1],std::ios::binary);if(!input)throw std::runtime_error("map open failed");
 std::vector<uint8_t>bytes((std::istreambuf_iterator<char>(input)),{});
 const std::string encoding=argv[2];if(encoding!="hdr"&&encoding!="ldr")throw std::runtime_error("explicit encoding required");
 SourceBspLeafAmbientCandidate data(bytes,encoding=="hdr"?SourceBspAmbientEncodingCandidate::Hdr:SourceBspAmbientEncodingCandidate::Ldr);
 const auto cube=SourceLeafAmbientInterpolationCandidate::evaluate(data,uint32_t(std::stoul(argv[3])),{std::stof(argv[4]),std::stof(argv[5]),std::stof(argv[6])});
 for(const auto&face:cube)for(float value:face){uint32_t bits;std::memcpy(&bits,&value,4);std::cout<<std::hex<<bits<<' ';}
 std::cout<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

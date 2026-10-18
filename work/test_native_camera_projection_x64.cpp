#include <string>
#include "recovered_camera_projection_candidate.h"
#include <fstream>
#include <cstring>
#include <iostream>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("native oracle required");std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("oracle missing");size_t count=0;for(;;){std::array<float,4>args;std::array<float,16>expected;f.read(reinterpret_cast<char*>(args.data()),16);if(f.gcount()==0&&f.eof())break;if(f.gcount()!=16)throw std::runtime_error("oracle args truncated");f.read(reinterpret_cast<char*>(expected.data()),64);if(f.gcount()!=64)throw std::runtime_error("oracle matrix truncated");auto actual=recoveredCameraProjectionCandidate(args[0],args[1],args[2],args[3],false);if(std::memcmp(expected.data(),actual.rows.data(),64))throw std::runtime_error("x64 native matrix mismatch case "+std::to_string(count));++count;}if(count!=60)throw std::runtime_error("oracle case count");std::cout<<"x64 vs original x86 projection passed cases="<<count<<" floats="<<count*16<<" bitwise_equal=1\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}


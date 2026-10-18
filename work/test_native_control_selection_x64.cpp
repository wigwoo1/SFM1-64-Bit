#include "ordered_transform_control_selection.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <array>
#include <stdexcept>
int main(int argc,char**argv){try{if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);unsigned cases=0;
for(unsigned count=0;count<=6;++count)for(unsigned pattern=0;pattern<(1u<<(count*2));++pattern){std::array<unsigned,6>states{};std::vector<uint32_t>channels;for(unsigned i=0;i<count;++i){states[i]=(pattern>>(i*2))&3;channels.push_back(states[i]?i:0xffffffffu);}uint32_t lookups=0,tests=0;
uint32_t chosen=selectRecoveredOrderedTransformControlCandidate(channels,[&](uint32_t channel){++lookups;return states[channel]==1?0xffffffffu:channel;},[&](uint32_t transform){++tests;return states[transform]==3;});std::array<uint32_t,3>expected{};f.read(reinterpret_cast<char*>(expected.data()),12);if(!f||expected!=std::array<uint32_t,3>{chosen,lookups,tests})throw std::runtime_error("native selection/short circuit mismatch");++cases;}
char extra;if(f.read(&extra,1))throw std::runtime_error("trailing native records");std::cout<<"native_control_selection_matches="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}

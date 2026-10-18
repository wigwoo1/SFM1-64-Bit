#include "software_flex_vertex_weight_candidate.h"
#include <fstream>
#include <iostream>
int main(){try{std::ofstream output("outputs/software-flex-weight-candidate.bin",std::ios::binary);if(!output)throw std::runtime_error("output missing");for(int s=0;s<256;++s)for(int t=0;t<256;++t){auto value=softwareFlexVertexWeightCandidate(.83f,.12f,.67f,.29f,uint8_t(s),uint8_t(t));output.write(reinterpret_cast<const char*>(&value),4);}if(!output)throw std::runtime_error("output failed");std::cout<<"software flex coefficient candidate wrote 65536 strict-float results\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

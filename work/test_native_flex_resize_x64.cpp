#include "flex_weight_resize.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
static uint32_t read(std::ifstream& in){uint32_t v=0;if(!in.read(reinterpret_cast<char*>(&v),4))throw std::runtime_error("short oracle");return v;}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("oracle required");std::ifstream in(argv[1],std::ios::binary);for(uint32_t old=0;old<32;++old)for(uint32_t n=0;n<32;++n){std::vector<uint32_t> v;for(uint32_t i=0;i<old;++i)v.push_back(0x3f000000u+i);auto append=read(in),remove=read(in);if(append!=(n>old?n-old:0)||remove!=(old>n?old-n:0))throw std::runtime_error("event counts");recoveredFlexWeightResizeCandidate(v,n);for(auto x:v)if(x!=read(in))throw std::runtime_error("payload mismatch");}if(in.peek()!=EOF)throw std::runtime_error("trailing oracle");std::cout<<"x64 flex resize: 1024 native comparisons passed\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

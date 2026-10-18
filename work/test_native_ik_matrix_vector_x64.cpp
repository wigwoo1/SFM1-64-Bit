#include "ik_matrix_vector.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdint>
int main(int argc,char**argv){
 static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)return 2;
 std::ifstream input(argv[1],std::ios::binary);if(!input)return 2;
 unsigned checks=0,mismatches=0;uint32_t pattern;
 while(input.read(reinterpret_cast<char*>(&pattern),4)){
  if(pattern>=4096)return 2;
  std::array<float,3>v{};RecoveredMatrix3x4 m{};
  for(unsigned i=0;i<3;++i)v[i]=float(int((pattern*17+i*13)%127)-63)*.03125f;
  for(unsigned i=0;i<12;++i)m[i/4][i%4]=float(int((pattern*23+i*7)%113)-56)*.0625f;
  for(unsigned mode=0;mode<4;++mode){std::array<float,3>expected{};
   input.read(reinterpret_cast<char*>(expected.data()),12);if(!input)return 2;
   const auto actual=(mode&1)?rotateRecoveredIkVectorCandidate(v,m):transformRecoveredIkPointCandidate(v,m);
   if(std::memcmp(actual.data(),expected.data(),12))++mismatches;++checks;
  }
 }
 std::cout<<"ik_matrix_vector_checks="<<checks<<" mismatches="<<mismatches<<std::endl;
 return checks==16384&&!mismatches?0:1;
}

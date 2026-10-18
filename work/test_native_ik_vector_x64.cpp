#include "ik_vector_normalize.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdint>
int main(int argc,char**argv){
 static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)return 2;
 std::ifstream input(argv[1],std::ios::binary);if(!input)return 2;
 unsigned checks=0,mismatches=0;uint32_t pattern;
 while(input.read(reinterpret_cast<char*>(&pattern),4)){
  std::array<float,3> expected{},q{};float expectedLength;
  input.read(reinterpret_cast<char*>(expected.data()),12);
  input.read(reinterpret_cast<char*>(&expectedLength),4);
  if(!input||pattern>=4096)return 2;
  for(unsigned i=0;i<3;++i)q[i]=float(int((pattern*17+i*13)%127)-63)*.03125f;
  if(pattern==0)q={0,0,0};if(pattern==1)q={0x1p-24f,0,0};
  const float length=normalizeRecoveredIkVectorCandidate(q);
  if(std::memcmp(q.data(),expected.data(),12)||std::memcmp(&length,&expectedLength,4))++mismatches;
  ++checks;
 }
 std::cout<<"ik_vector_checks="<<checks<<" mismatches="<<mismatches<<std::endl;
 return checks==4096&&!mismatches?0:1;
}

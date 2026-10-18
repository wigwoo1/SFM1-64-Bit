#include "ik_basis_quaternion.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdint>
int main(int argc,char**argv){
 static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)return 2;
 std::ifstream input(argv[1],std::ios::binary);if(!input)return 2;
 unsigned checks=0,mismatches=0;uint32_t pattern;
 while(input.read(reinterpret_cast<char*>(&pattern),4)){
  std::array<float,4> expected{};std::array<float,3>d{},g{};
  input.read(reinterpret_cast<char*>(expected.data()),16);if(!input||pattern>=4096)return 2;
  for(unsigned i=0;i<3;++i){d[i]=float(int((pattern*17+i*13)%127)-63)*.03125f;g[i]=float(int((pattern*23+i*7)%113)-56)*.0625f;}
  if(pattern<3){d={1,0,0};g={0,0,1};if(pattern==1)d={-1,0,0};if(pattern==2){d={-1,0,0};g={0,0,-1};}}
  const auto actual=recoveredIkBasisQuaternionCandidate(d,g);
  if(std::memcmp(actual.data(),expected.data(),16))++mismatches;
  ++checks;
 }
 std::cout<<"ik_basis_checks="<<checks<<" mismatches="<<mismatches<<std::endl;
 return checks==4096&&!mismatches?0:1;
}

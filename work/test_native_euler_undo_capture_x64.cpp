#include "float_components_undo_replay.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdint>
int main(int argc,char**argv){
 static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)return 2;
 std::ifstream input(argv[1],std::ios::binary);if(!input)return 2;
 unsigned checks=0,mismatches=0;uint32_t pattern;
 while(input.read(reinterpret_cast<char*>(&pattern),4)){
  std::array<float,4> expected{};std::array<float,3>q{};
  input.read(reinterpret_cast<char*>(expected.data()),16);
  if(!input||pattern>=4096)return 2;
  for(unsigned i=0;i<3;++i)q[i]=float(int((pattern*17+i*13)%721)-360)*.5f;
  RecoveredFloatComponentsUndoPayloadCandidate<4>record;record.owner=pattern;record.name=42;const std::array<float,4>incoming{1,2,3,4};captureRecoveredEulerQuaternionUndoCandidate(record,q,incoming);if(record.owner!=pattern||record.name!=42||record.after!=incoming)return 1;const auto actual=record.before;
  if(std::memcmp(actual.data(),expected.data(),16))++mismatches;
  ++checks;
 }
 std::cout<<"euler_undo_capture_checks="<<checks<<" mismatches="<<mismatches<<std::endl;
 return checks==4096&&!mismatches?0:1;
}

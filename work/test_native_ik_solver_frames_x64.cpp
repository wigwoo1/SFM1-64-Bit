#include "ik_prepared_solution.h"
#include "ik_coordinate_frame.h"
#include "ik_matrix_vector.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdint>
int main(int argc,char**argv){if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);if(!f)return 2;
 unsigned checks=0,mismatches=0;uint32_t p;
 while(f.read(reinterpret_cast<char*>(&p),4)){
  std::array<float,4>a{},b{};float weight;f.read(reinterpret_cast<char*>(a.data()),16);f.read(reinterpret_cast<char*>(b.data()),16);f.read(reinterpret_cast<char*>(&weight),4);if(!f||p>=1024)return 2;
  RecoveredMatrix3x4 frame{};const float scale=(p&1)?2.0f:.5f;frame[0][1]=-scale;frame[1][0]=scale;frame[2][2]=scale;frame[0][3]=float(int(p%9)-4)*.125f;frame[1][3]=.25f;frame[2][3]=-.5f;const auto inverse=recoveredIkCoordinateInverseCandidate(frame);
  auto pole=transformRecoveredIkPointCandidate({.25f,.5f,1},inverse);normalizeRecoveredIkVectorCandidate(pole);
  std::array<float,4>offset{},midOffset{};for(unsigned i=0;i<4;++i){offset[i]=float(int((p*17+i*13)%127)-63)*.03125f;midOffset[i]=float(int((p*23+i*7)%113)-56)*.0625f;}
  const auto s=recoveredIkPreparedSolutionCandidate({0,0,0},transformRecoveredIkPointCandidate({.25f+float(p%31)*.125f,float(int(p%17)-8)*.125f,.375f},inverse),pole,
   .5f+float(p%11)*.125f,.75f+float(p%7)*.125f,offset,midOffset);
  if(std::memcmp(a.data(),s.start.data(),16)||std::memcmp(b.data(),s.mid.data(),16)||weight!=.75f)++mismatches;++checks;
 }
 std::cout<<"ik_solver_checks="<<checks<<" mismatches="<<mismatches<<std::endl;return checks==1024&&!mismatches?0:1;
}

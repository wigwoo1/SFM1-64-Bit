#include "quaternion_interpolation.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdint>
int main(int argc,char**argv){if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);if(!f)return 2;
 unsigned checks=0,mismatches=0;uint32_t p,step;
 while(f.read(reinterpret_cast<char*>(&p),4)){f.read(reinterpret_cast<char*>(&step),4);if(!f||p>=32||step>300)return 2;
  for(unsigned joint=0;joint<2;++joint){std::array<float,4>expected{},q{},solved{};f.read(reinterpret_cast<char*>(expected.data()),16);if(!f)return 2;
   for(unsigned i=0;i<4;++i){q[i]=float(int((p*17+joint*19+i*13)%127)-63)*.03125f;solved[i]=float(int((p*23+joint*11+i*7)%113)-56)*.0625f;}
   const auto actual=interpolateRecoveredQuaternion(q,solved,float(int(step)-100)*.01f,false);
   if(std::memcmp(expected.data(),actual.data(),16))++mismatches;}
  ++checks;
 }
 std::cout<<"ik_eval_checks="<<checks<<" mismatches="<<mismatches<<std::endl;return checks==9632&&!mismatches?0:1;
}

#include "engine_projection_normalize_candidate.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
void check(bool value){if(!value)std::abort();}
int main(){
 float zero[3]={0,-0.0f,0};check(engineProjectionNormalizeCandidate(zero)==0);check(zero[0]==0&&std::signbit(zero[1]));
 float small[3]={0x1p-23f,0,0};check(engineProjectionNormalizeCandidate(small)==0x1p-23f);check(small[0]==0.5f);
 unsigned count=0;
 for(int i=-128;i<=128;++i)for(int j=-128;j<=128;++j){
  float vector[3]={float(i),float(j),3};float length=engineProjectionNormalizeCandidate(vector);
  double expected=std::sqrt(double(i)*i+double(j)*j+9);
  check(std::abs(double(length)-expected)<0.00003);
  double norm=double(vector[0])*vector[0]+double(vector[1])*vector[1]+double(vector[2])*vector[2];
  check(std::abs(norm-1)<0.000001);++count;
 }
 std::printf("PASS: %u vector lengths/unit-norm tolerances; zero signed-bit preservation and epsilon-sized vector produces0.5, not1. Native parity pending.\n",count);
}

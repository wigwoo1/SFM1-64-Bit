#include "engine_projection_math_stage_candidate.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
void check(bool ok){if(!ok)std::abort();}
int main(){unsigned cases=0;
 for(int x=1;x<=16;++x)for(int y=1;y<=16;++y)for(int z=1;z<=16;++z){
  float input[16]={float(x),0,0,-7,0,float(y),0,11,0,0,float(z),-3,0,0,0,1},shadow[16],scratch[16];EngineProjectionMathOutputCandidate output;
  check(engineProjectionMathStageCandidate(input,shadow,scratch,output));check(!std::memcmp(input,shadow,64));
  double scales[3]={1.0/x,1.0/y,1.0/z},expected[3]={7.5/x,-10.5/y,3.5/z};
  for(int k=0;k<3;++k)check(std::abs(output.center[k]-expected[k])<2e-6);
  check(std::abs(output.radius-.5*std::sqrt(scales[0]*scales[0]+scales[1]*scales[1]+scales[2]*scales[2]))<2e-6);
  for(int i=0;i<6;++i){auto& block=i<4?output.planes.first:output.planes.last;int lane=i<4?i:i-4;float distance=0;
   for(int k=0;k<3;++k){float normal;std::memcpy(&normal,&block.components[k][lane],4);distance+=normal*output.center[k];}
   float d;std::memcpy(&d,&block.components[3][lane],4);check(distance>d);
  }++cases;
 }
 float zero[16]{},shadow[16],scratch[16]={1,0,0,2,0,1,0,3,0,0,1,4,0,0,0,1},before[16];std::memcpy(before,scratch,64);
 EngineProjectionMathOutputCandidate output;std::memset(&output,0xa5,sizeof(output));
 check(!engineProjectionMathStageCandidate(zero,shadow,scratch,output));check(!std::memcmp(scratch,before,64));
 check(output.center[0]==2.5f&&output.center[1]==3.5f&&output.center[2]==4.5f);check(std::abs(output.radius-std::sqrt(.75f))<1e-6);
 float overlapping[17];for(int i=0;i<17;++i)overlapping[i]=float(i+1);
 std::memcpy(scratch,before,64);check(!engineProjectionMathStageCandidate(overlapping,overlapping+1,scratch,output));for(float v:overlapping)check(v==1);
 std::printf("PASS: %u integrated affine projections; six planes and sphere; singular scratch consumption and forward overlap. Full caller integration pending.\n",cases);
}

#include "engine_projection_generate_candidate.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
void check(bool v){if(!v)std::abort();}
float value(uint32_t b){float f;std::memcpy(&f,&b,4);return f;}
int main(){
 unsigned count=0;
 for(int t=-64;t<=64;++t){
  float matrix[16]={2,0,0,float(t),0,3,0,float(t+2),0,0,4,float(t-1),0,0,0,1};
  EngineProjectionPlanesCandidate output;engineProjectionGeneratePlanesCandidate(matrix,output);
  float center[3]={float(t)+1,float(t+2)+1.5f,float(t-1)+2};
  for(int i=0;i<6;++i){auto& block=i<4?output.first:output.last;int lane=i<4?i:i-4;
   float normal[3];for(int k=0;k<3;++k)normal[k]=value(block.components[k][lane]);float distance=value(block.components[3][lane]);
   check(normal[0]*center[0]+normal[1]*center[1]+normal[2]*center[2]-distance>0);
   int axis=i/2;float face[3]={center[0],center[1],center[2]};
   face[axis]=matrix[axis*4+3]+((i%2)?0:matrix[axis*4+axis]);
   if(i>=4)face[2]=matrix[11]+((i==5)?4:0);
   check(std::abs(normal[0]*face[0]+normal[1]*face[1]+normal[2]*face[2]-distance)<0.00003f);
  }++count;
 }
 float zeroW[16]={1,0,0,2,0,1,0,3,0,0,1,4,0,0,0,0},point[3]={1,2,3},result[3];
 engineProjectionPointCandidate(zeroW,point,result);check(result[0]==0&&result[1]==0&&result[2]==0);
 std::printf("PASS: %u translated/scaled volumes, six inward planes and face equations each; zero-W transform behavior. Native parity pending.\n",count);
}

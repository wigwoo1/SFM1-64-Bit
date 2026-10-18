#include "engine_projection_sphere_candidate.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
void check(bool ok){if(!ok)std::abort();}
int main(){unsigned cases=0;
 for(int x=1;x<=16;++x)for(int y=1;y<=16;++y)for(int z=1;z<=16;++z){
  float m[16]={float(x),0,0,-7,0,float(y),0,11,0,0,float(z),-3,0,0,0,1};float center[3],radius;
  engineProjectionSphereCandidate(m,center,radius);
  check(std::abs(center[0]-(-7+x*.5f))<1e-5f&&std::abs(center[1]-(11+y*.5f))<1e-5f&&std::abs(center[2]-(-3+z*.5f))<1e-5f);
  double expected=.5*std::sqrt(double(x*x+y*y+z*z));check(std::abs(radius-expected)<3e-6);
  for(int corner=0;corner<8;++corner){double d=0;for(int k=0;k<3;++k){double delta=m[k*4+3]+((corner>>k)&1)*m[k*4+k]-center[k];d+=delta*delta;}check(std::abs(std::sqrt(d)-radius)<3e-6);}++cases;
 }
 float collapsed[16]={1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1},center[3],radius;
 engineProjectionSphereCandidate(collapsed,center,radius);check(std::isnan(radius));for(float c:center)check(std::isnan(c));
 std::printf("PASS: %u affine boxes, centers/radii and eight corners each; collapsed axis preserves NaN. Native parity pending.\n",cases);
}

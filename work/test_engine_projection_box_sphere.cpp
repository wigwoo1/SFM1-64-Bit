#include "engine_projection_box_sphere_candidate.h"
#include <cmath>
#include <initializer_list>
#include <cstdlib>
#include <cstdio>
void check(bool v){if(!v)std::abort();}
int main(){float minimum[3]={-1,-1,-1},maximum[3]={1,1,1};unsigned count=0;
 for(int x=-8;x<=8;++x)for(int y=-8;y<=8;++y)for(int z=-8;z<=8;++z){float center[3]={float(x),float(y),float(z)};int squared=0;for(int value:{x,y,z}){int d=std::abs(value)>1?std::abs(value)-1:0;squared+=d*d;}
  for(int radius=-8;radius<=8;++radius){check(engineProjectionBoxSphereCandidate(minimum,maximum,center,float(radius))==(radius*radius>squared));++count;}}
 float tangent[3]={2,0,0};check(!engineProjectionBoxSphereCandidate(minimum,maximum,tangent,1));check(engineProjectionBoxSphereCandidate(minimum,maximum,tangent,std::nextafter(1.0f,2.0f)));
 std::printf("PASS: %u independently computed integer-distance cases, signed/zero radii, tangency rejection and nextafter acceptance. Native parity pending.\n",count);
}


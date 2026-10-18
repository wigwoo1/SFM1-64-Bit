#include "source_displacement_ray_triangle_candidate.h"
#include <iostream>
#include <limits>
int main(){try{
 const std::array<std::array<float,3>,3>p={{{0,0,0},{1,0,0},{0,1,0}}};
 auto check=[&](std::array<float,3>s,std::array<float,3>d,bool one,float expected){if(sourceDisplacementRayTriangleCandidate(s,d,p,one)!=expected)throw std::runtime_error("analytical ray case");};
 check({.25f,.25f,1},{0,0,-2},true,.5f);check({.25f,.25f,-1},{0,0,2},true,-1);check({.25f,.25f,-1},{0,0,2},false,.5f);
 check({1.1f,1.1f,1},{0,0,-2},false,-1);check({0,0,1},{0,0,-2},true,.5f);check({.25f,.25f,1},{1,0,0},false,-1);
 check({.25f,.25f,-.0005f},{0,0,-1},false,0);check({.25f,.25f,1.0005f},{0,0,-1},false,1);check({.25f,.25f,1.002f},{0,0,-1},false,-1);
 unsigned rejected=0;for(unsigned mode=0;mode<3;++mode){auto points=p;std::array<float,3>s{0,0,1},d{0,0,-2};if(mode==0)points[0][0]=std::numeric_limits<float>::quiet_NaN();if(mode==1)s[0]=std::numeric_limits<float>::infinity();if(mode==2)d[0]=std::numeric_limits<float>::quiet_NaN();try{sourceDisplacementRayTriangleCandidate(s,d,points,false);}catch(const std::runtime_error&){++rejected;}}
 if(rejected!=3)throw std::runtime_error("ray guard coverage");std::cout<<"PASS nine analytical hits/misses/clamps and three finite guards\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

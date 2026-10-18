#pragma once
#include <array>
#include <cmath>
#include <stdexcept>

// Original 1014DD40 and ray-only branch of tolerance helper 1014DCA0.
// Takes a start + delta segment (not an endpoint), returns fraction or -1.
// Original caller may swap triangle vertices; this primitive preserves its
// supplied order. Hull extents and world traversal are separate components.
inline float sourceDisplacementRayTriangleCandidate(const std::array<float,3>&start,
 const std::array<float,3>&delta,const std::array<std::array<float,3>,3>&points,bool oneSided){
 for(float v:start)if(!std::isfinite(v))throw std::runtime_error("nonfinite triangle ray start");
 for(float v:delta)if(!std::isfinite(v))throw std::runtime_error("nonfinite triangle ray delta");
 for(const auto&p:points)for(float v:p)if(!std::isfinite(v))throw std::runtime_error("nonfinite ray triangle vertex");
 std::array<float,3>e1{},e2{},t{};for(size_t a=0;a<3;++a){e1[a]=points[1][a]-points[0][a];e2[a]=points[2][a]-points[0][a];t[a]=start[a]-points[0][a];}
 auto subtractProducts=[](float a,float b,float c,float d){const float left=a*b,right=c*d;return left-right;};
 if(oneSided){
  const float nx=subtractProducts(e2[2],e1[1],e2[1],e1[2]);
  const float ny=subtractProducts(e1[2],e2[0],e2[2],e1[0]);
  const float nz=subtractProducts(e2[1],e1[0],e2[0],e1[1]);
  const float x=nx*delta[0],y=delta[1]*ny,z=delta[2]*nz;const float xy=y+x;const float dot=xy+z;
  if(dot>=0.f)return -1.f;
 }
 const float px=subtractProducts(delta[1],e2[2],delta[2],e2[1]);
 const float py=subtractProducts(delta[2],e2[0],delta[0],e2[2]);
 const float pz=subtractProducts(delta[0],e2[1],delta[1],e2[0]);
 const float dy=py*e1[1],dx=px*e1[0],dz=pz*e1[2];const float dxy=dy+dx,determinant=dxy+dz;
 if(double(std::fabs(determinant))<1.e-6)return -1.f;
 const float inverse=1.f/determinant;
 const float uy=t[1]*py,ux=t[0]*px,uz=t[2]*pz;const float uxy=uy+ux,usum=uxy+uz,u=usum*inverse;
 if(u<0.f||u>1.f)return -1.f;
 const float qx=subtractProducts(t[1],e1[2],t[2],e1[1]);
 const float qy=subtractProducts(t[2],e1[0],t[0],e1[2]);
 const float qz=subtractProducts(t[0],e1[1],t[1],e1[0]);
 const float vx=delta[0]*qx,vy=qy*delta[1],vz=qz*delta[2];const float vxy=vx+vy,vsum=vxy+vz,v=vsum*inverse;
 if(v<0.f)return -1.f;const float uv=v+u;if(uv>1.f)return -1.f;
 const float ty=qy*e2[1],tx=qx*e2[0],tz=qz*e2[2];const float txy=ty+tx,tsum=txy+tz,fraction=tsum*inverse;
 if(!std::isfinite(fraction))throw std::runtime_error("nonfinite triangle ray intermediate");
 constexpr float tolerance=.001f;const float upper=1.f+tolerance;
 if(fraction<-tolerance||fraction>upper)return -1.f;
 if(fraction>1.f)return 1.f;if(fraction<0.f)return 0.f;return fraction;
}

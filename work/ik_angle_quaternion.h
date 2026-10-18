#pragma once
#include <array>
#include <cmath>
// Native 1049A3D0 uses x87 FSINCOS. Host double trig remains a candidate.
inline std::array<float,4> recoveredIkAngleQuaternionCandidate(const std::array<float,3>&angles){
 std::array<float,3>s{},c{};
 for(unsigned i=0;i<3;++i){const float radians=angles[i]*0x1.1df46ap-6f;
  const float half=radians*.5f;s[i]=float(std::sin(double(half)));c[i]=float(std::cos(double(half)));}
 const float a=s[2]*c[0],b=c[2]*s[0],d=c[2]*c[0],e=s[2]*s[0];
 return {a*c[1]-b*s[1],a*s[1]+b*c[1],d*s[1]-e*c[1],d*c[1]+e*s[1]};
}

#pragma once
#include <array>
inline std::array<float,4> multiplyRecoveredAlignedQuaternions(
    const std::array<float,4>&a,std::array<float,4>b){
    float difference=0,sum=0;
    for(unsigned c=0;c<4;++c){const float d=a[c]-b[c],s=a[c]+b[c];difference=difference+d*d;sum=sum+s*s;}
    if(difference>sum)for(auto&c:b)c=-c;
    const float x0=a[0]*b[3],x1=b[2]*a[1],x2=b[1]*a[2],x3=a[3]*b[0];
    const float y0=b[3]*a[1],y1=a[0]*b[2],y2=b[0]*a[2],y3=a[3]*b[1];
    const float z0=a[0]*b[1],z1=b[0]*a[1],z2=b[3]*a[2],z3=a[3]*b[2];
    const float w0=a[3]*b[3],w1=a[0]*b[0],w2=b[1]*a[1],w3=b[2]*a[2];
    return {((x0+x1)-x2)+x3,((y0-y1)+y2)+y3,((z0-z1)+z2)+z3,w0-((w1+w2)+w3)};
}

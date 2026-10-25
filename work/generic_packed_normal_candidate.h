#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
// Explicit CPU convention for shader DP3/NRM: left-associated binary32 dot,
// correctly rounded float sqrt, reciprocal, then multiply. GPU NRM precision
// and dot reduction order are not asserted by this reference implementation.
inline std::array<float,3>genericNormalizeReferenceCandidate(const std::array<float,3>&n){
    for(float v:n)if(!std::isfinite(v))throw std::invalid_argument("nonfinite reference normal");
    const float xx=n[0]*n[0],yy=n[1]*n[1],zz=n[2]*n[2];const float pair=xx+yy,length=pair+zz;
    if(!std::isfinite(length)||length<=0)throw std::invalid_argument("degenerate reference normal");
    const float root=std::sqrt(length),reciprocal=1.0f/root;
    return {n[0]*reciprocal,n[1]*reciprocal,n[2]*reciprocal};
}
// Original studiorender1000A100, standard nonzero finite XYZ domain only.
// This is the low normal half of the packed normal/tangent DWORD.
inline std::array<uint8_t,4>genericPackNormalCandidate(const std::array<float,3>&n){
    for(float v:n)if(!std::isfinite(v)||std::abs(v)>1.05f)throw std::invalid_argument("packed normal input domain");
    const float sx=n[0]<0?-1.0f:1.0f,sy=n[1]<0?-1.0f:1.0f,sz=n[2]<0?-1.0f:1.0f;
    const float ax=sx*n[0],ay=sy*n[1],az=sz*n[2];const float pair=ay+ax,sum=pair+az,reciprocal=1.0f/sum;
    if(!(sum>0)||!std::isfinite(reciprocal))throw std::invalid_argument("degenerate packed normal");
    float x=reciprocal*ax;x=x*63.0f;x=x*sx;const float xb=(1.0f-sx)*0.5f;x=x-xb;x=x+64.0f;x=x*sz;const float zb=(1.0f-sz)*0.5f;x=x-zb;x=x+128.0f;
    float y=reciprocal*ay;y=y*63.0f;y=y*sy;const float yb=(1.0f-sy)*0.5f;y=y-yb;y=y+64.0f;y=y+128.0f;
    if(!std::isfinite(x)||!std::isfinite(y)||x<0||x>=256||y<0||y>=256)throw std::invalid_argument("packed normal conversion domain");
    return {uint8_t(int(x)&255),uint8_t(int(y)&255),0,0};
}
// Installed compressed VS20 static0/dynamic5,7 low-two-byte decode.
inline std::array<float,3>genericDecodeNormalCandidate(const std::array<uint8_t,2>&packed){
    std::array<float,2>distance{},sign{};const float zSign=packed[0]<128?-1.0f:1.0f;
    for(unsigned i=0;i<2;++i){const float centered=float(packed[i])-128.0f;const float negative=centered<0?1.0f:0.0f;const float absolute=std::abs(centered);const float folded=absolute-negative;const float shifted=folded-64.0f;
        const float negativeAxis=shifted<0?1.0f:0.0f;distance[i]=std::abs(shifted)-negativeAxis;sign[i]=negativeAxis*(-2.0f)+1.0f;}
    constexpr float scale=1.0f/63.0f;const float z0=distance[0]*(-scale),z1=z0+1.0f,z2=distance[1]*(-scale),z=z2+z1;
    const auto normalized=genericNormalizeReferenceCandidate({distance[0]*scale,distance[1]*scale,z});
    return {sign[0]*normalized[0],sign[1]*normalized[1],zSign*normalized[2]};
}

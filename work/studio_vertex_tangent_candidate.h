#pragma once
#include "generic_packed_normal_candidate.h"
#include <cstring>
struct StudioVertexTangentCandidate {
    std::array<float,4> tangent{};
    std::array<uint8_t,4> packedFrame{};
    bool repaired=false;
};
// Installed full vertex writers' OPTIONAL-TANGENT-PRESENT finite domain.
// A cross-product length below 0.1 selects an axis using abs(normal.x)>0.7.
// The replacement is normalized with the native float epsilon, not a generic
// unit-vector routine. The original tangent sign is retained. No model/world
// transform happens here; this is a vertex-buffer input handoff.
inline StudioVertexTangentCandidate studioVertexTangentCandidate(
    const std::array<float,3>&normal,const std::array<float,4>&input,bool compressed){
    // Also establishes a meaningful, nonzero normal for either writer mode.
    const auto packedNormal=genericPackNormalCandidate(normal);
    for(unsigned i=0;i<3;++i)if(!std::isfinite(input[i])||std::abs(input[i])>10000.0f)throw std::invalid_argument("tangent input domain");
    if(input[3]!=1.0f&&input[3]!=-1.0f)throw std::invalid_argument("tangent handedness domain");
    StudioVertexTangentCandidate out;out.tangent=input;
    const float x0=normal[1]*input[2],x1=normal[2]*input[1],cx=x0-x1;
    const float y0=normal[2]*input[0],y1=normal[0]*input[2],cy=y0-y1;
    const float z0=normal[0]*input[1],z1=normal[1]*input[0],cz=z0-z1;
    const float xx=cx*cx,yy=cy*cy,zz=cz*cz,pair=xx+yy,length=std::sqrt(pair+zz);
    if(length<0.1f){
        const float zero=0.0f;
        if(std::abs(normal[0])>0.7f){
            const float yz=normal[1]*zero,nzz=normal[2]*zero,nxz=normal[0]*zero;
            out.tangent[0]=yz-normal[2];out.tangent[1]=nzz-nxz;out.tangent[2]=normal[0]-yz;
        }else{
            const float yz=normal[1]*zero,nzz=normal[2]*zero,nxz=normal[0]*zero;
            out.tangent[0]=yz-nzz;out.tangent[1]=normal[2]-nxz;out.tangent[2]=nxz-normal[1];
        }
        const float ax=out.tangent[0]*out.tangent[0],ay=out.tangent[1]*out.tangent[1],az=out.tangent[2]*out.tangent[2];
        const float xy=ax+ay,root=std::sqrt(xy+az),denominator=root+0x1p-23f,reciprocal=1.0f/denominator;
        for(unsigned i=0;i<3;++i)out.tangent[i]=out.tangent[i]*reciprocal;
        out.repaired=true;
    }
    if(!compressed)return out;
    const auto&t=out.tangent;const float sx=t[0]<0?-1.0f:1.0f,sy=t[1]<0?-1.0f:1.0f,sz=t[2]<0?-1.0f:1.0f;
    const float ax=sx*t[0],ay=sy*t[1],az=sz*t[2],xy=ay+ax,sum=xy+az,reciprocal=1.0f/sum;
    if(!(sum>0)||!std::isfinite(reciprocal))throw std::invalid_argument("packed tangent domain");
    float x=ax*reciprocal;x=x*63.0f;x=x*sx;const float xb=(1.0f-sx)*0.5f;x=x-xb;x=x+64.0f;x=x*sz;const float zb=(1.0f-sz)*0.5f;x=x-zb;x=x+128.0f;
    float y=ay*reciprocal;y=y*63.0f;y=y*sy;const float yb=(1.0f-sy)*0.5f;y=y-yb;y=y+64.0f;y=y*t[3];const float hb=(1.0f-t[3])*0.5f;y=y-hb;y=y+128.0f;
    if(!std::isfinite(x)||!std::isfinite(y)||x<0||x>=256||y<0||y>=256)throw std::invalid_argument("packed tangent conversion domain");
    out.packedFrame={packedNormal[0],packedNormal[1],uint8_t(int(x)&255),uint8_t(int(y)&255)};return out;
}

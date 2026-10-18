#pragma once
#include "source_physics_line_simplex_candidate.h"

// Complete finite-result scalar policy of original10027710. Reads only old
// vertices0/1 and incoming; retains opaque unused vertices and feature words.
// This reduction chooses a point, an edge, or an oriented triangle. Even a
// zero direction returns false: this routine alone does not prove a hit.
inline bool sourcePhysicsTriangleSimplexCandidate(SourcePhysicsSimplexCandidate&simplex,
    SourcePhysicsSimplexVertexCandidate incoming,std::array<float,3>&direction){
    const auto a=simplex.vertices[0].point,b=simplex.vertices[1].point,p=incoming.point;
    auto finite=[](float value){if(!std::isfinite(value))throw std::runtime_error("nonfinite triangle simplex intermediate");return value;};
    for(const auto&point:{a,b,p})for(float value:point)finite(value);
    const float ax=finite(a[0]-p[0]),ay=finite(a[1]-p[1]),az=finite(a[2]-p[2]);
    const float bx=finite(b[0]-p[0]),by=finite(b[1]-p[1]),bz=finite(b[2]-p[2]);
    // Keep the original scalar operation order, including Y+X before Z in
    // projections. Generic vector rewrites can alter rounding and signed zero.
    const float nx=finite(bz*ay-az*by),ny=finite(az*bx-bz*ax),nz=finite(ax*by-ay*bx);
    const float ex=finite(bz*ny-nz*by),ey=finite(nz*bx-bz*nx),ez=finite(nx*by-bx*ny);
    const float sideB=finite((p[1]*ey+p[0]*ex)+p[2]*ez);
    auto result=simplex;std::array<float,3>next{};bool reduced=false;
    if(sideB<0){
        const float dotB=finite((p[1]*by+p[0]*bx)+p[2]*bz);
        if(dotB<0){
            result.count=2;result.vertices[0]=incoming;
            const float t0=finite(p[2]*by-p[1]*bz),t1=finite(p[0]*bz-p[2]*bx),t2=finite(p[1]*bx-p[0]*by);
            next={t1*bz-t2*by,t2*bx-bz*t0,t0*by-t1*bx};reduced=true;
        }
    }else{
        const float fx=finite(nz*ay-az*ny),fy=finite(az*nx-nz*ax),fz=finite(ax*ny-ay*nx);
        const float sideA=finite((p[1]*fy+p[0]*fx)+p[2]*fz);
        if(sideA>0){
            result.count=3;const float plane=finite((p[1]*ny+p[0]*nx)+p[2]*nz);
            if(plane<0){result.vertices[2]=incoming;next={-nx,-ny,-nz};}
            else{result.vertices[2]=simplex.vertices[1];result.vertices[1]=incoming;next={nx,ny,nz};}
            reduced=true;
        }
    }
    if(!reduced){
        const float dotA=finite((p[1]*ay+p[0]*ax)+p[2]*az);
        if(dotA<0){
            result.count=2;result.vertices[1]=incoming;
            const float t0=finite(p[2]*ay-p[1]*az),t1=finite(p[1]*ax-p[0]*ay),t2=finite(p[0]*az-p[2]*ax);
            next={az*t2-t1*ay,t1*ax-t0*az,t0*ay-ax*t2};
        }else{result.count=1;result.vertices[0]=incoming;next=p;}
    }
    for(float value:next)finite(value);
    // Invalid/overflowing inputs leave both caller-owned outputs untouched.
    simplex=result;direction=next;return false;
}

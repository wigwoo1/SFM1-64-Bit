#pragma once
#include "source_physics_triangle_simplex_candidate.h"

// Complete finite-result scalar reduction of original10027C00. Old count,
// vertex3 and input direction have no geometric meaning to this routine.
// The enclosing case retains the input direction verbatim and returns true.
// Nonfinite/overflow failure preserves both caller-owned outputs.
inline bool sourcePhysicsTetrahedronSimplexCandidate(SourcePhysicsSimplexCandidate&simplex,
    SourcePhysicsSimplexVertexCandidate incoming,std::array<float,3>&direction){
    using V=std::array<float,3>;
    auto finite=[](float x){if(!std::isfinite(x))throw std::runtime_error("nonfinite tetrahedron simplex intermediate");return x;};
    const V p=incoming.point;for(float x:p)finite(x);
    auto offset=[&](const V&v){for(float x:v)finite(x);return V{finite(v[0]-p[0]),finite(v[1]-p[1]),finite(v[2]-p[2])};};
    const V a=offset(simplex.vertices[0].point),b=offset(simplex.vertices[1].point),c=offset(simplex.vertices[2].point);
    // Cross product component signs match the original instructions; never
    // replace subtraction with negated reverse subtraction (signed zero).
    auto cross=[&](const V&u,const V&v){return V{finite(u[1]*v[2]-u[2]*v[1]),finite(u[2]*v[0]-u[0]*v[2]),finite(u[0]*v[1]-u[1]*v[0])};};
    auto dot=[&](const V&v){return finite((p[1]*v[1]+p[0]*v[0])+p[2]*v[2]);};
    const V ab=cross(a,b),bc=cross(b,c),ca=cross(c,a);
    // Original precomputes all six side vectors before branch dispatch.
    const V abB=cross(ab,b),abA=cross(a,ab),bcC=cross(bc,c),bcB=cross(b,bc),caA=cross(ca,a),caC=cross(c,ca);
    const unsigned mask=(dot(ab)<0?1u:0u)|(dot(bc)<0?2u:0u)|(dot(ca)<0?4u:0u);
    auto result=simplex;V next=direction;bool complete=false;
    auto point=[&](){result.count=1;result.vertices[0]=incoming;next=p;};
    auto edge=[&](unsigned index){
        const V d=index==0?a:(index==1?b:c);
        result.count=2;
        if(index==1)result.vertices[0]=incoming;
        else{if(index==2)result.vertices[0]=simplex.vertices[2];result.vertices[1]=incoming;}
        const float t0=finite(p[2]*d[1]-p[1]*d[2]),t1=finite(p[0]*d[2]-p[2]*d[0]),t2=finite(p[1]*d[0]-p[0]*d[1]);
        next={finite(t1*d[2]-t2*d[1]),finite(t2*d[0]-t0*d[2]),finite(t0*d[1]-t1*d[0])};
    };
    auto face=[&](unsigned normal){result.count=3;const V n=normal==0?ab:(normal==1?bc:ca);result.vertices[normal==0?2:(normal==1?0:1)]=incoming;next={-n[0],-n[1],-n[2]};};
    switch(mask){
    case 0:result.count=4;result.vertices[3]=incoming;complete=true;break;
    case 1:complete=sourcePhysicsTriangleSimplexCandidate(result,incoming,next);break;
    case 2:result.vertices[0]=simplex.vertices[2];complete=sourcePhysicsTriangleSimplexCandidate(result,incoming,next);break;
    case 4:result.vertices[1]=simplex.vertices[2];complete=sourcePhysicsTriangleSimplexCandidate(result,incoming,next);break;
    case 3:
        if(dot(abB)<0){
            if(dot(bcB)<0){if(dot(b)<0)edge(1);else point();}
            else if(dot(bcC)<0)edge(2);else face(1);
        }else if(dot(abA)<0){if(dot(a)<0)edge(0);else point();}
        else face(0);
        break;
    case 5:
        if(dot(caA)<0){
            if(dot(abA)<0){if(dot(a)<0)edge(0);else point();}
            else if(dot(abB)<0)edge(1);else face(0);
        }else if(dot(caC)<0){if(dot(c)<0)edge(2);else point();}
        else face(2);
        break;
    case 6:
        if(dot(bcC)<0){
            if(dot(caC)<0){if(dot(c)<0)edge(2);else point();}
            else if(dot(caA)<0)edge(0);else face(2);
        }else if(dot(bcB)<0){if(dot(b)<0)edge(1);else point();}
        else face(1);
        break;
    case 7:
        if(dot(a)<0)edge(0);else if(dot(b)<0)edge(1);else if(dot(c)<0)edge(2);else point();
        break;
    }
    simplex=result;direction=next;return complete;
}

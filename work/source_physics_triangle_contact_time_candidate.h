#pragma once
#include "source_physics_line_simplex_candidate.h"

// Original A7F30 uses single-precision length and an additive FLT_EPSILON,
// including for a zero vector. It is not normalization with a length clamp.
inline std::array<float,3> sourcePhysicsContactNormalizeCandidate(std::array<float,3>v){
    auto finite=[](float x){if(!std::isfinite(x))throw std::runtime_error("nonfinite contact normalization");return x;};
    for(float x:v)finite(x);
    const float length=finite(std::sqrt(finite((v[0]*v[0]+v[1]*v[1])+v[2]*v[2])));
    const float scale=finite(1.f/(length+0x1p-23f));
    for(float&x:v)x=finite(x*scale);return v;
}

// Complete original 25F50 triangle contact-time helper. All three points are
// used regardless of the simplex count; metadata and unused vertex3 ignored.
// This is a dependency of the iterative search, not a complete trace.
inline float sourcePhysicsTriangleContactTimeCandidate(const SourcePhysicsSimplexCandidate&simplex,
    const std::array<float,3>&direction,float tolerance){
    using V=std::array<float,3>;
    auto finite=[](float x){if(!std::isfinite(x))throw std::runtime_error("nonfinite triangle contact arithmetic");return x;};
    for(float x:direction)finite(x);finite(tolerance);
    const V a=simplex.vertices[0].point,b=simplex.vertices[1].point,c=simplex.vertices[2].point;
    auto subtract=[&](const V&u,const V&v){V r;for(unsigned j=0;j<3;++j)r[j]=finite(finite(u[j])-finite(v[j]));return r;};
    auto cross=[&](const V&u,const V&v){return V{finite(u[1]*v[2]-u[2]*v[1]),finite(u[2]*v[0]-u[0]*v[2]),finite(u[0]*v[1]-u[1]*v[0])};};
    auto dot=[&](const V&u,const V&v){return finite((u[1]*v[1]+u[0]*v[0])+u[2]*v[2]);};
    const V u=subtract(a,c),v=subtract(b,c),w=subtract(b,a);
    const V normal=sourcePhysicsContactNormalizeCandidate(cross(u,v));
    const std::array<V,3>sides{{sourcePhysicsContactNormalizeCandidate(cross(u,normal)),sourcePhysicsContactNormalizeCandidate(cross(normal,v)),sourcePhysicsContactNormalizeCandidate(cross(w,normal))}};
    constexpr float sentinel=1.0000000138484279e24f;
    std::array<float,3>times{};
    for(unsigned side=0;side<3;++side){const float denominator=dot(direction,sides[side]);times[side]=denominator>0?finite(dot(side==0?a:b,sides[side])/denominator):sentinel;}
    unsigned chosen=0;if(times[0]>times[1])chosen=1;if(times[chosen]>times[2])chosen=2;
    const float denominator=dot(direction,sides[chosen]);if(!(denominator>0))return sentinel;
    // The original final divide/add stays in x87 registers until its caller
    // stores the float. Default x87 precision is 53 bits in the test runtime.
    return finite(float(double(tolerance)/double(denominator)+double(times[chosen])));
}

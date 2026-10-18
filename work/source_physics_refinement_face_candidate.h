#pragma once
#include "source_physics_line_simplex_candidate.h"

// Original25460 (initial four-face choice) and25B10 (three-face update).
// Input count is ignored; all four point records must be present. The returned
// normal is not normalized. The caller normalizes before the next support call.
inline std::array<float,3> sourcePhysicsRefinementFaceCandidate(
    SourcePhysicsSimplexCandidate&simplex,const std::array<float,3>&direction,bool includeBase){
    using V=std::array<float,3>;
    auto finite=[](float x){if(!std::isfinite(x))throw std::runtime_error("nonfinite refinement-face arithmetic");return x;};
    for(float x:direction)finite(x);for(const auto&vertex:simplex.vertices)for(float x:vertex.point)finite(x);
    const auto&a=simplex.vertices[0].point;const auto&b=simplex.vertices[1].point;const auto&c=simplex.vertices[2].point;const auto&d=simplex.vertices[3].point;
    auto subtract=[&](const V&u,const V&v){return V{finite(u[0]-v[0]),finite(u[1]-v[1]),finite(u[2]-v[2])};};
    auto cross=[&](const V&u,const V&v){return V{finite(u[1]*v[2]-u[2]*v[1]),finite(u[2]*v[0]-u[0]*v[2]),finite(u[0]*v[1]-u[1]*v[0])};};
    auto dot=[&](const V&u,const V&v){return finite((u[0]*v[0]+u[1]*v[1])+u[2]*v[2]);};
    const V u=subtract(a,d),v=subtract(b,d),w=subtract(c,d);
    std::array<V,4>normals{{cross(u,v),cross(v,w),cross(w,u),{}}};
    if(includeBase)normals[3]=cross(subtract(c,a),subtract(b,a));
    constexpr float sentinel=1.0000000138484279e24f;
    std::array<float,4>times{};
    for(unsigned face=0;face<(includeBase?4u:3u);++face){const float denominator=dot(direction,normals[face]);times[face]=denominator>0?finite(dot(face==3?a:d,normals[face])/denominator):sentinel;}
    unsigned chosen=includeBase?3u:2u;
    if(times[chosen]>times[0])chosen=0;if(times[chosen]>times[1])chosen=1;if(includeBase&&times[chosen]>times[2])chosen=2;
    auto result=simplex;result.count=3;
    if(chosen==0)result.vertices[2]=simplex.vertices[3];
    else if(chosen==1)result.vertices[0]=simplex.vertices[3];
    else if(chosen==2)result.vertices[1]=simplex.vertices[3];
    else{result.vertices[3]=simplex.vertices[1];result.vertices[1]=simplex.vertices[2];result.vertices[2]=simplex.vertices[1];}
    simplex=result;return normals[chosen];
}

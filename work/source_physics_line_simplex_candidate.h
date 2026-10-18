#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
struct SourcePhysicsSimplexVertexCandidate {
    std::array<float,3>point{};
    uint16_t featureA=0,featureB=0;
};
struct SourcePhysicsSimplexCandidate {
    std::array<SourcePhysicsSimplexVertexCandidate,4>vertices{};
    uint32_t count=0;
};
// Complete finite-result policy of original10027580: old vertex0 and a new
// point form a one/two-point simplex. Input count and unused vertices are not
// read; packed feature words are copied in full, including their high bits.
// Return is always false (neither branch proves intersection by itself).
inline bool sourcePhysicsLineSimplexCandidate(SourcePhysicsSimplexCandidate&simplex,
    SourcePhysicsSimplexVertexCandidate incoming,std::array<float,3>&direction){
    const auto&old=simplex.vertices[0].point;const auto&p=incoming.point;
    for(float x:old)if(!std::isfinite(x))throw std::runtime_error("nonfinite simplex existing point");
    for(float x:p)if(!std::isfinite(x))throw std::runtime_error("nonfinite simplex incoming point");
    const float dx=old[0]-p[0],dy=old[1]-p[1],dz=old[2]-p[2];
    const float y=p[1]*dy,x=p[0]*dx,z=p[2]*dz;const float dot=(y+x)+z;
    if(!std::isfinite(dx)||!std::isfinite(dy)||!std::isfinite(dz)||!std::isfinite(dot))throw std::runtime_error("simplex projection overflow");
    auto result=simplex;std::array<float,3>next;
    if(dot<0){
        result.count=2;result.vertices[1]=incoming;
        // Preserve original subtraction/product signs and order; a generic
        // cross-product rewrite can change signed-zero and rounding outcomes.
        const float a=p[2]*dy-p[1]*dz,b=p[1]*dx-p[0]*dy,c=p[0]*dz-p[2]*dx;
        next={dz*c-b*dy,dx*b-a*dz,a*dy-dx*c};
    }else{result.count=1;result.vertices[0]=incoming;next=p;}
    for(float value:next)if(!std::isfinite(value))throw std::runtime_error("simplex search direction overflow");simplex=result;direction=next;return false;
}

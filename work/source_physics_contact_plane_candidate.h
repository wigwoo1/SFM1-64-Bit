#pragma once
#include "source_physics_refinement_candidate.h"
#include <algorithm>
struct SourcePhysicsContactPlaneCandidate {std::array<float,3>normal{};float distance=0;};
// Original272D0 ignores the point-support fast flag and decodes stored bounds.
inline std::array<float,3> sourcePhysicsBoxVertexCandidate(const SourcePhysicsBoxSupportCandidate&box,uint32_t feature){
    std::array<float,3>p;for(unsigned axis=0;axis<3;++axis){p[axis]=box.axes[axis][(feature>>axis)&1];if(!std::isfinite(p[axis]))throw std::runtime_error("nonfinite box vertex");}return p;
}
// Original271E0: packed vertices are already transformed; raw/span vertices
// use global feature indices and original A8190's Y+X then Z transform.
inline std::array<float,3> sourcePhysicsShapeVertexCandidate(const SourcePhysicsSupportContextCandidate&shape,uint32_t feature){
    auto finite=[](float x){if(!std::isfinite(x))throw std::runtime_error("nonfinite contact vertex");return x;};
    if(shape.mode>2)throw std::runtime_error("invalid contact vertex mode");size_t index=feature;if(shape.mode==1){if(feature<shape.firstVertex)throw std::runtime_error("contact span index before first");index-=shape.firstVertex;}if(index>=shape.points.size())throw std::runtime_error("contact feature outside shape");const auto&p=shape.points[index];for(float x:p)finite(x);if(shape.mode==2)return p;
    std::array<float,3>result;for(unsigned row=0;row<3;++row){const auto*m=shape.transform.data()+row*4;for(unsigned i=0;i<4;++i)finite(m[i]);result[row]=finite(((m[1]*p[1]+m[0]*p[0])+m[2]*p[2])+m[3]);}return result;
}
// Complete original24ED0 policy. Only feature records/count are read from the
// simplex. Existing trace endpoint is needed only by A-side contact branches.
// Returning a value allows callers to commit the16 plane bytes after success.
inline SourcePhysicsContactPlaneCandidate sourcePhysicsContactPlaneCandidate(
    const SourcePhysicsSimplexCandidate&simplex,const std::array<float,16>&ray,
    const SourcePhysicsBoxSupportCandidate&box,const SourcePhysicsSupportContextCandidate&shape,
    const std::array<float,3>&traceEnd){
    using V=std::array<float,3>;const int32_t count=int32_t(simplex.count);if(count>4)throw std::runtime_error("contact simplex extent");
    auto finite=[](float x){if(!std::isfinite(x))throw std::runtime_error("nonfinite contact-plane arithmetic");return x;};
    auto subtract=[&](const V&a,const V&b){V r;for(unsigned j=0;j<3;++j)r[j]=finite(finite(a[j])-finite(b[j]));return r;};
    auto cross=[&](const V&a,const V&b){return V{finite(a[1]*b[2]-a[2]*b[1]),finite(a[2]*b[0]-a[0]*b[2]),finite(a[0]*b[1]-a[1]*b[0])};};
    auto dot=[&](const V&a,const V&b){for(float x:a)finite(x);for(float x:b)finite(x);return finite((a[1]*b[1]+a[0]*b[0])+a[2]*b[2]);};
    std::vector<uint16_t>a{uint16_t(simplex.vertices[0].featureA&0x7fff)},b{simplex.vertices[0].featureB};
    for(int32_t j=1;j<count;++j){const auto&vertex=simplex.vertices[size_t(j)];const uint16_t fa=vertex.featureA&0x7fff,fb=vertex.featureB;if(std::find(a.begin(),a.end(),fa)==a.end())a.push_back(fa);if(std::find(b.begin(),b.end(),fb)==b.end())b.push_back(fb);}
    SourcePhysicsContactPlaneCandidate result;const V delta{ray[6],ray[7],ray[8]};
    auto orient=[&](){if(dot(delta,result.normal)>0)for(float&x:result.normal)x=-x;};
    auto aDistance=[&](const V&point){V p;for(unsigned j=0;j<3;++j)p[j]=finite(finite(traceEnd[j])+point[j]);return dot(p,result.normal);};
    if(count==2&&a.size()==2){
        const V p=sourcePhysicsBoxVertexCandidate(box,a[0]),q=sourcePhysicsBoxVertexCandidate(box,a[1]),edge=subtract(q,p);for(float x:delta)finite(x);result.normal=sourcePhysicsContactNormalizeCandidate(cross(edge,cross(edge,delta)));result.distance=aDistance(p);
    }else if(a.size()==3){
        const V p=sourcePhysicsBoxVertexCandidate(box,a[0]),q=sourcePhysicsBoxVertexCandidate(box,a[1]),r=sourcePhysicsBoxVertexCandidate(box,a[2]);result.normal=sourcePhysicsContactNormalizeCandidate(cross(subtract(q,p),subtract(r,p)));orient();result.distance=aDistance(p);
    }else if(a.size()==2&&b.size()==2){
        const V p=sourcePhysicsBoxVertexCandidate(box,a[0]),q=sourcePhysicsBoxVertexCandidate(box,a[1]),r=sourcePhysicsShapeVertexCandidate(shape,b[0]),s=sourcePhysicsShapeVertexCandidate(shape,b[1]);result.normal=sourcePhysicsContactNormalizeCandidate(cross(subtract(q,p),subtract(s,r)));orient();result.distance=aDistance(p);
    }else if(b.size()==3){
        const V p=sourcePhysicsShapeVertexCandidate(shape,b[0]),q=sourcePhysicsShapeVertexCandidate(shape,b[1]),r=sourcePhysicsShapeVertexCandidate(shape,b[2]);result.normal=sourcePhysicsContactNormalizeCandidate(cross(subtract(q,p),subtract(r,p)));orient();result.distance=dot(p,result.normal);
    }else{
        for(unsigned j=0;j<3;++j)result.normal[j]=-finite(ray[9+j]);result.distance=count!=0?dot(sourcePhysicsShapeVertexCandidate(shape,simplex.vertices[0].featureB),result.normal):0.f;
    }
    return result;
}
